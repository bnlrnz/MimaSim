#include <stdio.h>
#include "mima.h"
#include "log.h"

int main(int argc, char** argv){

	if(argc < 2){
		printf("Provide mima source code file as parameter... \n");
		return -1;
	}

	const char* fileName = argv[1];	

	mima_t mima = mima_init();

	log_set_level(LOG_TRACE);

	if (!mima_compile(&mima, fileName)){
		printf("Failed to compile %s :(\n", fileName);
		return -1;
	}

	mima_run(&mima);

	mima_print_state(&mima);
	mima_print_memory_at(&mima, 0);

	printf("\nDefined memory:\n");
	mima_print_memory_at(&mima, 0xFF0);

	mima_delete(&mima);

	return 0;
}