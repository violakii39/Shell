#include "s3.h"



void init_lwd(char *lwd)
{
    if (getcwd(lwd, MAX_PROMPT_LEN - 6) == NULL) {
        perror("getcwd");
        return;
    }
}


///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    char cwd[MAX_PROMPT_LEN-6];
    if (getcwd(cwd, MAX_PROMPT_LEN - 6) == NULL) {
        perror("getcwd");
        return;
    }
    snprintf(shell_prompt, MAX_PROMPT_LEN, "[%s s3]$", cwd);
}



bool command_with_redirection(char line[]){
    int i=0;
    while(line[i]!='\0'){
        if(line[i]=='>'||line[i]=='<') return true;
        i++;
    }
    return false;
}

///Prints a shell prompt and reads input from the user
void read_command_line(char line[])
{
    char shell_prompt[MAX_PROMPT_LEN];
    construct_shell_prompt(shell_prompt);
    printf("%s", shell_prompt);

    ///See man page of fgets(...)
    if (fgets(line, MAX_LINE, stdin) == NULL)
    {
        perror("fgets failed");
        exit(1);
    }
    ///Remove newline (enter)
    line[strlen(line) - 1] = '\0'; 
}


int split_operators(char *token, char *args[], int *argsc){
    int p1=0;
    int p2=0;
    while(token[p2]!='\0'){
        if(token[p2]=='<'||token[p2]=='>'||token[p2]=='|'||token[p2]==';'||token[p2]=='('||token[p2]==')'){
            char *op;
            int op_len=1;
            if(token[p2]=='<'){
                op="<";
            }
            else if(token[p2]=='>'&&token[p2+1]=='>'){
                op_len++;
                op=">>";
            }
            else if(token[p2]=='>'){
                op=">";
            }
            else if(token[p2]=='|'){
                op="|";
            }
            else if(token[p2]==';'){
                op=";";
            }
            else if(token[p2]=='('){
                op="(";
            }
            else if(token[p2]==')'){
                op=")";
            }
            if(p2>p1){
                if(*argsc>=MAX_ARGS-1) return -1;
                token[p2]='\0';
                args[(*argsc)++]=&token[p1];
            }
            if(*argsc>=MAX_ARGS-1) return -1;
            args[(*argsc)++]=op;

            p2+=op_len;
            p1=p2;
        }
        else p2++;   
    }

    if(p2>p1){
        if(*argsc>=MAX_ARGS-1) return -1;
        args[(*argsc)++]=&token[p1];
    }
    return 0;
}


void parse_command(char line[], char *args[], int *argsc){
    *argsc=0;
    args[0]=NULL;
    char *token=strtok(line, " ");
    while(token!=NULL){
        if(split_operators(token, args, argsc)==-1){
            *argsc=0;
            args[0]=NULL;
            return;
        }

        token=strtok(NULL, " ");
    }
    args[*argsc]=NULL;
}

///Launch related functions
void child(char *args[], int argsc)
{
    ///Implement this function:

    ///Use execvp to load the binary 
    ///of the command specified in args[ARG_PROGNAME].
    ///For reference, see the code in lecture 3.
    execvp(args[ARG_PROGNAME],args);
    perror("execvp");
    exit(1);
}

int group_commands(char *args[], int argsc, command commands[]){
    if(argsc<0||argsc>=MAX_ARGS) return -1;
    if(argsc==0) return 0;

    int p1=0;
    int commandc=0;
    bool at_end=false;
    bool is_pipe;
    bool is_batch;
    for(int p2=0;p2<=argsc;p2++){
        at_end=(p2==argsc);
        is_pipe=(!at_end && strcmp(args[p2],"|")==0);
        is_batch=(!at_end && strcmp(args[p2],";")==0);
        if(!at_end && !is_batch && !is_pipe) continue;
        if(p2==p1){
            if(at_end && commandc>0 && !commands[commandc-1].pipe_to_next) break;
            fprintf(stderr, "empty command error\n");
            return -1;
        }

        commands[commandc].args=&args[p1];
        commands[commandc].argsc=(p2-p1);
        commands[commandc].pipe_to_next=is_pipe;
        commandc++;
        args[p2]=NULL;
        p1=p2+1;
    }
    return commandc;
}

