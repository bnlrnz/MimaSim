#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include "mima.h"
#include "mima_compiler.h"

mima_t mima_init()
{
    mima_t mima =
    {
        .control_unit = {
            .IR  = 0,
            .IAR = 0,
            .IP  = 0,
            .TRA = 0,
            .RUN = mima_true
        },
        .memory_unit = {
            .SIR = 0,
            .SAR = 0,
            .memory = 0
        },
        .processing_unit = {
            .ONE = 1,
            .ACC = 0,
            .X   = 0,
            .Y   = 0,
            .Z   = 0,
            .MICRO_CYCLE = 1 // 1 - 12 cycles per instruction
        }
    };

    // we allocate mima words aka 32 Bit integers
    mima.memory_unit.memory = malloc(mima_words * sizeof(mima_word));

    if(!mima.memory_unit.memory)
    {
        log_fatal("Could not allocate Mima memory :(\n");
        assert(0);
    }

    mima_labels = malloc(labels_capacity * sizeof(mima_label));

    if (!mima_labels)
    {
        log_fatal("Could not allocate memory for labels :(\n");
        assert(0);
    }

    return mima;
}

void mima_prompt_print_help()
{
    printf("\n=====================\n mima_shell commands \n=====================\n");
    printf(" s [#].........runs # micro instructions \n");
    printf(" S [#].........runs # instructions \n");
    printf(" S.............runs/ends current instruction\n");
    printf(" m addr [#]....prints # lines of memory at address\n");
    printf(" i addr........sets the IAR to address\n");
    printf(" r.............runs program till end or breakpoint\n");
    printf(" p.............prints mima state\n");
    printf(" L [LOG_LEVEL].prints or sets the log level\n");
    printf(" q.............quits mima\n");
    printf("=====================\n");
}

void mima_prompt_set_IAR(mima_t *mima, char *arg)
{
    char *endptr;
    int address = strtol(arg, &endptr, 0);

    if(endptr == arg)
        address = 0;

    mima->control_unit.IAR = address;
}

void mima_prompt_print_memory(mima_t *mima, char *arg)
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

void mima_prompt_set_log_level(char *arg)
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

int mima_prompt_execute_command(mima_t *mima, char *input)
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
        mima_prompt_print_memory(mima, input + 1);
        break;
    case 'i':
        mima_prompt_set_IAR(mima, input + 1);
        break;
    case 'r':
    {

        if (mima->current_instruction.op_code == HLT)
        {
            printf("Last instruction was HLT. Are you shure you want to run the mima? -> y|N\n\nThis could result in a lot of ADD instructions if you just executed a mima program\nand the IAR points to the end of your defined memory.\nYou can set the IAR (e.g. to zero) with the 'i' command.\n\n");
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

            // TODO: check breakpoints
        }
        break;
    }
    case 'L':
        mima_prompt_set_log_level(input + 1);
        break;
    case 'p':
        mima_print_state(mima);
        break;
    case 'q':
        return 0;
    default:
        mima_prompt_print_help();
        break;
    }

    return 1;
}

int mima_prompt(mima_t *mima)
{
    static char last_command[32] = "S";
    char command[32];
    char *input;

    printf("mima_shell> ");
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

    return mima_prompt_execute_command(mima, input);
}

void mima_run(mima_t *mima, mima_bool interactive)
{
    log_info("\n\n==========================\nStarting Mima...\n==========================\n");
    if (interactive)
    {
        while(mima_prompt(mima)) {};
    }
    else
    {
        while(mima->control_unit.RUN)
        {
            mima_micro_instruction_step(mima);
            // do not check for breakpoints here
        }
    }
}

void mima_run_instruction_steps(mima_t *mima, char *arg)
{
    mima->control_unit.RUN = mima_true;

    char *endptr;
    int steps = strtol(arg, &endptr, 0);

    if(endptr == arg)
        steps = 1;

    // if current instruction was not fully executed, end it
    int current_micro_cycle = mima->processing_unit.MICRO_CYCLE;

    if (current_micro_cycle != 1)
    {
        while( current_micro_cycle++ != 13 && mima->control_unit.RUN )
        {
            mima_micro_instruction_step(mima);
        }
        return;
    }


    steps *= 12; // 12 micro step = 1 instruction
    while( (steps--) && mima->control_unit.RUN )
    {
        mima_micro_instruction_step(mima);

        // TODO: check breakpoints
    }
}

