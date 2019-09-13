#ifdef WEBASM
#include <emscripten.h>
#endif
#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "mima.h"
#include "mima_webasm_interface.h"

#define UNUSED(x) (void)(x)

void mima_wasm_to_log(const char *str)
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

        log.innerHTML = "<p>" + message + "</p>" + log.innerHTML;
        log.scrollTop = 0;
    }, str, strlen(str));
#endif
}

void mima_wasm_to_output(const char* str)
{
    UNUSED(str);
#ifdef WEBASM
    EM_ASM_(
    {
        var ptr = $0;
        var len = $1;
        var message = UTF8ToString(ptr, $1);
        var output = document.getElementById('output_text');
        output.innerHTML = "<p>" + message + "</p>" + output.innerHTML;
        output.scrollTop = 0;
    }, str, strlen(str));
#endif
}

char mima_wasm_input_single_char()
{
#ifdef WEBASM
    return EM_ASM_(
    {
        var char;

        do{
         char = prompt("Input single char: ", "");
        }while(char.length != 1);

        return char;
    });
#endif
    return 0;
}

int mima_wasm_input_single_int()
{
#ifdef WEBASM
   return EM_ASM_(
    {
        var number;
        do{
         number = prompt("Input single number, pls: ", "");
        }while(isNaN(number));

        return number;
    });
#endif
   return 0;
}

void mima_wasm_push_memory_line_correspondence(size_t line_number, mima_word address)
{
    UNUSED(line_number);
    UNUSED(address);
#ifdef WEBASM
    EM_ASM_(
    {
        memoryLineMap.set($1, $0);
    }, line_number, address);
#endif
}

void mima_wasm_mark_error_line(size_t line_number)
{
    UNUSED(line_number);
#ifdef WEBASM
    EM_ASM_(
    {
        document.querySelector('.tln-wrapper :nth-child('+$0+')').setAttribute('style', 'background:#ffaaaa;');
        document.querySelector('.tln-wrapper :nth-child('+$0+')').setAttribute('id', 'compile_error');
    }, line_number);
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
void mima_wasm_set_run(mima_t *mima, mima_bool run)
{
    mima_set_run(mima, run);
}

#ifdef WEBASM
EMSCRIPTEN_KEEPALIVE
#endif
void mima_wasm_hit_breakpoint()
{
#ifdef WEBASM
    EM_ASM_(
    {
        hitBreakpoint();
    });
#endif
}

#ifdef WEBASM
EMSCRIPTEN_KEEPALIVE
#endif
void mima_wasm_free(void* ptr)
{
    if(ptr)
        free(ptr);
}

#ifdef WEBASM
EMSCRIPTEN_KEEPALIVE
#endif
void mima_wasm_set_log_level(int log_level){
    log_set_level(log_level);
}

#ifdef WEBASM
EMSCRIPTEN_KEEPALIVE
#endif
void mima_wasm_halt()
{
#ifdef WEBASM
    EM_ASM_(
    {
        setHalted();
    });
#endif
}