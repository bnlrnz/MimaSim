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
	MICRO_CYCLE
}mima_register_type;

void mima_wasm_run(mima_t* mima);
void mima_wasm_send_string(const char* str);
void mima_wasm_register_transfer(mima_t* mima, mima_register_type source, mima_register_type target, mima_word value);

#endif // mima_webasm_interface_h
