#ifndef mima_compiler_h
#define mima_compiler_h

#define INITIAL_LABEL_CAPACITY 8

mima_bool mima_compile_file(mima_t*mima, const char* file_name);
mima_bool mima_assemble_instruction(mima_register* instruction, uint32_t op_code, uint32_t value, size_t line);

typedef struct _mima_label{
	char label_name[32];
	uint32_t address;
}mima_label;

extern uint32_t labels_count;
extern uint32_t labels_capacity;
mima_label* mima_labels;

void mima_push_label(const char* label_name, uint32_t address, size_t line);
uint32_t mima_address_for_label(const char* label_name, size_t line);

#endif // mima_compiler_h