void execute_commands(command commands[], int commandc, char *lwd){
    int prev_pipeout=-1;
    int pending=0;

    //for piped commands separated by |, first command's stdout and second command's stdin goes into the pipe
    for(int i=0;i<commandc;i++){
        command *cmd=&commands[i];

        //no previous incoming pipe & not piped to next command -> single command, execute directly
        if(prev_pipeout==-1 && !cmd->pipe_to_next){
            launch_program(cmd->args, cmd->argsc, lwd);
            reap();
            continue;
        }

        //command is piped, initialize pipe
        int fd[2];
        if(cmd->pipe_to_next && pipe(fd)==-1){
            perror("pipe");
            exit(1);
        }

        //fd[0] = read end of pipe, fd[1] = write end of pipe

        pid_t pid=fork();

        if(pid<0){
            perror("fork");
            exit(1);
        }

        if(pid==0){
            if(prev_pipeout!=-1){ //the previous command piped, configure stdin to previous pipe's output
                dup2(prev_pipeout, STDIN_FILENO);
                close(prev_pipeout);//close redundant descriptors
            }
            if(cmd->pipe_to_next){ //current command pipes to next command, configure stdout to pipe's input
                close(fd[0]);//close unused end
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);//closed redundant descriptors
            }
            configure_redirection_dup(cmd->args, cmd->argsc);//configure redireection within command
            child(cmd->args,cmd->argsc);
            exit(1);
        }

        pending++;//record +1 pending piped command for later reaping
        if(prev_pipeout!=-1) close(prev_pipeout);
        if(cmd->pipe_to_next){ //current command didn't reach end of pipe line, do not reap, keep launching next command
            close(fd[1]);//close fd[1] on parents end
            prev_pipeout=fd[0];//current command piped, send input end of pipe to next command for dup configuration
        }
        else{//reached end of pipeline
            prev_pipeout=-1;
            for(int j=0;j<pending;j++) reap();
            pending=0;
        }
    }

}

void configure_redirection_dup(char *args[], int argsc){
    for (int i = 0; i < argsc; i++){ // configure every redirection in the command
        int dup_fd[2];
        if (strcmp(args[i], "<") == 0) {
            dup_fd[0] = open(args[i+1], O_RDONLY);
            dup_fd[1] = STDIN_FILENO;
        }
        else if (strcmp(args[i], ">") == 0) {
            dup_fd[0] = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup_fd[1] = STDOUT_FILENO;
        }
        else if (strcmp(args[i], ">>") == 0) {
            dup_fd[0] = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup_fd[1] = STDOUT_FILENO;
        }  

        else{ //if there is no redirection, don't execute the following code
            continue;
        }

        if(dup_fd[0] == -1){
            perror("file error");
            _exit(1);
        }
        if(dup2(dup_fd[0],dup_fd[1]) == -1){
            perror("dup2 error");
            _exit(1);
        }

        close(dup_fd[0]);

        for (int j = i; j < argsc - 2; j++) { //dup configuration finished, remove refirection tokens from args by shifting 2
            args[j] = args[j + 2];
        }
        argsc -= 2;
        args[argsc] = NULL;
        i--; //examine the current instruction again for potentially shifted in redirection operators
    }
}

void change_directory (char *args[], int argsc, char *lwd){
    char cwd[MAX_PROMPT_LEN-6];
    if (getcwd(cwd, MAX_PROMPT_LEN - 6) == NULL) {
        perror("getcwd");
        return;
    }

    if (argsc == 1){
        if (chdir(getenv("HOME")) == -1){
            perror("chdir error");
            return;
        }
    }
    else if (strcmp(args[1], "-") == 0){
        if (chdir(lwd) == -1){
            perror("chdir error");
            return;
        }
    }
    else{
        if (chdir(args[1]) == -1){
            perror("chdir error");
            return;
        }
    }

    strcpy(lwd,cwd);
}

void launch_program(char *args[], int argsc, char *lwd)
{
    ///Implement this function:
    if (argsc==0||args[ARG_PROGNAME]==NULL) return;

    if (strcmp(args[ARG_PROGNAME],"exit")==0) exit(0);

    if (strcmp(args[ARG_PROGNAME],"cd")==0){
        change_directory(args, argsc, lwd);
        return;
    }

    pid_t pid=fork();

    if(pid<0){
        printf("fork failed\n");
        exit(1);
    } 
    else if(pid==0) {
        configure_redirection_dup(args,argsc);
        child(args,argsc);
        exit(1);
    }
    ///fork() a child process.
    ///In the child part of the code,
    ///call child(args, argv)
    ///For reference, see the code in lecture 2.

    ///Handle the 'exit' command;
    ///so that the shell, not the child process,
    ///exits.
}
