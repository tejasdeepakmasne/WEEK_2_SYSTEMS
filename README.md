#CSOC week 2 submission - Tejas Masne

## work done -
### instruction groups implemented -
1. 8-bit load group
2. 16-bit load group
3. 8-bit arithmetic group
4. rotate and shift group
5. general purpose registers
   
### testing 
1. added functions to check values of registers and flags

### supporting fucntions
1. added functions to toggle flags
2. function to calculate two's complement displaced integer
3. function to extract bits from numbers
4. used parityLookupTable to calculate parity values
5. fucntions to calculate current address pointed by HL, IX+d and IY+d

### general description of how the program works 
- the program loads the specified file into memory starting from 0x0000
- the while loop iterates over the the whole memory executing the opcodes loaded which are choosen from a switch-case
- the required opcodes are executed by calling to the specific function and the required changes to the memory and registers are made