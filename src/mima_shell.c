#include <string.h>
#include <stdlib.h>
#include "mima_shell.h"
#include "mima_compiler.h"
#include "log.h"

void mima_shell_print_help()
{
    printf("\n=====================\n mima_shell commands \n=====================\n");
    printf(" s [#].........runs # micro instructions \n");
    printf(" s.............equals \"s 1\"\n");
    printf(" S [#].........runs # instructions \n");
    printf(" S.............equals \"S 1\" or ends current instruction\n");
    printf(" m addr [#]....prints # lines of memory at address\n");
    printf(" m addr........prints 10 lines of memory at address\n");
    printf(" m.............prints 10 lines of memory at IAR\n");
    printf(" i [addr]......sets the IAR to address\n");
    printf(" i.............sets the IAR to zero\n");
    printf(" r.............runs program till end or breakpoint\n");
    printf(" p.............prints mima state\n");
    printf(" L [LOG_LEVEL].sets the log level\n");
    printf(" L.............prints current and available log level\n");
    printf(" q.............quits mima\n");
    printf(" -ENTER-.......repeats last command\n");
    printf("=====================\n");
}

void mima_shell_toggle_breakpoint(uint32_t address){
    for (int i = 0; i < breakpoints_count; ++i)
    {
        if(mima_breakpoints[i].address == address)
        {
            mima_breakpoints[i].active = !mima_breakpoints[i].active;
            return;
        }
    }    
}

void mima_shell_set_IAR(mima_t *mima, char *arg)
{
    char *endptr;
    int address = strtol(arg, &endptr, 0);

    if(endptr == arg)
        address = 0;

    mima->control_unit.IAR = address;
}

void mima_shell_print_memory(mima_t *mima, char *arg)
{
    char *endptr;
    mima_register address;
    uint32_t count;

    address = strtol(arg, &endptr, 0);

    // if no address provided, print from IAR
    if(endptr == arg) address = mima->control_unit.IAR;

    arg = endptr;

    count = strtol(arg, &endptr, 0);
    if(endptr == arg) count = 10;

    mima_print_memory_at(mima, address, count);
}

void mima_shell_set_log_level(char *arg)
{
    if (strncmp(arg + 1, "FATAL", 5) == 0)
    {
        log_set_level(LOG_FATAL);
    }
    else if (strncmp(arg + 1, "ERROR", 5) == 0)
    {
        log_set_level(LOG_ERROR);
    }
    else if (strncmp(arg + 1, "WARN", 4) == 0)
    {
        log_set_level(LOG_WARN);
    }
    else if (strncmp(arg + 1, "INFO", 4) == 0)
    {
        log_set_level(LOG_INFO);
    }
    else if (strncmp(arg + 1, "DEBUG", 5) == 0)
    {
        log_set_level(LOG_DEBUG);
    }
    else if (strncmp(arg + 1, "TRACE", 5) == 0)
    {
        log_set_level(LOG_TRACE);
    }
    else
    {
        printf("Current Log Levels: %s\n", log_get_level_name());
        printf("Available Log Levels: TRACE, DEBUG, INFO, WARN, ERROR, FATAL\n");
    }

}

int mima_shell_execute_command(mima_t *mima, char *input)
{
    switch(input[0])
    {
    case 's':
        mima_run_micro_instruction_steps(mima, input + 1);
        break;
    case 'S':
        mima_run_instruction_steps(mima, input + 1);
        break;
    case 'm':
        mima_shell_print_memory(mima, input + 1);
        break;
    case 'i':
        mima_shell_set_IAR(mima, input + 1);
        break;
    case 'r':
    {

        if (mima->current_instruction.op_code == HLT && mima->control_unit.IAR != 0)
        {
            printf("Last instruction was HLT and IAR != 0. Are you shure you want to run the mima? -> y|N\n\nThis could result in a lot of ADD instructions if you just executed a mima program\nand the IAR points to the end of your defined memory.\nYou can set the IAR (e.g. to zero) with the 'i' command.\n\nBeware: the first run of your program could have modified the mima state or memory.\nThis could result in an endless loop.\n");
            char res = getchar();
            if (res != 'y')
            {
                break;
            }
        }

        mima->control_unit.RUN = mima_true;

        while(mima->control_unit.RUN)
        {
            mima_micro_instruction_step(mima);

            if(mima_hit_active_breakpoint(mima)){
                mima->control_unit.RUN = mima_false;
                mima_shell(mima);
            }
        }
        break;
    }
    case 'L':
        mima_shell_set_log_level(input + 1);
        break;
    case 'p':
        mima_print_state(mima);
        break;
    case 'q':
        return 0;
    default:
        mima_shell_print_help();
        break;
    }

    return 1;
}

int mima_shell(mima_t *mima)
{
    static char last_command[32] = "S";
    char command[32];
    char *input;

    printf("\e[1;32mmima_shell>\e[m ");
    input = fgets(command, 31, stdin);

    if(!input)
    {
        printf("\n");
        return 1;
    }

    input[strlen(input) - 1] = 0;

    if (strlen(input) == 0)
    {
        strcpy(input, last_command);
    }
    else
    {
        strcpy(last_command, input);
    }

    return mima_shell_execute_command(mima, input);
}