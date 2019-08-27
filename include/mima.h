#ifndef mima_h
#define mima_h

#include <stdint.h>
#include <stdarg.h>
#include "log.h"

typedef uint32_t 	mima_register;
typedef uint32_t 	mima_word;

typedef uint8_t 	mima_flag;
typedef uint8_t 	mima_bool;

#define mima_words 	768000000/4

#define mima_true 	1
#define mima_false	0

#define mima_key_input 		0xc000001
#define mima_ascii_output 	0xc000002
#define mima_integer_output	0xc000003

typedef enum _mima_instruction_type
{
    ADD = 0, AND, OR, XOR, LDV, STV, LDC, JMP, JMN, EQL, HLT = 0xF0, NOT, RAR, RRN
} mima_instruction_type;

typedef struct _mima_instruction
{
    mima_instruction_type 	op_code;
    uint32_t 				value;
    mima_bool				extended;
} mima_instruction;

typedef struct _mima_control_unit
{
    mima_register 	IR;
    mima_register 	IAR;
    mima_register 	IP;
    mima_flag 		TRA;
    mima_flag 		RUN;
} mima_control_unit;

typedef struct _mima_memory_unit
{
    mima_register 	SIR;
    mima_register 	SAR;
    mima_word 		*memory;
} mima_memory_unit;

typedef struct _mima_processing_unit
{
    const mima_register 	ONE;
    mima_register 	ACC;
    mima_register 	X;
    mima_register 	Y;
    mima_register 	Z;
    mima_instruction_type ALU;
    uint8_t			MICRO_CYCLE;
    //		ALU;
} mima_processing_unit;

typedef struct _mima_t
{
    mima_control_unit 		control_unit;
    mima_memory_unit 		memory_unit;
    mima_processing_unit 	processing_unit;
    mima_instruction 		current_instruction;
} mima_t;

mima_t mima_init();
void mima_delete(mima_t *mima);

mima_bool mima_compile(mima_t *mima, const char *file_name);

void mima_run(mima_t *mima);
void mima_instruction_step(mima_t *mima);

mima_instruction mima_instruction_decode(mima_t *mima);
mima_bool mima_sar_external(mima_t *mima);

// ADD, AND, OR, XOR, EQL
void mima_instruction_common(mima_t *mima);
void mima_instruction_LDV(mima_t *mima);
void mima_instruction_STV(mima_t *mima);
void mima_instruction_HLT(mima_t *mima);
void mima_instruction_LDC(mima_t *mima);
void mima_instruction_JMP(mima_t *mima);
void mima_instruction_JMN(mima_t *mima);
void mima_instruction_NOT(mima_t *mima);
void mima_instruction_RAR(mima_t *mima);
void mima_instruction_RRN(mima_t *mima);

const char *mima_get_instruction_name(mima_instruction_type instruction);

void mima_print_state(mima_t *mima);
void mima_print_memory_at(mima_t *mima, mima_register address);
void mima_print_memory_unit_state(mima_t *mima);
void mima_print_control_unit_state(mima_t *mima);
void mima_print_processing_unit_state(mima_t *mima);

#endif // mima_h