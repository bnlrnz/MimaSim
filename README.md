# MimaSim
A Mima Simulator written in C.
The MiMa (aka Mininal Machine) is a academic, simple and minimalistic microprocessor model/didactic tool to teach basic cpu architecture (Von-Neumann architecture). 

### Build

```bash
$make
```
or

```bash
$mkdir build
$cd build
$cmake ../
$make
```

### Run

```bash
$./MimaSim fibonacci.asm
```

## Mima Shell

While running the mima interactively (**mima_run()**'s second parameter must be **mima_true** to access mima shell), one can type the following commands:

```
s [#].........runs # micro instructions 
s.............equals s 1
S [#].........runs # instructions 
S.............equals S 1 or ends current instruction
m addr [#]....prints # lines of memory at address
m addr........prints 10 lines of memory at address
m.............prints 10 lines of memory at IAR
i [addr]......sets the IAR to address
i.............sets the IAR to zero
r.............runs program till end or breakpoint
p.............prints mima state
L [LOG_LEVEL].sets the log level
L.............prints current and available log level
l.............prints the source file
b addr........sets or toggles breakpoint at address
b.............lists all breakpoints and their state
q.............quits mima
-ENTER-.......repeats last command
```

## Mima Assembler Instructions
| Mnemonic | Opcode | Pseudo code                          | Description                                                                               |
|----------|--------|--------------------------------------|-------------------------------------------------------------------------------------------|
| ADD a    | 0x0    | ACC ← ACC + mem[a]                   | Add value at address a to Accumulator                                                     |
| AND a    | 0x1    | ACC ← ACC & mem[a]                   | Link value at address a to Accumulator by bitwise AND                                     |
| OR a     | 0x2    | ACC ← ACC | mem[a]                   | Link value at address a to Accumulator by bitwise OR                                      |
| XOR a    | 0x3    | ACC ← ACC ^ mem[a]                   | Link value at address a to Accumulator by bitwise XOR                                     |
| LDV a    | 0x4    | ACC ← mem[a]                         | Load value at address a into Accumulator                                                  |
| STV a    | 0x5    | mem[a] ← ACC                         | Store value in Accumulator at address a                                                   |
| LDC v    | 0x6    | ACC ← v                              | Load value v into Accumulator                                                             |
| JMP a    | 0x7    | IAR ← a                              | Continue program execution at address a                                                   |
| JMN a    | 0x8    | IAR ← (ACC < 0) ? a : ++IAR          | Continue program execution at address a if value in Accumulator is negative               |
| EQL a    | 0x9    | ACC ← (ACC == mem[a]) ? -1 : 0       | Set Accumulator to -1 if value in Accumulator is equal to value at address a, 0 otherwise |
| RRN v    | 0xa    | ACC ← (ACC << (32-v)) \| (ACC >>> v)  | Rotate value in Accumulator right by v bits                                               |
| HLT      | 0xf0   | Halt                                 | Stop program execution                                                                    |
| NOT      | 0xf1   | ACC ← ~ACC                           | Negate value in Accumulator                                                               |
| RAR      | 0xf2   | ACC ← (ACC << 31) \| (ACC >>> 1)      | Rotate value in Accumulator right by 1 bit                                                
## Mima Assembler Syntax

##### Mnemoic + Value/Address

- always use mnemonics, op codes are not supported (yet)
- values can be written hex (e.g. 0x1A4) or decimal

```
LDC   41
ADD   0xFF1
STV   0xFF1
0xFF1 0x1
```

##### Labels + Jumps

- labels are case sensitive
- it's not a good idea to jump to a self defined address, unless you know what you are doing

```
:Loop         // this is a label/jump target
LDV   0xAFFE  // some fancy calculation
NOT
STV   0xAFFE
JMP Loop      // will jump to label :Loop
```

##### Breakpoints

- breakpoints are set in source code by typing a upper case 'B' on a separate line or interactively in the mima shell
- when in mima shell, breakpoints can be toggled on/off

```
LDV 0xF1
ADD 0xF2
B
STV 0xF1
```

##### Define storage

- you can define the value at a specific memory address
- the value can be a variable or an instruction

```
LDV   0xFF1
...   
0xFF1 0x1   //puts a one at 0xFF1
```

##### Memory Mapped I/O

The mimas "general purpose" memory is defined from **0x0000 0000 - 0x0C00 0000**.
Addresses above **0x0C00 0000** are used for memory mapped I/O.

There are some defined addresses you can use to output information to the terminal:

- **0x0c000001** single char input
- **0x0c000002** integer input
- **0x0c000003** single char output
- **0x0c000004** integer output

e.g.

```
LDV 0xC000001   // will ask for single char
LDV 0xC000002   // will ask for integer
LDC 42
STV 0xC000003   // will print a '*' to the terminal
LDC 0x40
STV 0xC000004   // will print a 64 to the terminal
```
You can register callback functions for I/O and a specific address.
These callbacks should always set the TRA Bit to **mima_false** when finished.

```C
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
    const char *fileName = argv[1];
    mima_t mima = mima_init();

    mima_compile(&mima, fileName);

    mima_register_IO_LDV_callback(&mima, 0x0c000005, get_number);
    mima_register_IO_STV_callback(&mima, 0x0c000006, print_number);

    mima_run(&mima, mima_true);

    mima_delete(&mima);

    return 0;
}

```

##### Comments

Every lines first "word" that could not be identified as mnemonic, address, nor label, will be ignored.
Therefore you can use something like "//" or "#" for comments in separate lines (or even after valid commands).

e.g.
```
// this will be ignored
ADD 0x01 #you can also do that
STV 0x00 // and this
```















