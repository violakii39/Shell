#include "s3.h"

int main(int argc, char *argv[]){

    ///Stores the command line input
    char line[MAX_LINE];

    ///Stores pointers to command arguments.
    ///The first element of the array is the command name.
    char *args[MAX_ARGS];

    ///The last (previous) working directory 
    char lwd[MAX_PROMPT_LEN-6];

    init_lwd(lwd);

    ///Stores the number of arguments
    int argsc;

    while (1) {
        read_command_line(line);
        parse_command(line, args, &argsc);
        launch_program(args, argsc, lwd);
        reap();
    }
    return 0;
}