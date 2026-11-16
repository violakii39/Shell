#include "s3.h"

int main(int argc, char *argv[])
{
    char line[MAX_LINE];
    char *args[MAX_ARGS];
    command commands[MAX_ARGS];

    char lwd[MAX_PROMPT_LEN - 6];
    int argsc;

    init_lwd(lwd);

    while (1) {
        read_command_line(line);
        parse_command(line, args, &argsc);

        // 把整行 Token 分成各条命令
        int commandc =
            group_commands(args, argsc, commands);

        if (commandc <= 0)
            continue;

        if (commandc == 1) {
            // 单条命令：继续使用原来的执行方式
            launch_program(commands[0].args,
                           commands[0].argsc,
                           lwd);
            reap();
        }
        else {
            // 多条命令：处理管道和分号
            execute_commands(commands, commandc, lwd);
        }
    }

    return 0;
}