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