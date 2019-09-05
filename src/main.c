#include <stdio.h>
#include <stdlib.h>
#include "mima.h"
#include "log.h"

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

    mima_register_IO_LDV_callback(&mima, 0x0c000005, get_number);
    mima_register_IO_STV_callback(&mima, 0x0c000006, print_number);

    mima_run(&mima, mima_true);

    mima_delete(&mima);

    return 0;
}