#include "s3.h"

int main(int argc, char *argv[])
{
    char line[MAX_LINE];
    char *args[MAX_ARGS];
    command commands[MAX_ARGS];

    char lwd[MAX_PROMPT_LEN - 6];
    int argsc; //
    int commandc; 

    init_lwd(lwd);

    while (1) {
        read_command_line(line);

        parse_command(line, args, &argsc);

        commandc = group_commands(args, argsc, commands);
        
        execute_commands(commands, commandc, lwd);
    }

    return 0;
}