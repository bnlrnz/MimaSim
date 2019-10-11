#ifndef mima_webasm_interface_h
#define mima_webasm_interface_h

typedef enum _mima_register_type{
	IR = 1,
	IAR,
	TRA,
	RUN,
	SIR,
	SAR,
	ACC,
	X,
	Y,
	Z,
	ALU,
	MICRO_CYCLE,
	ONE,
	IMMEDIATE,
	MEMORY,
	IOMEMORY
}mima_register_type;

void mima_wasm_set_run(mima_t *mima, mima_bool run);
void mima_wasm_to_log(const char* str);
void mima_wasm_to_output(const char* str);
void mima_wasm_to_memorydump(mima_t* mima, mima_register address);
char mima_wasm_input_single_char();
int  mima_wasm_input_single_int();
void mima_wasm_hit_breakpoint();
void mima_wasm_push_memory_line_correspondence(size_t line_number, mima_word address);
void mima_wasm_mark_error_line(size_t line_number);
void mima_wasm_halt();
void mima_wasm_set_log_level(int log_level);
void mima_wasm_register_transfer(mima_t* mima, mima_register_type target, mima_register_type source, mima_word value);
void mima_wasm_free(void* ptr);

#endif // mima_webasm_interface_h
