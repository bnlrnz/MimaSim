:START 				// Labels start with a ':' and are limited to 31 chars
LDV 0xFF1			// Load value from defined storage (last line)
NOT
STV 0x00
JMP START 			// Jump to label, the compiler will set the address
0xFF1 0x0FFFFFFF	// Define storage by just providing an address and the value