#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include "log.h"
#include "mima.h"
#include "mima_compiler.h"
#include "mima_shell.h"

#include "mima_webasm_interface.h"

mima_t mima_init()
{
    log_set_level(LOG_TRACE);

    mima_t mima =
    {
        .control_unit = {
            .IR  = 0,
            .IAR = 0,
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
        },
        .stv_callbacks_count = 0,
        .stv_callbacks_capacity = 8,
        .ldv_callbacks_count = 0,
        .ldv_callbacks_capacity = 8
    };

    // we allocate mima words aka 32 Bit integers
    mima.memory_unit.memory = malloc(mima_words * sizeof(mima_word));
    memset(mima.memory_unit.memory, 0, mima_words * sizeof(mima_word));

    if(!mima.memory_unit.memory)
    {
        log_fatal("Could not allocate Mima memory :(\n");
        assert(0);
    }

    mima.mima_labels = malloc(labels_capacity * sizeof(mima_label));
    memset(mima.mima_labels, 0, labels_capacity * sizeof(mima_label));

    if (!mima.mima_labels)
    {
        log_fatal("Could not allocate memory for labels :(\n");
        assert(0);
    }

    mima.mima_breakpoints = malloc(breakpoints_capacity * sizeof(mima_breakpoint));
    memset(mima.mima_breakpoints, 0, breakpoints_capacity * sizeof(mima_breakpoint));

    if (!mima.mima_breakpoints)
    {
        log_fatal("Could not allocate memory for breakpoints :(\n");
        assert(0);
    }

    mima.stv_callbacks = malloc(mima.stv_callbacks_capacity * sizeof(mima_io_callback));
    memset(mima.stv_callbacks, 0, mima.stv_callbacks_capacity * sizeof(mima_io_callback));

    if (!mima.stv_callbacks)
    {
        log_fatal("Could not allocate memory for stv_callbacks :/\n");
        assert(0);
    }

    mima.ldv_callbacks = malloc(mima.ldv_callbacks_capacity * sizeof(mima_io_callback));
    memset(mima.ldv_callbacks, 0, mima.ldv_callbacks_capacity * sizeof(mima_io_callback));

    if (!mima.ldv_callbacks)
    {
        log_fatal("Could not allocate memory for ldv_callbacks :/\n");
        assert(0);
    }

    return mima;
}

void mima_set_run(mima_t* mima, mima_bool run)
{
    if (mima->control_unit.RUN == run)
    {
        return;
    }

    mima->control_unit.RUN = run;
    mima_wasm_register_transfer(mima, RUN, IMMEDIATE, run);
}

void mima_run(mima_t *mima, mima_bool interactive)
{
    mima_set_run(mima, mima_true);

    log_info("\n\n==========================\nStarting Mima...\n==========================\n");
    if (interactive)
    {
        while(mima_shell(mima)) {};
    }
    else
    {
        while(mima->control_unit.RUN && mima->control_unit.TRA == mima_false)
        {
            mima_micro_instruction_step(mima);
            // do not check for breakpoints here -> it's non interactive mode
        }
    }
}

void mima_run_micro_instruction_step(mima_t *mima){
    mima_run_micro_instruction_steps(mima, " 1\n");
}

void mima_run_instruction_step(mima_t *mima){
    mima_run_instruction_steps(mima, " 1\n");
}

void mima_run_instruction_steps(mima_t *mima, char *arg)
{
    mima_set_run(mima, mima_true);

    char *endptr;
    int steps = strtol(arg, &endptr, 0);

    if(endptr == arg)
        steps = 1;

    // if current instruction was not fully executed, end it
    int current_micro_cycle = mima->processing_unit.MICRO_CYCLE;

    if (current_micro_cycle != 1)
    {
        while( current_micro_cycle++ != 13 && mima->control_unit.RUN && mima->control_unit.TRA == mima_false)
        {
            mima_micro_instruction_step(mima);
        }
        return;
    }


    steps *= 12; // 12 micro step = 1 instruction
    while( (steps--) && mima->control_unit.RUN && mima->control_unit.TRA == mima_false)
    {
        mima_micro_instruction_step(mima);

        if (mima_hit_active_breakpoint(mima))
        {
            mima_set_run(mima, mima_false);
            mima_wasm_hit_breakpoint();
#ifndef WEBASM
            mima_shell(mima);
#endif
        }
    }
}

