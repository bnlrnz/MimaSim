#include <stdio.h>
#include <stdlib.h>
#include "mima.h"
#include "log.h"

char* source_code = 
    "0xFF0 1     // F0 \n"
    "0xFF1 1     // F1 \n"
    "0xFF2 0     // Fn \n"
    "0xFF3 1     // 1 \n"
    "0xFFC 0     // Counter \n"
    "0xFFD 10    // Limit + 2 \n"
    "LDV 0x0c000005  // ask for limit \n"
    "STV 0xFFD       // save limit \n"
    "LDV 0xFF0 \n"
    "STV 0x0c000006 \n"
    "STV 0x0c000006 \n"
    ":LOOP \n"
    "LDV 0xFF0 \n"
    "ADD 0xFF1 \n"
    "STV 0xFF2 \n"
    "STV 0x0c000006  // Output Fn \n"
    "LDV 0xFF1 \n"
    "STV 0xFF0 \n"
    "LDV 0xFF2 \n"
    "STV 0xFF1 \n"
    "LDV 0xFFC \n"
    "ADD 0xFF3 \n"
    "STV 0xFFC \n"
    "EQL 0xFFD \n"
    "NOT \n"
    "JMN LOOP \n"
    "HLT \n";

void get_number(mima_t *mima, mima_register *value)
{
    printf("Type integer:");
    char number_string[32] = {0};
    char *endptr;
    fgets(number_string, 31, stdin);
    *value = strtol(number_string, &endptr, 0);
    mima->control_unit.TRA = mima_false;
}

void print_number(mima_t* mima, mima_register* value)
{
    printf("Fib: %d\n", *value);
    mima->control_unit.TRA = mima_false;
}

int main(int argc, char **argv)
{

    mima_t mima = mima_init();
    log_set_level(LOG_TRACE);

/*
    if(argc < 2)
    {
        printf("Provide mima source code file as parameter... \n");
        return -1;
    }

    const char *fileName = argv[1];


    if (!mima_compile_f(&mima, fileName))
    {
        printf("Failed to compile %s :(\n", fileName);
        return -1;
    }
*/

    if (!mima_compile_s(&mima, source_code))
    {
        printf("Failed to compile:\n%s\n", source_code);
        return -1;
    }

    mima_register_IO_LDV_callback(&mima, 0x0c000005, get_number);
    mima_register_IO_STV_callback(&mima, 0x0c000006, print_number);

    mima_run(&mima, mima_true);

    mima_delete(&mima);

    return 0;
}