void mima_run_micro_instruction_steps(mima_t *mima, char *arg)
{
    mima->control_unit.RUN = mima_true;

    char *endptr;
    int steps = strtol(arg, &endptr, 0);

    if(endptr == arg)
        steps = 1;

    while( (steps--) && mima->control_unit.RUN )
    {
        mima_micro_instruction_step(mima);

        // TODO: check breakpoints
    }
}

mima_bool mima_compile(mima_t *mima, const char *file_name)
{
    return mima_compile_file(mima, file_name);
}

mima_instruction mima_instruction_decode(mima_t *mima)
{

    mima_word mem = mima->memory_unit.memory[mima->memory_unit.SAR];

    mima_instruction instr;

    if(mem >> 28 != 0xF)
    {
        // standard instruction
        instr.op_code   = mem >> 28;
        instr.value     = mem & 0x0FFFFFFF;
        instr.extended  = mima_false;
    }
    else
    {
        // extended instruction
        instr.op_code   = mem >> 24;
        instr.value     = mem & 0x00FFFFFF;
        instr.extended  = mima_true;
    }

    return instr;
}

mima_bool mima_sar_external(mima_t *mima)
{
    if(mima->current_instruction.value >= 0xC000000)
        return mima_true;
    return mima_false;
}

void mima_micro_instruction_step(mima_t *mima)
{
    //FETCH: first 5 cycles are the same for all instructions
    switch(mima->processing_unit.MICRO_CYCLE)
    {
    case 1:
        mima->memory_unit.SAR   = mima->control_unit.IAR;
        log_trace("Fetch - %02d: IAR -> SAR \t\t\t 0x%08x -> SAR \t\t I/O Read disposed", mima->processing_unit.MICRO_CYCLE, mima->control_unit.IAR);
        mima->processing_unit.X = mima->control_unit.IAR;
        log_trace("Fetch - %02d: IAR -> X \t\t\t 0x%08x -> X \t", mima->processing_unit.MICRO_CYCLE, mima->control_unit.IAR);
        break;
    case 2:
        mima->processing_unit.Y = mima->processing_unit.ONE;
        log_trace("Fetch - %02d: ONE -> Y", mima->processing_unit.MICRO_CYCLE);
        mima->processing_unit.ALU = ADD;
        log_trace("Fetch - %02d: Set ALU to ADD \t\t\t\t\t\t I/O waiting...", mima->processing_unit.MICRO_CYCLE);
        break;
    case 3:
        mima->processing_unit.Z = mima->processing_unit.X + mima->processing_unit.Y;
        log_trace("Fetch - %02d: X + Y -> Z \t\t\t 0x%08x + 0x%08x -> Z \t I/O waiting...", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.X, mima->processing_unit.Y);
        break;
    case 4:
        mima->control_unit.IAR = mima->processing_unit.Z;
        log_trace("Fetch - %02d: Z -> IAR \t\t\t 0x%08x -> IAR", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.Z);
        mima->current_instruction = mima_instruction_decode(mima);
        mima->memory_unit.SIR = mima->memory_unit.memory[mima->memory_unit.SAR];
        log_trace("Fetch - %02d: mem[SAR] -> SIR \t\t mem[0x%08x] -> SIR \t I/O Read done", mima->processing_unit.MICRO_CYCLE, mima->memory_unit.SAR);
        break;
    case 5:
        mima->control_unit.IR = mima->memory_unit.SIR;
        log_trace("Fetch - %02d: SIR -> IR \t\t\t 0x%08x -> IR", mima->processing_unit.MICRO_CYCLE, mima->memory_unit.SIR);
        break;
    default:
    {
        switch(mima->current_instruction.op_code)
        {
        case AND:
        case OR:
        case XOR:
        case ADD:
        case EQL:
            mima_instruction_common(mima);
            break;
        case LDV:
            mima_instruction_LDV(mima);
            break;
        case STV:
            mima_instruction_STV(mima);
            break;
        case LDC:
            mima_instruction_LDC(mima);
            break;
        case HLT:
            mima_instruction_HLT(mima);
            break;
        case JMP:
            mima_instruction_JMP(mima);
            break;
        case JMN:
            mima_instruction_JMN(mima);
            break;
        case NOT:
            mima_instruction_NOT(mima);
            break;
        case RAR:
            mima_instruction_RAR(mima);
            break;
        case RRN:
            mima_instruction_RRN(mima);
            break;
        default:
            log_warn("Invalid instruction - nr.%d - :(\n", mima->current_instruction.op_code);
            assert(0);
        }
    }
    }

    mima->processing_unit.MICRO_CYCLE++;
    if (mima->processing_unit.MICRO_CYCLE > 12)
    {
        mima->processing_unit.MICRO_CYCLE = 1;
    }
}

