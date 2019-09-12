#ifdef WEBASM
#include <emscripten.h>
#endif
#include <string.h>
#include <stdlib.h>

#include "mima.h"
#include "mima_webasm_interface.h"

#define UNUSED(x) (void)(x)

void mima_wasm_send_string(const char *str)
{
    UNUSED(str);
#ifdef WEBASM
    EM_ASM_(
    {
        var ptr = $0;
        var len = $1;
        var message = UTF8ToString(ptr, $1);
        var log = document.getElementById('log_text');

        message = message.replace("TRACE", "<font color=\"blue\">TRACE</font>");
        message = message.replace("DEBUG", "<font color=\"blue\">DEBUG</font>");
        message = message.replace("INFO", "<font color=\"green\">INFO</font>");
        message = message.replace("WARN", "<font color=\"orange\">WARN</font>");
        message = message.replace("ERROR", "<font color=\"red\">ERROR</font>");
        message = message.replace("FATAL", "<font color=\"red\">FATAL</font>");

        log.innerHTML = message + "<br>" + log.innerHTML;
        log.scrollTop = 0;
    }, str, strlen(str));
#endif
}

void mima_wasm_register_transfer(mima_t *mima, mima_register_type target, mima_register_type source, mima_word value)
{
    UNUSED(mima);
    UNUSED(target);
    UNUSED(source);
    UNUSED(value);
#ifdef WEBASM
    EM_ASM_(
    {
        //TODO trigger transfer function source -> target vis
        updateMimaState($1, $2);
    }, source, target, value);
#endif
}

#ifdef WEBASM
EMSCRIPTEN_KEEPALIVE
#endif
void mima_wasm_run(mima_t *mima)
{
    if (mima->current_instruction.op_code == HLT && mima->control_unit.IAR != 0)
    {
        mima_wasm_send_string("Last instruction was HLT and IAR != 0. Are you shure you want to run the mima? -> y|N\n\nThis could result in a lot of ADD instructions if you just executed a mima program\nand the IAR points to the end of your defined memory.\nYou can set the IAR (e.g. to zero) with the 'i' command.\n\nBeware: the first run of your program could have modified the mima state or memory.\nThis could result in an endless loop.\n");
        char res = getchar();
        if (res != 'y')
        {
            return;
        }
    }

    mima->control_unit.RUN = mima_true;

    while(mima->control_unit.RUN && mima->control_unit.TRA == mima_false)
    {
        mima_micro_instruction_step(mima);

        if(mima_hit_active_breakpoint(mima))
        {
            mima->control_unit.RUN = mima_false;
            //mima_shell(mima);
        }
    }
}

#ifdef WEBASM
EMSCRIPTEN_KEEPALIVE
#endif
void mima_wasm_free(void* ptr)
{
    if(ptr)
        free(ptr);
}