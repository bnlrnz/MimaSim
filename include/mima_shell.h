#ifndef mima_shell_h
#define mima_shell_h

#include "mima.h"

void mima_shell_print_help();
void mima_shell_set_IAR(mima_t *mima, char *arg);
void mima_shell_print_memory(mima_t *mima, char *arg);
void mima_shell_set_log_level(char *arg);
int  mima_shell_execute_command(mima_t *mima, char *input);
int  mima_shell(mima_t *mima);

#endif // mima_shell_h