void mima_run_micro_instruction_steps(mima_t *mima, char *arg)
{
    char *endptr;
    int steps = strtol(arg, &endptr, 0);

    if(endptr == arg)
        steps = 1;

    while( (steps--) && mima->control_unit.RUN && mima->control_unit.TRA == mima_false)
    {
        mima_micro_instruction_step(mima);

        if (mima_hit_active_breakpoint(mima))
        {
            mima_set_run(mima, mima_false);
            mima_wasm_hit_breakpoint();
#ifndef WEBASM
            mima_shell(mima);
#endif
        }
    }
}

mima_bool mima_compile_f(mima_t *mima, const char *file_name)
{
    return mima_compile_file(mima, file_name);
}

mima_bool mima_compile_s(mima_t* mima, char* source_code)
{
    return mima_compile_string(mima, source_code);
}

mima_instruction mima_instruction_decode_mem(mima_word mem)
{
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

mima_instruction mima_instruction_decode(mima_t *mima)
{
    return mima_instruction_decode_mem(mima->memory_unit.memory[mima->memory_unit.SAR]);
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
        mima_wasm_register_transfer(mima, SAR, IAR, mima->control_unit.IAR);
        log_trace("Fetch - %02d: IAR -> SAR \t\t\t 0x%08x -> SAR \t\t I/O Read disposed", mima->processing_unit.MICRO_CYCLE, mima->control_unit.IAR);
        mima->processing_unit.X = mima->control_unit.IAR;
        mima_wasm_register_transfer(mima, X, IAR, mima->control_unit.IAR);
        log_trace("Fetch - %02d: IAR -> X \t\t\t 0x%08x -> X \t", mima->processing_unit.MICRO_CYCLE, mima->control_unit.IAR);
        break;
    case 2:
        mima->processing_unit.Y = mima->processing_unit.ONE;
        mima_wasm_register_transfer(mima, Y, ONE, mima->processing_unit.ONE);
        log_trace("Fetch - %02d: ONE -> Y", mima->processing_unit.MICRO_CYCLE);
        mima->processing_unit.ALU = ADD;
        log_trace("Fetch - %02d: Set ALU to ADD \t\t\t\t\t\t I/O waiting...", mima->processing_unit.MICRO_CYCLE);
        break;
    case 3:
        mima->processing_unit.Z = mima->processing_unit.X + mima->processing_unit.Y;
        mima_wasm_register_transfer(mima, Z, ALU, mima->processing_unit.Z);
        log_trace("Fetch - %02d: X + Y -> Z \t\t\t 0x%08x + 0x%08x -> Z \t I/O waiting...", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.X, mima->processing_unit.Y);
        break;
    case 4:
        mima->control_unit.IAR = mima->processing_unit.Z;
        mima_wasm_register_transfer(mima, IAR, Z, mima->processing_unit.Z);
        log_trace("Fetch - %02d: Z -> IAR \t\t\t 0x%08x -> IAR", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.Z);
        mima->current_instruction = mima_instruction_decode(mima);
        mima->memory_unit.SIR = mima->memory_unit.memory[mima->memory_unit.SAR];
        mima_wasm_register_transfer(mima, SIR, MEMORY, mima->memory_unit.memory[mima->memory_unit.SAR]);
        log_trace("Fetch - %02d: mem[SAR] -> SIR \t\t mem[0x%08x] -> SIR \t I/O Read done", mima->processing_unit.MICRO_CYCLE, mima->memory_unit.SAR);
        break;
    case 5:
        mima->control_unit.IR = mima->memory_unit.SIR;
        mima_wasm_register_transfer(mima, IR, SIR, mima->memory_unit.SIR);
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

    mima_wasm_register_transfer(mima, MICRO_CYCLE, SIR, mima->processing_unit.MICRO_CYCLE);
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
        mima_wasm_register_transfer(mima, SAR, IR, mima->control_unit.IR & 0x0FFFFFFF);
        log_trace("%5s - %02d: IR & 0x0FFFFFFF -> SAR \t 0x%08x -> SAR \t\t I/O Read disposed", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->control_unit.IR & 0x0FFFFFFF);
        break;
    case 7:
        mima->processing_unit.X = mima->processing_unit.ACC;
        mima_wasm_register_transfer(mima, X, ACC, mima->processing_unit.ACC);
        log_trace("%5s - %02d: ACC -> X \t\t\t 0x%08x -> X \t\t I/O waiting...", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->processing_unit.ACC);
        break;
    case 8:
        log_trace("%5s - %02d: empty \t\t\t\t\t\t\t I/O waiting...", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE);
        break;
    case 9:
        mima->memory_unit.SIR = mima->memory_unit.memory[mima->memory_unit.SAR];
        mima_wasm_register_transfer(mima, SIR, MEMORY, mima->memory_unit.memory[mima->memory_unit.SAR]);
        log_trace("%5s - %02d: mem[SAR] -> SIR \t\t mem[0x%08x] -> SIR \t I/O Read done", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->memory_unit.SAR);
        break;
    case 10:
        mima->processing_unit.Y = mima->memory_unit.SIR;
        mima_wasm_register_transfer(mima, Y, SIR, mima->memory_unit.SIR);
        log_trace("%5s - %02d: SIR -> Y \t\t\t 0x%08x -> Y", mima_get_instruction_name(mima->current_instruction.op_code), mima->processing_unit.MICRO_CYCLE, mima->memory_unit.SIR);
        mima->processing_unit.ALU = mima->current_instruction.op_code;
        mima_wasm_register_transfer(mima, ALU, IMMEDIATE, mima->current_instruction.op_code);
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

        mima_wasm_register_transfer(mima, Z, ALU, mima->processing_unit.Z);

        break;
    }
    case 12:
        mima->processing_unit.ACC = mima->processing_unit.Z;
        mima_wasm_register_transfer(mima, ACC, Z, mima->processing_unit.Z);
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
        mima_wasm_register_transfer(mima, SAR, IR, mima->control_unit.IR & 0x0FFFFFFF);
        log_trace("  LDV - %02d: IR & 0x0FFFFFFF -> SAR \t 0x%08x -> SAR \t\t I/O Read disposed", mima->processing_unit.MICRO_CYCLE, mima->control_unit.IR & 0x0FFFFFFF);
        break;
    case 7:
        log_trace("  LDV - %02d: empty \t\t\t\t\t\t\t I/O waiting...", mima->processing_unit.MICRO_CYCLE);
        break;
    case 8:
        log_trace("  LDV - %02d: empty \t\t\t\t\t\t\t I/O waiting...", mima->processing_unit.MICRO_CYCLE);
        break;
    case 9:
    {
        mima_register address = mima->memory_unit.SAR;

        if (address < 0xC000000)
        {
            // internal memory
            mima->memory_unit.SIR = mima->memory_unit.memory[mima->memory_unit.SAR];
            mima_wasm_register_transfer(mima, SIR, MEMORY, mima->memory_unit.memory[mima->memory_unit.SAR]);
            log_trace("  LDV - %02d: mem[SAR] -> SIR \t\t mem[0x%08x] -> SIR \t I/O Read done", mima->processing_unit.MICRO_CYCLE, mima->memory_unit.SAR);
        }
        else
        {
            mima->control_unit.TRA = mima_true;
            mima_wasm_register_transfer(mima, TRA, IMMEDIATE, mima_true);
            
            // I/O space
            if (address == mima_char_input)
            {
                char res;
                
                #ifdef WEBASM
                    res =  mima_wasm_input_single_char();
                #else
                    printf("Waiting for single char:");
                    res = getchar();
                #endif

                mima->memory_unit.SIR = res;
                mima_wasm_register_transfer(mima, SIR, IOMEMORY, res);
                log_trace("  LDV - %02d: Char -> SIR \t\t '%c' -> SIR \t I/O Read done", mima->processing_unit.MICRO_CYCLE, res);
                mima->control_unit.TRA = mima_false;
                mima_wasm_register_transfer(mima, TRA, IMMEDIATE, mima_false);
                break;
            }

            if (address == mima_integer_input)
            {
                int number = 0;

                #ifdef WEBASM
                    number =  mima_wasm_input_single_int();
                #else
                    printf("Waiting for number (dec or hex [with 0x-prefix]):");
                    char number_string[32] = {0};
                    char *endptr;
                    fgets(number_string, 31, stdin);
                    number = strtol(number_string, &endptr, 0);
                #endif
                
                mima->memory_unit.SIR = number;
                mima_wasm_register_transfer(mima, SIR, IOMEMORY, number);
                log_trace("  LDV - %02d: Integer -> SIR \t\t %d aka 0x%08x -> SIR \t I/O Read done", mima->processing_unit.MICRO_CYCLE, number, number);
                mima->control_unit.TRA = mima_false;
                mima_wasm_register_transfer(mima, TRA, IMMEDIATE, mima_false);
                break;
            }

            mima_bool taken = mima_false;
            for (size_t i = 0; i < mima->ldv_callbacks_count; ++i)
            {
                if (mima->ldv_callbacks[i].address == address)
                {
                    log_info("  LDV - %02d: Calling callback function...", mima->processing_unit.MICRO_CYCLE);
                    mima->ldv_callbacks[i].func(mima, &mima->memory_unit.SIR);

                    if (mima->control_unit.TRA == mima_true)
                    {
                        log_warn("  LDV - %02d: I/O callback functions should always set the TRA flag to mima_false, when finished! Otherwise, the mima stops...", mima->processing_unit.MICRO_CYCLE);
                    }

                    taken = mima_true;
                    break;
                }
            }

            if(!taken)
                log_warn("Reading from undefined I/O space. This will (probably) be garbage!");
        }
        break;
    }
    case 10:
        mima->processing_unit.ACC = mima->memory_unit.SIR;
        mima_wasm_register_transfer(mima, ACC, SIR, mima->memory_unit.SIR);
        log_trace("  LDV - %02d: SIR -> ACC \t\t\t 0x%08x -> ACC", mima->processing_unit.MICRO_CYCLE, mima->memory_unit.SIR);
        break;
    case 11:
        log_trace("  LDV - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 12:
        log_trace("  LDV - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        log_info("  LDV - ACC = 0x%08x", mima->processing_unit.ACC);
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
        mima_wasm_register_transfer(mima, SIR, ACC, mima->processing_unit.ACC);
        log_trace("  STV - %02d: ACC -> SIR \t\t\t 0x%08x -> SIR", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.ACC);
        break;
    case 7:
        mima->memory_unit.SAR = mima->control_unit.IR & 0x0FFFFFFF;
        mima_wasm_register_transfer(mima, SAR, IR, mima->memory_unit.SAR);
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
        mima_register value   = mima->memory_unit.SIR;

        // writing to "internal" memory
        if (address < 0xc000000)
        {
            mima->memory_unit.memory[address] = mima->memory_unit.SIR;
            mima_wasm_register_transfer(mima, MEMORY, SIR, mima->memory_unit.SIR);
            log_trace("  STV - %02d: SIR -> mem[IR & 0x0FFFFFFF] \t 0x%08x -> mem[0x%08x] \t I/O Write done", mima->processing_unit.MICRO_CYCLE, mima->memory_unit.SIR, address);
            break;
        }
        else
        {
            mima->control_unit.TRA = mima_true;
            mima_wasm_register_transfer(mima, TRA, IMMEDIATE, mima_true);
            mima_wasm_register_transfer(mima, IOMEMORY, SIR, mima->memory_unit.SIR);
            
            // writing to IO
            if (address == mima_char_output)
            {
                char cha[2];
                cha[0] = (char)value;
                cha[1] = 0;
                mima_wasm_to_output(cha);
                printf("Output (char): %c - %d\n", value, value);
                mima->control_unit.TRA = mima_false;
                mima_wasm_register_transfer(mima, TRA, IMMEDIATE, mima_false);
                break;
            }

            if (address == mima_integer_output)
            {
                char num[32];
                snprintf(num, 32, "(int32_t) %d\n", (int32_t)value);
                mima_wasm_to_output(num);
                printf("Output: %s\n", num);
                mima->control_unit.TRA = mima_false;
                mima_wasm_register_transfer(mima, TRA, IMMEDIATE, mima_false);
                break;
            }

            mima_bool taken = mima_false;
            for (size_t i = 0; i < mima->stv_callbacks_count; ++i)
            {
                if (mima->stv_callbacks[i].address == address)
                {
                    log_info("  STV - %02d: Calling callback function", mima->processing_unit.MICRO_CYCLE);
                    mima->stv_callbacks[i].func(mima, &value);

                    if (mima->control_unit.TRA == mima_true)
                    {
                        log_warn("  STV - %02d: I/O callback functions should always set the TRA flag to mima_false, when finished! Otherwise, the mima stops...", mima->processing_unit.MICRO_CYCLE);
                    }

                    taken = mima_true;
                    break;
                }
            }

            if (!taken)
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
    mima_set_run(mima, mima_false);
    mima_wasm_register_transfer(mima, RUN, IMMEDIATE, mima_false);
    mima_wasm_halt();
    mima->processing_unit.MICRO_CYCLE = 0;
}

void mima_instruction_LDC(mima_t *mima)
{
    switch(mima->processing_unit.MICRO_CYCLE)
    {
    case 6:
        mima->processing_unit.ACC = mima->control_unit.IR & 0x0FFFFFFF;
        mima_wasm_register_transfer(mima, ACC, IR, mima->processing_unit.ACC);
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
        mima_wasm_register_transfer(mima, IAR, IR, mima->control_unit.IAR);
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
            mima_wasm_register_transfer(mima, IAR, IR, mima->control_unit.IAR);
            log_trace("  JMN - %02d: ACC = 0x%08x - Jumping to: 0x%08x", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.ACC, mima->control_unit.IR & 0x0FFFFFFF);
        }
        else
        {
            log_trace("  JMN - %02d: ACC = 0x%08x - No jump taken", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.ACC, mima->control_unit.IR & 0x0FFFFFFF);
        }
        break;
    case 7:
        log_trace("  JMN - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 8:
        log_trace("  JMN - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 9:
        log_trace("  JMN - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 10:
        log_trace("  JMN - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 11:
        log_trace("  JMN - %02d: empty", mima->processing_unit.MICRO_CYCLE);
        break;
    case 12:
        log_trace("  JMN - %02d: empty", mima->processing_unit.MICRO_CYCLE);

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
        mima_wasm_register_transfer(mima, X, ACC, mima->processing_unit.ACC);
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
        mima_wasm_register_transfer(mima, ALU, IMMEDIATE, NOT);
        log_trace("  NOT - %02d: Set ALU to NOT", mima->processing_unit.MICRO_CYCLE);
        break;
    case 11:
        mima->processing_unit.Z = ~mima->processing_unit.X;
        mima_wasm_register_transfer(mima, Z, ALU, ~mima->processing_unit.X);
        log_trace("  NOT - %02d: ~X -> Z \t\t\t ~0x%08x -> Z", mima->processing_unit.MICRO_CYCLE, mima->processing_unit.X);
        break;
    case 12:
        mima->processing_unit.ACC = mima->processing_unit.Z;
        mima_wasm_register_transfer(mima, ACC, Z, mima->processing_unit.Z);
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
        mima_wasm_register_transfer(mima, X, ACC, mima->processing_unit.ACC);
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
        mima_wasm_register_transfer(mima, Y, ONE, 1);
        log_trace("  RAR - %02d: ONE -> Y", mima->processing_unit.MICRO_CYCLE);
        mima->processing_unit.ALU = RAR;
        mima_wasm_register_transfer(mima, ALU, IMMEDIATE, RAR);
        log_trace("  RAR - %02d: Set ALU to RAR", mima->processing_unit.MICRO_CYCLE);
        break;
    case 11:
    {
        int32_t shifted = mima->processing_unit.X >> mima->processing_unit.Y;
        int32_t rotated = mima->processing_unit.X << (32 - mima->processing_unit.Y);
        int32_t combined = shifted | rotated;
        mima->processing_unit.Z = combined;
        mima_wasm_register_transfer(mima, Z, ALU, combined);
        log_trace("  RAR - %02d: (X >>> 1) | (X << 31) -> Z", mima->processing_unit.MICRO_CYCLE);
        break;
    }
    case 12:
        mima->processing_unit.ACC = mima->processing_unit.Z;
        mima_wasm_register_transfer(mima, ACC, Z, mima->processing_unit.Z);
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
        mima_wasm_register_transfer(mima, X, ACC, mima->processing_unit.ACC);
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
        mima_wasm_register_transfer(mima, Y, IR, mima->control_unit.IR & 0x00FFFFFF);
        log_trace("  RRN - %02d: IR & 0x00FFFFFF -> Y \t 0x%08x -> Y", mima->processing_unit.MICRO_CYCLE, mima->control_unit.IR & 0x00FFFFFF);
        mima->processing_unit.ALU = RRN;
        mima->processing_unit.ALU = RRN;
        log_trace("  RRN - %02d: Set ALU to RRN", mima->processing_unit.MICRO_CYCLE);
        break;
    case 11:
    {
        int32_t shifted = mima->processing_unit.X >> mima->processing_unit.Y;
        int32_t rotated = mima->processing_unit.X << (32 - mima->processing_unit.Y);
        int32_t combined = shifted | rotated;
        mima->processing_unit.Z = combined;
        mima_wasm_register_transfer(mima, Z, ALU, combined);
        log_trace("  RRN - %02d: (X >>> Y) | (X << (32-Y)) -> Z", mima->processing_unit.MICRO_CYCLE);
        break;
    }
    case 12:
        mima->processing_unit.ACC = mima->processing_unit.Z;
        mima_wasm_register_transfer(mima, ACC, Z, mima->processing_unit.Z);
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
    if (/*address < 0 || is unsigned*/ address > mima_words - 1)
    {
        log_warn("Invalid address %d\n", address);
        return;
    }

    for (size_t i = 0; address + i < mima_words - 1 && i < count; ++i)
    {
        mima_instruction instruction = mima_instruction_decode_mem(mima->memory_unit.memory[address + i]);
        printf("mem[0x%08lx] = 0x%08x = %s 0x%08x\n", address + i, mima->memory_unit.memory[address + i], mima_get_instruction_name(instruction.op_code), instruction.value);
    }
}

void mima_dump_memory_as_text_at(mima_t* mima, mima_register address, char* text)
{
    if (/*address < 0 || is unsigned*/ address > mima_words - 1)
    {
        log_warn("Invalid address %d\n", address);
        return;
    }

    for (size_t i = 0; address + i < mima_words - 1 && i < 64; ++i)
    {
        mima_instruction instruction = mima_instruction_decode_mem(mima->memory_unit.memory[address + i]);
        snprintf(text, 45, "mem[0x%08lx] 0x%08x : %-3s 0x%08x\n", address + i, mima->memory_unit.memory[address + i], mima_get_instruction_name(instruction.op_code), instruction.value);
        text += 44;
    }
    
    *text = 0;
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
        return "OR ";
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
    return "INV";
}

void mima_delete(mima_t *mima)
{
    free(mima->stv_callbacks);
    free(mima->ldv_callbacks);
    free(mima->memory_unit.memory);
    free(mima->mima_labels);
    free(mima->mima_breakpoints);
}

mima_bool mima_register_IO_LDV_callback(mima_t *mima, uint32_t address, mima_io_callback_fun fun_ptr)
{
    for (size_t i = 0; i < mima->ldv_callbacks_count; ++i)
    {
        if(address == mima->ldv_callbacks[i].address)
        {
            log_warn("Address 0x%08x already registered for ldv callback. Overwriting existing function pointer!", address);
            break;
        }
    }

    if (address < 0x0C000004 || address > 0xFFFFFFFF)
    {
        log_error("Could not register io ldv callback. Address exceeds I/O space.");
        log_error("You can use 0x0C000005 - 0xFFFFFFFF.");
        return mima_true;
    }

    if(mima->ldv_callbacks_count + 1 > mima->ldv_callbacks_capacity)
    {
        mima->ldv_callbacks_capacity *= 2; // double the size
        mima->ldv_callbacks = realloc(mima->ldv_callbacks, sizeof(mima_io_callback) * mima->ldv_callbacks_capacity);

        if (!mima->ldv_callbacks)
        {
            log_error("Could not realloc memory for ldv callbacks.");
            return mima_false;
        }
    }

    mima->ldv_callbacks[mima->ldv_callbacks_count].func = fun_ptr;
    mima->ldv_callbacks[mima->ldv_callbacks_count].address = address;
    log_info("Registered LDV callback for address 0x%08x", address);

    mima->ldv_callbacks_count++;
    return mima_true;
}

mima_bool mima_register_IO_STV_callback(mima_t *mima, uint32_t address, mima_io_callback_fun fun_ptr)
{
    for (size_t i = 0; i < mima->stv_callbacks_count; ++i)
    {
        if(address == mima->stv_callbacks[i].address)
        {
            log_warn("Address 0x%08x already registered for stv callback. Overwriting existing function pointer!", address);
            break;
        }
    }

    if (address < 0x0C000004 || address > 0xFFFFFFFF)
    {
        log_error("Could not register io stv callback. Address exceeds I/O space.");
        log_error("You can use 0x0C000005 - 0xFFFFFFFF.");
        return mima_false;
    }

    if(mima->stv_callbacks_count + 1 > mima->stv_callbacks_capacity)
    {
        mima->stv_callbacks_capacity *= 2; // double the size
        mima->stv_callbacks = realloc(mima->stv_callbacks, sizeof(mima_io_callback) * mima->stv_callbacks_capacity);

        if (!mima->stv_callbacks)
        {
            log_error("Could not realloc memory for stv callbacks.");
            return mima_false;
        }
    }

    mima->stv_callbacks[mima->stv_callbacks_count].func = fun_ptr;
    mima->stv_callbacks[mima->stv_callbacks_count].address = address;
    log_info("Registered STV callback for address 0x%08x", address);

    mima->stv_callbacks_count++;
    return mima_true;
}

mima_bool mima_hit_active_breakpoint(mima_t* mima)
{
    // only stop at instructions, not inbetween
    if (mima->processing_unit.MICRO_CYCLE-1 % 12 != 0)
    {
        return mima_false;    
    }

    for (size_t i = 0; i < breakpoints_count; ++i)
    {
        if(mima->mima_breakpoints[i].address == mima->control_unit.IAR && mima->mima_breakpoints[i].active){
            log_info("Hit breakpoint at 0x%08x", mima->mima_breakpoints[i].address);
            return mima_true;
        }
    }

    return mima_false;
}