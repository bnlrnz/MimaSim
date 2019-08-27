#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mima.h"
#include "mima_compiler.h"
#include "log.h"

uint32_t labels_count = 0;
uint32_t labels_capacity = INITIAL_LABEL_CAPACITY;

mima_bool mima_string_to_number(const char *string, uint32_t *number)
{
    *number = -1;

    // check for hex
    if (strncmp(string, "0x", 2) == 0)
    {
        // parse hex
        *number = strtol(string, NULL, 0);
        return mima_true;
    }
    else if (string[0] >= '0' && string[0] <= '9')
    {
        // parse decimal integer
        *number = strtol(string, NULL, 10);
        return mima_true;
    }

    return mima_false;
}

mima_bool mima_string_to_op_code(const char *op_code_string, uint32_t *op_code)
{

    *op_code = -1;

    // thank god, this was done in sublime!
    if (strcmp(op_code_string, "AND") == 0 || strcmp(op_code_string, "and") == 0)
    {
        *op_code = AND;
    }
    else if (strcmp(op_code_string, "ADD") == 0 || strcmp(op_code_string, "add") == 0)
    {
        *op_code = ADD;
    }
    else if (strcmp(op_code_string, "OR ") == 0 || strcmp(op_code_string, "or ") == 0)
    {
        *op_code = OR ;
    }
    else if (strcmp(op_code_string, "XOR") == 0 || strcmp(op_code_string, "xor") == 0)
    {
        *op_code = XOR;
    }
    else if (strcmp(op_code_string, "LDV") == 0 || strcmp(op_code_string, "ldv") == 0)
    {
        *op_code = LDV;
    }
    else if (strcmp(op_code_string, "STV") == 0 || strcmp(op_code_string, "stv") == 0)
    {
        *op_code = STV;
    }
    else if (strcmp(op_code_string, "LDC") == 0 || strcmp(op_code_string, "ldc") == 0)
    {
        *op_code = LDC;
    }
    else if (strcmp(op_code_string, "JMP") == 0 || strcmp(op_code_string, "jmp") == 0)
    {
        *op_code = JMP;
    }
    else if (strcmp(op_code_string, "JMN") == 0 || strcmp(op_code_string, "jmn") == 0)
    {
        *op_code = JMN;
    }
    else if (strcmp(op_code_string, "EQL") == 0 || strcmp(op_code_string, "eql") == 0)
    {
        *op_code = EQL;
    }
    else if (strcmp(op_code_string, "HLT") == 0 || strcmp(op_code_string, "hlt") == 0)
    {
        *op_code = HLT;
    }
    else if (strcmp(op_code_string, "NOT") == 0 || strcmp(op_code_string, "not") == 0)
    {
        *op_code = NOT;
    }
    else if (strcmp(op_code_string, "RAR") == 0 || strcmp(op_code_string, "rar") == 0)
    {
        *op_code = RAR;
    }
    else if (strcmp(op_code_string, "RRN") == 0 || strcmp(op_code_string, "rrn") == 0)
    {
        *op_code = RRN;
    }

    if (*op_code == -1)
    {
        return mima_false;
    }

    return mima_true;
}

mima_bool mima_compile_file(mima_t *mima, const char *file_name)
{
    FILE *file = fopen(file_name, "r");

    if(!file)
    {
        log_error("Failed to open source code file: %s :(", file_name);
        return mima_false;
    }

    char line[256];
    size_t line_number = 0;
    size_t memory_address = 0;
    while(fgets(line, sizeof(line), file))
    {
        line_number++;

        char *string1 = NULL;
        char *string2 = NULL;

        string1 = strtok(line, " \r\n");

        if(string1 == NULL)
        {
            log_warn("Line %zu: Found nothing useful in \n\t\t %s\n", line_number, line);
            continue;
        }

        // classify first string, possible things:
        // op code          like: ADD AND OR ...
        // label            like: :Label1, :START, :loop ...
        // address + value  like: 0xF1, 0xFA8 ... = define storage
        // breakpoint       like: b or B

        // string1 is op code
        // -> string2 will be value or empty (NOT, HLT, RAR)
        uint32_t op_code;
        if (mima_string_to_op_code(string1, &op_code))
        {
            uint32_t value = 0;

            // parse value if available
            if (op_code != NOT && op_code != HLT && op_code != RAR)
            {
                string2 = strtok(NULL, " ");

                if (!mima_string_to_number(string2, &value))
                {
                    // could not parse number string -> is there a label?
                    value = mima_address_for_label(&string2[0]);
                }
            }

            mima_register instruction = 0;

            if (!mima_assemble_instruction(&instruction, op_code, value))
            {
                log_warn("Line %zu: Could not assemble instruction: %s\n", line_number, line);
                continue;
            }

            log_trace("Line %zu: %s 0x%08x -> stored at mem[0x%08x]", line_number, mima_get_instruction_name(op_code), value, memory_address);
            mima->memory_unit.memory[memory_address++] = instruction;
            continue;
        }

        // string1 is a label -> safe for later and remeber line number aka address
        if (string1[0] == ':')
        {
            log_trace("Line %zu: Label %s for address 0x%08x", line_number, &string1[1], memory_address);
            mima_push_label(&string1[1], memory_address);
            continue;
        }

        // define storage: address + hexnumber
        if (mima_string_to_number(string1, &op_code))
        {
            uint32_t value = 0;

            string2 = strtok(NULL, " ");

            if(!mima_string_to_number(string2, &value)){
                log_warn("Found address at line %d - value should follow, but did not :(\n", line_number);
            }

            log_trace("Line %zu: Define mem[0x%08x] = 0x%08x", line_number, op_code, value);

            // op_code holds the address in this case
            mima->memory_unit.memory[op_code] = value;
            continue;
        }

        // TODO: Breakpoints

        log_warn("Line %zu: Ignoring - %s :(", line_number, line);
    }

    return mima_true;
}

void mima_push_label(const char *label_name, uint32_t address)
{
    if(labels_count + 1 > labels_capacity)
    {
        labels_capacity *= 2; // double the size
        mima_labels = realloc(mima_labels, sizeof(mima_label) * labels_capacity);

        if (!mima_labels)
        {
            log_error("Could not realloc memory for labels :(\n");
        }
    }

    if(strlen(label_name) > 31)
    {
        log_error("Label size is limited by 32 chars...\n");
    }

    strncpy(mima_labels[labels_count].label_name,label_name,31);
    mima_labels[labels_count].address = address;

    labels_count++;
}

uint32_t mima_address_for_label(const char *label_name)
{
    for (int i = 0; i < labels_count; ++i)
    {
        if (strcmp(mima_labels[i].label_name, label_name) == 0)
        {
            return mima_labels[i].address;
        }
    }

    log_error("Could not find your label: %s\n", label_name);
    return -1;
}

mima_bool mima_assemble_instruction(mima_register *instruction, uint32_t op_code, uint32_t value)
{
    if(op_code < 0 || op_code > 0xFF)
    {
        log_warn("Invalid op code %d\n", op_code);
        return mima_false;
    }

    // extended instruction, no value added
    if (op_code >= 0xF0)
    {
        *instruction = (op_code << 24) | (value & 0x00FFFFFF);
        return mima_true;
    }

    if(value < 0 || value > 0x0FFFFFFF)
    {
        log_warn("Invalid value %d\n", op_code);
        return mima_false;
    }

    *instruction = (op_code << 28) | (value & 0x0FFFFFFF);
    return mima_true;
}
