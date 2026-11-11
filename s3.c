#include "s3.h"

///Simple for now, but will be expanded in a following section
void construct_shell_prompt(char shell_prompt[])
{
    strcpy(shell_prompt, "[s3]$ ");
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
        if(token[p2]=='<'||token[p2]=='>'){
            char *op;
            int op_len=1;
            if(token[p2]=='<'){
                op="<";
            }
            else if(token[p2+1]=='>'){
                op_len++;
                op=">>";
            }
            else{
                op=">";
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

bool command_with_redirection(char line[]){
    int i=0;
    while(line[i]!=NULL){
        if(line[i]=='>'||line[i]=='<') return true;
        i++;
    }
    return false;
}