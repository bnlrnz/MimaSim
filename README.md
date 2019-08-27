# MimaSim
A Mima Simulator written in C

## Mima Assembler
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
| RRN v    | 0xa    | ACC ← (ACC << (32-v)) | (ACC >>> v)  | Rotate value in Accumulator right by v bits                                               |
| HLT      | 0xf0   | Halt                                 | Stop program execution                                                                    |
| NOT      | 0xf1   | ACC ← ~ACC                           | Negate value in Accumulator                                                               |
| RAR      | 0xf2   | ACC ← (ACC << 31) | (ACC >>> 1)      | Rotate value in Accumulator right by 1 bit                                                |
| RRN v    | 0xf3   | ACC ← (ACC << (32 -v)) | (ACC >>> v) | Rotate value in Accumulator right by v bits    