// ADD, AND, OR, XOR, EQL
void mima_instruction_common(mima_t *mima)
{
    switch(mima->processing_unit.MICRO_CYCLE)
    {
    case 6:
        mima->memory_unit.SAR = mima->control_unit.IR & 0x0FFFFFFF;
        log_trace("%5s - %02d: IR & 0x0FFFFFFF -> SAR \t 0x%08x -> SAR \t\t I/O Read disposed", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->control_unit.IR & 0x0FFFFFFF);
        break;
    case 7:
        mima->processing_unit.X = mima->processing_unit.ACC;
        log_trace("%5s - %02d: ACC -> X \t\t\t 0x%08x -> X \t\t I/O waiting...", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->processing_unit.ACC);
        break;
    case 8:
        log_trace("%5s - %02d: empty \t\t\t\t\t\t\t I/O waiting...", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE);
        break;
    case 9:
        mima->memory_unit.SIR = mima->memory_unit.memory[mima->memory_unit.SAR];
        log_trace("%5s - %02d: mem[SAR] -> SIR \t\t mem[0x%08x] -> SIR \t I/O Read done", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->memory_unit.SAR);
        break;
    case 10:
        mima->processing_unit.Y = mima->memory_unit.SIR;
        log_trace("%5s - %02d: SIR -> Y \t\t\t 0x%08x -> Y", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->memory_unit.SIR);
        mima->processing_unit.ALU = mima->current_instruction.op_code;
        log_trace("%5s - %02d: Set ALU to %s", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima_get_instruction_name(mima->current_instruction.op_code));
        break;
    case 11:
    {
        switch(mima->current_instruction.op_code)
        {
        case AND:
            mima->processing_unit.Z = mima->processing_unit.X & mima->processing_unit.Y;
            log_trace("%5s - %02d: X & Y -> Z \t\t\t 0x%08x & 0x%08x -> Z", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->processing_unit.X, mima->processing_unit.Y);
            break;
        case OR:
            mima->processing_unit.Z = mima->processing_unit.X | mima->processing_unit.Y;
            log_trace("%5s - %02d: X | Y -> Z \t\t\t 0x%08x | 0x%08x -> Z", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->processing_unit.X, mima->processing_unit.Y);
            break;
        case XOR:
            mima->processing_unit.Z = mima->processing_unit.X ^ mima->processing_unit.Y;
            log_trace("%5s - %02d: X ^ Y -> Z \t\t\t 0x%08x ^ 0x%08x -> Z", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->processing_unit.X, mima->processing_unit.Y);
            break;
        case ADD:
            mima->processing_unit.Z = mima->processing_unit.X + mima->processing_unit.Y;
            log_trace("%5s - %02d: X + Y -> Z \t\t\t 0x%08x + 0x%08x -> Z", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->processing_unit.X, mima->processing_unit.Y);
            break;
        case EQL:
            mima->processing_unit.Z = mima->processing_unit.X == mima->processing_unit.Y ? -1 : 0;
            log_trace("%5s - %02d: X == Y ? -1 : 0 -> Z \t 0x%08x == 0x%08x ? -1 : 0 -> Z", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->processing_unit.X, mima->processing_unit.Y);
            break;
        default:
            break;
        }
        break;
    }
    case 12:
        mima->processing_unit.ACC = mima->processing_unit.Z;
        log_trace("%5s - %02d: Z -> ACC \t\t\t 0x%08x -> ACC", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->processing_unit.Z);
        log_info("%5s - ACC = 0x%08x", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.ACC);
        break;
    default:
        log_warn("Invalid micro cycle. Must be between 6-12, was %d :(\n", mima->processing_unit.MICRO_CYCLE);
        assert(0);
    }
}

