#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    strcpy(shell_prompt, "[s3]$ ");
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

void parse_command(char line[], char *args[], int *argsc)
{
    int p1=0;
    int p2=0;
    char *tmp_token;
    *argsc=0;
    while(line[p2]!='\0'){
        if(line[p2]==' '){
            if(p2>p1) args[(*argsc)++]=&line[p1];
            line[p2]='\0';
            p2++;
            while(line[p2]==' ') p2++;
            p1=p2;
        }
        else if(line[p2]=='<'||line[p2]=='>'){
            if(p2>p1) {tmp_token=malloc(p2-p1+1);
            memcpy(tmp_token,&line[p1],p2-p1);
            tmp_token[p2-p1]='\0';
            args[(*argsc)++]=tmp_token;}
            if(line[p2]=='>'&&line[p2+1]=='>') {
                tmp_token=malloc(3);
                tmp_token[0]='>';
                tmp_token[1]='>';
                tmp_token[2]='\0';
                args[(*argsc)++]=tmp_token;
                p2+=2;
                while(line[p2]==' ')p2++;
                p1=p2;
            }
            else {
                tmp_token=malloc(2);
                tmp_token[0]=line[p2];
                tmp_token[1]='\0';
                args[(*argsc)++]=tmp_token;
                p2++;
                while(line[p2]==' ')p2++;
                p1=p2;
            }
        }
        else p2++;
    }

    if(p2>p1) args[(*argsc)++]=&line[p1];
    
    args[*argsc] = NULL; ///args must be null terminated
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

void launch_program(char *args[], int argsc)
{
    ///Implement this function:
    if (argsc==0||args[ARG_PROGNAME]==NULL) return;
    if (strcmp(args[ARG_PROGNAME],"exit")==0) exit(0);

    
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