void mima_instruction_LDV(mima_t *mima)
{
    switch(mima->processing_unit.MICRO_CYCLE)
    {
    case 6:
        mima->memory_unit.SAR = mima->control_unit.IR & 0x0FFFFFFF;
        log_trace("  LDV - %02d: IR & 0x0FFFFFFF -> SAR \t 0x%08x -> SAR \t\t I/O Read disposed", mima->processing_unit.MICRO_CYCLE, mima->control_unit.IR & 0x0FFFFFFF);
        break;
    case 7:
        log_trace("  LDV - %02d: empty \t\t\t\t\t\t\t I/O waiting...");
        break;
    case 8:
        log_trace("  LDV - %02d: empty \t\t\t\t\t\t\t I/O waiting...");
        break;
    case 9:
        mima->memory_unit.SIR = mima->memory_unit.memory[mima->memory_unit.SAR];
        log_trace("  LDV - %02d: mem[SAR] -> SIR \t\t mem[0x%08x] -> SIR \t I/O Read done", mima->processing_unit.MICRO_CYCLE, mima->memory_unit.SAR);
        break;
    case 10:
        mima->processing_unit.ACC = mima->memory_unit.SIR;
        log_trace("  LDV - %02d: SIR -> ACC \t\t\t 0x%08x -> ACC", mima->processing_unit.MICRO_CYCLE, mima->memory_unit.SIR);
        break;
    case 11:
        log_trace("  LDV - %02d: empty");
        break;
    case 12:
        log_trace("  LDV - %02d: empty");
        log_info("  LDV - ACC = 0x%08x", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.ACC);
        break;
    default:
        log_warn("Invalid micro cycle. Must be between 6-12, was %d :(\n", mima->processing_unit.MICRO_CYCLE);
        assert(0);
    }
}

void mima_instruction_STV(mima_t *mima)
{
    switch(mima->processing_unit.MICRO_CYCLE)
    {
    case 6:
        mima->memory_unit.SIR = mima->processing_unit.ACC;
        log_trace("  STV - %02d: ACC -> SIR \t\t\t 0x%08x -> SIR", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.ACC);
        break;
    case 7:
        mima->memory_unit.SAR = mima->control_unit.IR & 0x0FFFFFFF;
        log_trace("  STV - %02d: IR & 0x0FFFFFFF -> SAR \t 0x%08x -> SAR \t\t I/O Write disposed", mima->processing_unit.MICRO_CYCLE, mima->control_unit.IR & 0x0FFFFFFF);
        break;
    case 8:
        log_trace("  STV - %02d: empty \t\t\t\t\t\t\t I/O waiting...", mima->processing_unit.MICRO_CYCLE);
        break;
    case 9:
        log_trace("  STV - %02d: empty \t\t\t\t\t\t\t I/O waiting...", mima->processing_unit.MICRO_CYCLE);
        break;
    case 10:
    {
        mima_register address = mima->control_unit.IR & 0x0FFFFFFF;

        log_trace("  STV - %02d: SIR -> mem[IR & 0x0FFFFFFF] \t 0x%08x -> mem[0x%08x] \t I/O Write done", mima->processing_unit.MICRO_CYCLE, mima->memory_unit.SIR, address);

        // writing to "internal" memory
        if (address < 0xc000000)
        {
            mima->memory_unit.memory[address] = mima->memory_unit.SIR;
            break;
        }
        else
        {
            // writing to IO -> ignoring the  first 4 bits
            if (address == mima_ascii_output)
            {
                printf("%c\n", mima->memory_unit.SIR & 0x0FFFFFFF);
                break;
            }

            if (address == mima_integer_output)
            {
                printf("%d\n", mima->memory_unit.SIR & 0x0FFFFFFF);
                break;
            }

            log_warn("Writing into undefined I/O space. Nothing will happen!");

            break;
        }
    }
    break;
    case 11:
        log_trace("  STV - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 12:
        log_trace("  STV - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        log_info("  STV - 0x%08x -> mem[0x%08x]", mima->memory_unit.SIR, mima->control_unit.IR & 0x0FFFFFFF);
        break;
    default:
        log_warn("Invalid micro cycle. Must be between 6-12, was %d :(\n", mima->processing_unit.MICRO_CYCLE);
        assert(0);
    }
}

void mima_instruction_HLT(mima_t *mima)
{
    log_trace("  HLT - %02d: Setting RUN to false", mima->processing_unit.MICRO_CYCLE);
    log_info("  HLT - Stopping Mima");
    mima->control_unit.RUN = mima_false;
    mima->processing_unit.MICRO_CYCLE = 0;
}

void mima_instruction_LDC(mima_t *mima)
{
    switch(mima->processing_unit.MICRO_CYCLE)
    {
    case 6:
        mima->processing_unit.ACC = mima->control_unit.IR & 0x0FFFFFFF;
        log_trace("  LDC - %02d: IR & 0x0FFFFFFF -> ACC \t 0x%08x -> ACC", mima->processing_unit.MICRO_CYCLE, mima->control_unit.IR & 0x0FFFFFFF);
        break;
    case 7:
        log_trace("  LDC - %02d: empty");
        break;
    case 8:
        log_trace("  LDC - %02d: empty");
        break;
    case 9:
        log_trace("  LDC - %02d: empty");
        break;
    case 10:
        log_trace("  LDC - %02d: empty");
        break;
    case 11:
        log_trace("  LDC - %02d: empty");
        break;
    case 12:
        log_trace("  LDC - %02d: empty");
        log_info("  LDC - ACC = 0x%08x", mima->processing_unit.ACC);
        break;
    default:
        log_warn("Invalid micro cycle. Must be between 6-12, was %d :(\n", mima->processing_unit.MICRO_CYCLE);
        assert(0);
    }
}

void mima_instruction_JMP(mima_t *mima)
{
    switch(mima->processing_unit.MICRO_CYCLE)
    {
    case 6:
        mima->control_unit.IAR = mima->control_unit.IR & 0x0FFFFFFF;
        log_trace("  JMP - %02d: IR & 0x0FFFFFFF -> JMP \t", mima->processing_unit.MICRO_CYCLE);
        break;
    case 7:
        log_trace("  JMP - %02d: empty");
        break;
    case 8:
        log_trace("  JMP - %02d: empty");
        break;
    case 9:
        log_trace("  JMP - %02d: empty");
        break;
    case 10:
        log_trace("  JMP - %02d: empty");
        break;
    case 11:
        log_trace("  JMP - %02d: empty");
        break;
    case 12:
        log_trace("  JMP - %02d: empty");
        log_info("  JMP - to 0x%08x", mima->control_unit.IAR);
        break;
    default:
        log_warn("Invalid micro cycle. Must be between 6-12, was %d :(\n", mima->processing_unit.MICRO_CYCLE);
        assert(0);
    }
}

void mima_instruction_JMN(mima_t *mima)
{
    switch(mima->processing_unit.MICRO_CYCLE)
    {
    case 6:
        if((int32_t)mima->processing_unit.ACC < 0)
        {
            mima->control_unit.IAR = mima->control_unit.IR & 0x0FFFFFFF;
            log_trace("  JMN - %02d: ACC = 0x%08x - Jumping to: 0x%08x", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.ACC, mima->control_unit.IR & 0x0FFFFFFF);
        }
        else
        {
            log_trace("  JMN - %02d: ACC = 0x%08x - No jump taken", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.ACC, mima->control_unit.IR & 0x0FFFFFFF);
        }
        break;
    case 7:
        log_trace("  JMN - %02d: empty");
        break;
    case 8:
        log_trace("  JMN - %02d: empty");
        break;
    case 9:
        log_trace("  JMN - %02d: empty");
        break;
    case 10:
        log_trace("  JMN - %02d: empty");
        break;
    case 11:
        log_trace("  JMN - %02d: empty");
        break;
    case 12:
        log_trace("  JMN - %02d: empty");

        if((int32_t)mima->processing_unit.ACC < 0)
        {
            log_info("  JMN - taken to 0x%08x", mima->control_unit.IR & 0x0FFFFFFF);
        }
        else
        {
            log_info("  JMN - not taken");
        }
        break;
    default:
        log_warn("Invalid micro cycle. Must be between 6-12, was %d :(\n", mima->processing_unit.MICRO_CYCLE);
        assert(0);
    }
}

void mima_instruction_NOT(mima_t *mima)
{
    switch(mima->processing_unit.MICRO_CYCLE)
    {
    case 6:
        log_trace("  NOT - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 7:
        mima->processing_unit.X = mima->processing_unit.ACC;
        log_trace("  NOT - %02d: ACC -> X \t\t\t 0x%08x -> X", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.ACC);
        break;
    case 8:
        log_trace("  NOT - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 9:
        log_trace("  NOT - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 10:
        mima->processing_unit.ALU = NOT;
        log_trace("  NOT - %02d: Set ALU to NOT", mima->processing_unit.MICRO_CYCLE);
        break;
    case 11:
        mima->processing_unit.Z = ~mima->processing_unit.X;
        log_trace("  NOT - %02d: ~X -> Z \t\t\t ~0x%08x -> Z", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.X);
        break;
    case 12:
        mima->processing_unit.ACC = mima->processing_unit.Z;
        log_trace("  NOT - %02d: Z -> ACC \t\t\t 0x%08x -> ACC", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.Z);
        log_info("  NOT - ACC = 0x%08x", mima->processing_unit.ACC);
        break;
    default:
        log_warn("Invalid micro cycle. Must be between 6-12, was %d :(\n", mima->processing_unit.MICRO_CYCLE);
        assert(0);
    }
}

void mima_instruction_RAR(mima_t *mima)
{
    switch(mima->processing_unit.MICRO_CYCLE)
    {
    case 6:
        log_trace("  RAR - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 7:
        mima->processing_unit.X = mima->processing_unit.ACC;
        log_trace("  RAR - %02d: ACC -> X \t\t\t 0x%08x -> X", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.ACC);
        break;
    case 8:
        log_trace("  RAR - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 9:
        log_trace("  RAR - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 10:
        mima->processing_unit.Y = mima->processing_unit.ONE;
        log_trace("  RAR - %02d: ONE -> Y", mima->processing_unit.MICRO_CYCLE);
        mima->processing_unit.ALU = RAR;
        log_trace("  RAR - %02d: Set ALU to RAR", mima->processing_unit.MICRO_CYCLE);
        break;
    case 11:
    {
        int32_t shifted = mima->processing_unit.X >> mima->processing_unit.Y;
        int32_t rotated = mima->processing_unit.X << (32 - mima->processing_unit.Y);
        int32_t combined = shifted | rotated;
        mima->processing_unit.Z = combined;
        log_trace("  RAR - %02d: (X >>> 1) | (X << 31) -> Z", mima->processing_unit.MICRO_CYCLE);
        break;
    }
    case 12:
        mima->processing_unit.ACC = mima->processing_unit.Z;
        log_trace("  RAR - %02d: Z -> ACC \t\t\t 0x%08x -> ACC", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.Z);
        log_info("  RAR - ACC = 0x%08x", mima->processing_unit.ACC);
        break;
    default:
        log_warn("Invalid micro cycle. Must be between 6-12, was %d :(\n", mima->processing_unit.MICRO_CYCLE);
        assert(0);
    }
}

void mima_instruction_RRN(mima_t *mima)
{
    switch(mima->processing_unit.MICRO_CYCLE)
    {
    case 6:
        log_trace("  RRN - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 7:
        mima->processing_unit.X = mima->processing_unit.ACC;
        log_trace("  RRN - %02d: ACC -> X \t\t\t 0x%08x -> X", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.ACC);
        break;
    case 8:
        log_trace("  RRN - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 9:
        log_trace("  RRN - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 10:
        mima->processing_unit.Y = mima->control_unit.IR & 0x00FFFFFF;
        log_trace("  RRN - %02d: IR & 0x00FFFFFF -> Y \t 0x%08x -> Y", mima->processing_unit.MICRO_CYCLE, mima->control_unit.IR & 0x00FFFFFF);
        mima->processing_unit.ALU = RRN;
        log_trace("  RRN - %02d: Set ALU to RRN", mima->processing_unit.MICRO_CYCLE);
        break;
    case 11:
    {
        int32_t shifted = mima->processing_unit.X >> mima->processing_unit.Y;
        int32_t rotated = mima->processing_unit.X << (32 - mima->processing_unit.Y);
        int32_t combined = shifted | rotated;
        mima->processing_unit.Z = combined;
        log_trace("  RRN - %02d: (X >>> Y) | (X << (32-Y)) -> Z", mima->processing_unit.MICRO_CYCLE);
        break;
    }
    case 12:
        mima->processing_unit.ACC = mima->processing_unit.Z;
        log_trace("  RRN - %02d: Z -> ACC \t\t\t 0x%08x -> ACC", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.Z);
        log_info("  RRN - ACC = 0x%08x", mima->processing_unit.ACC);
        break;
    default:
        log_warn("Invalid micro cycle. Must be between 6-12, was %d :(\n", mima->processing_unit.MICRO_CYCLE);
        assert(0);
    }
}

void mima_print_memory_at(mima_t *mima, mima_register address, uint32_t count)
{
    if (address < 0 || address > mima_words - 1)
    {
        log_warn("Invalid address %d\n", address);
        return;
    }

    for (int i = 0; address + i < mima_words - 1 && i < count; ++i)
    {
        printf("mem[0x%08x] = 0x%08x\n", address + i, mima->memory_unit.memory[address + i]);
    }
}

void mima_print_memory_unit_state(mima_t *mima)
{
    printf("\n");
    printf("=======MEMORY UNIT=======\n");
    printf(" SIR\t    = 0x%08x\n", mima->memory_unit.SIR);
    printf(" SAR\t    = 0x%08x\n", mima->memory_unit.SAR);
    printf("=========================\n");
}

void mima_print_control_unit_state(mima_t *mima)
{
    printf("\n");
    printf("=======CONTROL UNIT======\n");
    printf(" IR \t    = 0x%08x\n", mima->control_unit.IR);
    printf(" IAR\t    = 0x%08x\n", mima->control_unit.IAR);
    printf(" IP \t    = 0x%08x\n", mima->control_unit.IP);
    printf(" TRA\t    = %s\n", mima->control_unit.TRA ? "true" : "false");
    printf(" RUN\t    = %s\n", mima->control_unit.RUN ? "true" : "false");
    printf("=========================\n");
}

void mima_print_processing_unit_state(mima_t *mima)
{
    printf("\n");
    printf("=======PROC UNIT=========\n");
    printf(" ACC\t    = 0x%08x\n", mima->processing_unit.ACC);
    printf(" X  \t    = 0x%08x\n", mima->processing_unit.X);
    printf(" Y  \t    = 0x%08x\n", mima->processing_unit.Y);
    printf(" Z  \t    = 0x%08x\n", mima->processing_unit.Z);
    printf(" MICRO_CYCLE= %02d\n", mima->processing_unit.MICRO_CYCLE);
    printf(" ALU\t    = %s\n", mima_get_instruction_name(mima->processing_unit.ALU));
    printf("=========================\n");
}

void mima_print_state(mima_t *mima)
{
    mima_print_processing_unit_state(mima);
    mima_print_control_unit_state(mima);
    mima_print_memory_unit_state(mima);
}

const char *mima_get_instruction_name(mima_instruction_type op_code)
{
    switch(op_code)
    {
    case ADD:
        return "ADD";
    case AND:
        return "AND";
    case OR:
        return "OR";
    case XOR:
        return "XOR";
    case LDV:
        return "LDV";
    case STV:
        return "STV";
    case LDC:
        return "LDC";
    case JMP:
        return "JMP";
    case JMN:
        return "JMN";
    case EQL:
        return "EQL";
    case HLT:
        return "HLT";
    case NOT:
        return "NOT";
    case RAR:
        return "RAR";
    case RRN:
        return "RRN";
    }
    return "INVALID";
}

void mima_delete(mima_t *mima)
{
    free(mima->memory_unit.memory);
    free(mima_labels);
}


