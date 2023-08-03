#include<iostream>
#include<cstdint>
#include<cassert>

// TODO: implement DI, EI, HALT, IM 0, IM 1, IM 2
// TODO: add functions for 8bit LD interrupt vector and memory refresh
// TODO: add stack pointers and stack 
// TODO: implement exchange traansfer and search group
// TODO: implement RLC (IX+d), RLC (IY+d), RL (IX+d), RL (IY+d)
const uint16_t MEMORY_SIZE = 65535;

uint8_t memory[MEMORY_SIZE];
uint8_t stack[MEMORY_SIZE];

uint8_t readMemory(uint16_t address) {
    
    assert(address <= MEMORY_SIZE);
    return memory[address];
}

int twos_comp_displ_int(int n) {
    if (n >= 128) {
        n = n - 256;
    }
    return n;
}

int twos_complement(int n) {
    if(n == 0) {
        return n;
    }
    else {
        return 256-n;
    }
}

// lookup table for uint8_t parity, this can also be used for 16 bits
// by some clever bitshift techniques
static uint8_t parityLookUpTable[ 256 ] = {
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
  1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

void writeMemory(uint16_t address, uint8_t value) {
    assert(address < MEMORY_SIZE);
    memory[address] = value;
}

struct Registers {
    uint16_t pc; //program counter
    uint16_t sp ; // Stack Pointer
    uint8_t a,f; //Accumulator and Flags

    //general purpose registers
    uint8_t b,c,d,e,h,l;

    // Index registers
    uint16_t ix,iy;
};

Registers registers;

uint8_t fetchInstruction() {
    uint8_t instruction = readMemory(registers.pc);
    ++registers.pc;
    return instruction;
}

void loadProgram(const char* filename, uint16_t startAddress) {
    FILE* file = fopen(filename, "rb");
    assert(file != nullptr);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    rewind(file);

    assert(startAddress + fileSize < MEMORY_SIZE);

    fread(&memory[startAddress], 1, fileSize, file);
    fclose(file);
}

/*
z80 allows for usage of bits of register f for usage in certain flags
bit 0: Carry Flag - C
bit 1: Add/Subtract - N
bit 2: Parity/Overflow Flag - P/V
bit 3: not used
bit 4: Half Carry Flag - H
bit 5: not used 
bit 6: Zero Flag - Z
bit 7: Sign Flag - S

*/
// Function to extract k bits from p position
// and returns the extracted value as integer
int bitExtracted(int number, int k, int p)
{
    return (((1 << k) - 1) & (number >> (p)));
}



void set_carry_flag(int i) {
    switch(i) {
        case 1: registers.f |= 1;break;
        case 0: registers.f &= ~1;break;
    }
}

void set_add_sub_flag(int i) {
    switch(i) {
        case 1: registers.f |= (1 << 1);break;
        case 0: registers.f &= ~(1 << 1);break;
    }
}

void set_parity_overflow_flag(int i) {
    switch(i) {
        case 1: registers.f |= (1<<2);break;
        case 0: registers.f &= ~(1<<2);break;
    }
}

void set_half_carry_flag(int i) {
    switch(i) {
        case 1: registers.f |= (1<<4);break;
        case 0: registers.f &= ~(1<<4);break;
    }
}

void set_zero_flag(int i) {
    switch(i) {
        case 1: registers.f |= (1<<6);break;
        case 0: registers.f &= ~(1<<6);break;
    }
}

void set_sign_flag(int i) {
    switch(i) {
        case 1: registers.f |= (1<<7);break;
        case 0: registers.f &= ~(1<<7);break;
    }
}

uint16_t address_of_HL() {
    uint16_t address = (registers.h << 8) | registers.l;
    return address; 
}

uint16_t address_of_IXplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    return address;
}

uint16_t address_of_IYplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    return address;
}

// 8-bit load group instructions

// LD r,r' contents of register r' are loaded in r

//LD a.r'
void LD_a_a() {
    registers.a = registers.a;
}
void LD_a_b() {
    registers.a = registers.b;

}

void LD_a_c() {
    registers.a = registers.c;
}

void LD_a_d() {
    registers.a = registers.d;
}

void LD_a_e() {
    registers.a = registers.e;
}

void LD_a_f() {
    registers.a = registers.f;
}

void LD_a_l() {
    registers.a = registers.l;
}

//LD b,r'
void LD_b_a() {
    registers.b = registers.a;
}
void LD_b_b() {
    registers.b = registers.b;
}

void LD_b_c() {
    registers.b = registers.c;
}

void LD_b_d() {
    registers.b = registers.d;
}

void LD_b_e() {
    registers.b = registers.e;
}

void LD_b_f() {
    registers.b = registers.f;
}

void LD_b_l() {
    registers.b = registers.l;
}

//LD c,r'
void LD_c_a() {
    registers.c = registers.a;
}
void LD_c_b() {
    registers.c = registers.b;
}

void LD_c_c() {
    registers.c = registers.c;
}

void LD_c_d() {
    registers.c = registers.d;
}

void LD_c_e() {
    registers.c = registers.e;
}

void LD_c_f() {
    registers.c = registers.f;
}

void LD_c_l() {
    registers.c = registers.l;
}

//LD d,r'
void LD_d_a() {
    registers.d = registers.a;
}
void LD_d_b() {
    registers.d = registers.b;
}

void LD_d_c() {
    registers.d = registers.c;
}

void LD_d_d() {
    registers.d = registers.d;
}

void LD_d_e() {
    registers.d = registers.e;
}

void LD_d_f() {
    registers.d = registers.f;
}

void LD_d_l() {
    registers.d = registers.l;
}

//LD e,r'
void LD_e_a() {
    registers.e = registers.a;
}
void LD_e_b() {
    registers.e = registers.b;
}

void LD_e_c() {
    registers.e = registers.c;
}

void LD_e_d() {
    registers.e = registers.d;
}

void LD_e_e() {
    registers.e = registers.e;
}

void LD_e_f() {
    registers.e = registers.f;
}

void LD_e_l() {
    registers.e= registers.l;
}

//LD h,r'
void LD_h_a() {
    registers.h = registers.a;
}
void LD_h_b() {
    registers.h = registers.b;
}

void LD_h_c() {
    registers.h = registers.c;
}

void LD_h_d() {
    registers.h = registers.d;
}

void LD_h_e() {
    registers.h = registers.e;
}

void LD_h_f() {
    registers.h = registers.f;
}

void LD_h_l() {
    registers.h = registers.l;
}

//LD l,r'
void LD_l_a() {
    registers.l = registers.a;
}
void LD_l_b() {
    registers.l = registers.b;
}

void LD_l_c() {
    registers.l = registers.c;
}

void LD_l_d() {
    registers.l = registers.d;
}

void LD_l_e() {
    registers.l = registers.e;
}

void LD_l_f() {
    registers.l = registers.f;
}

void LD_l_l() {
    registers.l = registers.l;
}
// LD r,n load 8 bit integer n in register r
void LD_a_n() {
    registers.a = readMemory(registers.pc);
    ++registers.pc;
}

void LD_b_n() {
    registers.b = readMemory(registers.pc);
    ++registers.pc;
}
void LD_c_n() {
    registers.c = readMemory(registers.pc);
    ++registers.pc;
}

void LD_d_n() {
    registers.d = readMemory(registers.pc);
    ++registers.pc;
}

void LD_e_n() {
    registers.e = readMemory(registers.pc);
    ++registers.pc;
}

void LD_h_n() {
    registers.h = readMemory(registers.pc);
    ++registers.pc;
}
void LD_l_n() {
    registers.l = readMemory(registers.pc);
    ++registers.pc;
}


// LD r,(HL) load into register from memory location (HL)
void LD_a_memoryof_hl() {
    uint16_t address = (registers.h << 8) | registers.l;
    registers.a = readMemory(address);
}
void LD_b_memoryof_hl() {
    uint16_t address = (registers.h << 8) | registers.l;
    registers.a = readMemory(address);
}
void LD_c_memoryof_hl() {
    uint16_t address = (registers.h << 8) | registers.l;
    registers.a = readMemory(address);
}
void LD_d_memoryof_hl() {
    uint16_t address = (registers.h << 8) | registers.l;
    registers.a = readMemory(address);
}
void LD_e_memoryof_hl() {
    uint16_t address = (registers.h << 8) | registers.l;
    registers.a = readMemory(address);
}
void LD_h_memoryof_hl() {
    uint16_t address = (registers.h << 8) | registers.l;
    registers.a = readMemory(address);
}
void LD_l_memoryof_hl() {
    uint16_t address = (registers.h << 8) | registers.l;
    registers.a = readMemory(address);
}

// LD r,(IX+d) load into register r from memory location of (IX+d)
// here d is two's complement displaced integer
void LD_a_IXplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    registers.a = readMemory(address);
    ++registers.pc;
}
void LD_b_IXplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    registers.b = readMemory(address);
    ++registers.pc;
}
void LD_c_IXplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    registers.c = readMemory(address);
    ++registers.pc;
}
void LD_d_IXplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    registers.d = readMemory(address);
    ++registers.pc;
}
void LD_e_IXplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    registers.e = readMemory(address);
    ++registers.pc;
}
void LD_h_IXplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    registers.h = readMemory(address);
    ++registers.pc;
}
void LD_l_IXplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    registers.l = readMemory(address);
    ++registers.pc;
}

// LD r,(IY+d) load into register r from memory location of (IX+d)
// here d is two's complement displaced integer

void LD_a_IYplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    registers.a = readMemory(address);
    ++registers.pc;
}
void LD_b_IYplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    registers.b = readMemory(address);
    ++registers.pc;
}
void LD_c_IYplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    registers.c = readMemory(address);
    ++registers.pc;
}
void LD_d_IYplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    registers.d = readMemory(address);
    ++registers.pc;
}
void LD_e_IYplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    registers.e = readMemory(address);
    ++registers.pc;
}
void LD_h_IYplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    registers.h = readMemory(address);
    ++registers.pc;
}
void LD_l_IYplusD() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    registers.l = readMemory(address);
    ++registers.pc;
}

// LD (hl),r load into memory location of hl from register r
void LD_memoryof_hl_a() {
    uint16_t address = (registers.h << 8) | (registers.l);
    writeMemory(address,registers.a);
}

void LD_memoryof_hl_b() {
    uint16_t address = (registers.h << 8) | (registers.l);
    writeMemory(address,registers.b);
}

void LD_memoryof_hl_c() {
    uint16_t address = (registers.h << 8) | (registers.l);
    writeMemory(address,registers.c);
}

void LD_memoryof_hl_d() {
    uint16_t address = (registers.h << 8) | (registers.l);
    writeMemory(address,registers.d);
}

void LD_memoryof_hl_e() {
    uint16_t address = (registers.h << 8) | (registers.l);
    writeMemory(address,registers.e);
}

void LD_memoryof_hl_f() {
    uint16_t address = (registers.h << 8) | (registers.l);
    writeMemory(address,registers.f);
}

void LD_memoryof_hl_l() {
    uint16_t address = (registers.h << 8) | (registers.l);
    writeMemory(address,registers.l);
}

//LD (IX+d),r Load into memory location of (IX+d) from register r
// here d is two's complement displaced integer

void LD_memoryof_IXplusD_a() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    writeMemory(address,registers.a);
    ++registers.pc;
}

void LD_memoryof_IXplusD_b() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    writeMemory(address,registers.b);
    ++registers.pc;
}
void LD_memoryof_IXplusD_c() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    writeMemory(address,registers.c);
    ++registers.pc;
}
void LD_memoryof_IXplusD_d() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    writeMemory(address,registers.d);
    ++registers.pc;
}
void LD_memoryof_IXplusD_e() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    writeMemory(address,registers.e);
    ++registers.pc;
}
void LD_memoryof_IXplusD_f() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    writeMemory(address,registers.f);
    ++registers.pc;
}
void LD_memoryof_IXplusD_l() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix + twos_comp_displ_int(d);
    writeMemory(address,registers.l);
    ++registers.pc;
}

//LD (IY+d),r Load into memory location of (IX+d) from register r
// here d is two's complement displaced integer

void LD_memoryof_IYplusD_a() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    writeMemory(address,registers.a);
    ++registers.pc;
}

void LD_memoryof_IYplusD_b() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    writeMemory(address,registers.b);
    ++registers.pc;
}
void LD_memoryof_IYplusD_c() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    writeMemory(address,registers.c);
    ++registers.pc;
}
void LD_memoryof_IYplusD_d() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    writeMemory(address,registers.d);
    ++registers.pc;
}
void LD_memoryof_IYplusD_e() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy+ twos_comp_displ_int(d);
    writeMemory(address,registers.e);
    ++registers.pc;
}
void LD_memoryof_IYplusD_f() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    writeMemory(address,registers.f);
    ++registers.pc;
}
void LD_memoryof_IYplusD_l() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy + twos_comp_displ_int(d);
    writeMemory(address,registers.l);
    ++registers.pc;
}

//LD (HL),n load into memory location of HL 8bit integer n
void LD_memoryof_hl_n() {
    uint16_t address = (registers.h << 8) | registers.l;
    uint8_t n = readMemory(registers.pc);
    writeMemory(address,n);
    ++registers.pc;
}

//LD (IX+d),n Load into memory location of (IX+d) an 8bit integer n
// here d is two's complement displaced integer
void LD_memoryof_IXplusD_n() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.ix  + twos_comp_displ_int(d);
    uint8_t n = readMemory(registers.pc+1);
    writeMemory(address,n);
    registers.pc = registers.pc +2 ;
}

//LD (IY+d),n Load into memory location of (IY+d) an 8bit integer n
// here d is two's complement displaced integer
void LD_memoryof_IYplusD_n() {
    uint8_t d = readMemory(registers.pc);
    uint16_t address = registers.iy  + twos_comp_displ_int(d);
    uint8_t n = readMemory(registers.pc+1);
    writeMemory(address,n);
    registers.pc = registers.pc +2 ;
}

//LD A,(BC) load into accumulator from memory location (BC)
void LD_a_memoryof_bc() {
    uint16_t address = (registers.b << 8) | (registers.c);
    registers.a = readMemory(address);
}

//LD A,(DE) load into accumulator from memory location (DE)
void LD_a_memoryof_de() {
    uint16_t address = (registers.d << 8) | (registers.e);
    registers.a = readMemory(address);
}

//LD a,(nn) Load into accumulator from memory location given by (nn)
void LD_a_memoryof_nn() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte << 8) | lower_byte;
    registers.a = readMemory(address);
    registers.pc = registers.pc + 2;
}

//LD (BC),A Load into memory location of (BC) from accumulator
void LD_memoryof_bc_a() {
    uint16_t address = (registers.b << 8) | registers.c;
    writeMemory(address,registers.a); 
}

//LD (DE),A Load into memory location of (DE) from accumulator
void LD_memoryof_de_a() {
    uint16_t address = (registers.d << 8) | registers.e;
    writeMemory(address,registers.a); 
}

//LD (nn),A Load into memory location of (nn) from accumulator
void LD_memoryof_nn_a() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte<<8) | lower_byte;
    writeMemory(address,registers.a);
    registers.pc = registers.pc + 2;
}

// 16bit load group

//LD dd,nn Load into register pair dd i.e. BC,DE,HL,SP 16bit integer nn
void LD_BC_16bit_nn() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    registers.b = upper_byte;
    registers.c = lower_byte;
    registers.pc = registers.pc + 2;
}

void LD_DE_16bit_nn() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    registers.d = upper_byte;
    registers.e = lower_byte;
    registers.pc = registers.pc + 2;
}

void LD_HL_16bit_nn() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    registers.h = upper_byte;
    registers.l = lower_byte;
    registers.pc = registers.pc + 2;
}

void LD_SP_16bit_nn() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    registers.sp = (upper_byte << 8) | lower_byte;
    registers.pc = registers.pc + 2;
}

void LD_IX_16bit_nn() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    registers.ix = (upper_byte << 8) | lower_byte;
    registers.pc = registers.pc + 2;
}

void LD_IY_16bit_nn() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    registers.iy = (upper_byte << 8) | lower_byte;
    registers.pc = registers.pc + 2;
}

//LD hl,(nn) load into hl from memory location nn
// h is loaded with nn+1 and l with nn
void LD_hl_memoryof_nn() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte << 8) | lower_byte;
    registers.h = readMemory(address+1);
    registers.l = readMemory(address);
    registers.pc = registers.pc + 2;
}

//LD dd,(nn) load into register pair dd into memory location nn
// pairs:bc,de,sp,ix,iy
void LD_bc_memoryof_nn() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte << 8) | lower_byte;
    registers.b = readMemory(address+1);
    registers.c = readMemory(address);
    registers.pc = registers.pc + 2;
}

void LD_de_memoryof_nn() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte << 8) | lower_byte;
    registers.d = readMemory(address+1);
    registers.e = readMemory(address);
    registers.pc = registers.pc + 2;
}

void LD_sp_memoryof_nn() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte << 8) | lower_byte;
    registers.sp = ((readMemory(address+1)) << 8) | (readMemory(address));
    registers.pc = registers.pc + 2;
}

void LD_ix_memoryof_nn() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte << 8) | lower_byte;
    registers.ix = ((readMemory(address+1)) << 8) | (readMemory(address));
    registers.pc = registers.pc + 2;
}

void LD_iy_memoryof_nn() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte << 8) | lower_byte;
    registers.iy = ((readMemory(address+1)) << 8) | (readMemory(address));
    registers.pc = registers.pc + 2;
}

//LD (nn),HL Load into memory location nn+1 from register h
// Load into memory location nn from register l
void LD_memoryof_nn_hl() {
    uint8_t upper_byte = readMemory(registers.pc + 1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte << 8) | lower_byte;
    writeMemory(address+1,registers.h);
    writeMemory(address,registers.l);
    registers.pc = registers.pc + 2;
}

//LD (nn),dd Load into memory location nn+1 and nn from register pair dd
void LD_memoryof_nn_bc() {
    uint8_t upper_byte = readMemory(registers.pc + 1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte << 8) | lower_byte;
    writeMemory(address+1,registers.b);
    writeMemory(address,registers.c);
    registers.pc = registers.pc + 2;
}

void LD_memoryof_nn_de() {
    uint8_t upper_byte = readMemory(registers.pc + 1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte << 8) | lower_byte;
    writeMemory(address+1,registers.d);
    writeMemory(address,registers.e);
    registers.pc = registers.pc + 2;
}

void LD_memoryof_nn_sp() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte << 8) | lower_byte;
    uint8_t lower_sp = registers.sp << 8;
    uint8_t upper_sp = registers.sp >> 8;
    writeMemory(address+1,upper_sp);
    writeMemory(address,lower_sp);
    registers.pc = registers.pc + 2;
}

void LD_memoryof_nn_ix() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte << 8) | lower_byte;
    uint8_t lower_ix = registers.ix << 8;
    uint8_t upper_ix = registers.ix >> 8;
    writeMemory(address+1,upper_ix);
    writeMemory(address,lower_ix);
    registers.pc = registers.pc + 2;
}

void LD_memoryof_nn_iy() {
    uint8_t upper_byte = readMemory(registers.pc+1);
    uint8_t lower_byte = readMemory(registers.pc);
    uint16_t address = (upper_byte << 8) | lower_byte;
    uint8_t lower_iy = registers.iy << 8;
    uint8_t upper_iy = registers.iy >> 8;
    writeMemory(address+1,upper_iy);
    writeMemory(address,lower_iy);
    registers.pc = registers.pc + 2;
}

// LD SP,HL load into stack pointer from register pair HL
void LD_sp_hl() {
    uint8_t upper_byte = registers.h;
    uint8_t lower_byte = registers.l;
    uint16_t hl = (upper_byte << 8) | lower_byte;
    registers.sp = hl;
}

//LD SP,IX load into stack pointer from register pair IX
void LD_sp_ix() {
    registers.sp = registers.ix;
}

//LD SP,IY load into stack pointer from register pair IX
void LD_sp_iy() {
    registers.sp = registers.iy;
}

// 8-bit Arithmetic group

// ADD a,r contents of register R are added to the accumulator
void ADD_a_a() {
    int A = registers.a;
    int R = registers.a;
    
    // doing the addition
    registers.a = (registers.a + registers.a) % 256;

    //checking for carry
    if (A+R > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

void ADD_a_b() {
    int A = registers.a;
    int R = registers.b;
    
    // doing the addition
    registers.a = (registers.a + registers.b) % 256;

    //checking for carry
    if (A+R > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

void ADD_a_c() {
    int A = registers.a;
    int R = registers.c;
    
    // doing the addition
    registers.a = (registers.a + registers.c) % 256;

    //checking for carry
    if (A+R > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

void ADD_a_d() {
    int A = registers.a;
    int R = registers.d;
    
    // doing the addition
    registers.a = (A+R + registers.d) % 256;

    //checking for carry
    if (registers.a > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

void ADD_a_e() {
    int A = registers.a;
    int R = registers.e;
    
    // doing the addition
    registers.a = (registers.a + registers.e) % 256;

    //checking for carry
    if (A+R > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

void ADD_a_f() {
    int A = registers.a;
    int R = registers.f;
    
    // doing the addition
    registers.a = (registers.a + registers.f) % 256;

    //checking for carry
    if (A+R > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

void ADD_a_l() {
    int A = registers.a;
    int R = registers.l;
    
    // doing the addition
    registers.a = (registers.a + registers.l) % 256;

    //checking for carry
    if (A+R > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

// ADD A,n where A is accumulator and n is 8 bit integer and store result in A
void ADD_a_8bit_n() {
    int A = registers.a;
    int R = readMemory(registers.pc);
    
    // doing the addition
    registers.a = (registers.a + readMemory(registers.pc)) % 256;

    //checking for carry
    if (A+R > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    ++registers.pc;

}

// ADD A,(HL) Add value in A and memory location (HL) and store in A
void ADD_a_memoryOf_HL() {
    int A = registers.a;
    int R = readMemory(address_of_HL());
    
    // doing the addition
    registers.a = (registers.a + readMemory(address_of_HL())) % 256;

    //checking for carry
    if (A+R > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

// ADD A,(IX+d) add A and 8bit int at memory location (IX+d) and store in A
/*
here (IX+d) means the address specified by register pair IX and 2s complement 
displaced d
*/

void ADD_a_memoryOf_IXplusD() {
    int A = registers.a;
    int R = readMemory(address_of_IXplusD());
    
    // doing the addition
    registers.a = (registers.a + readMemory(address_of_IXplusD())) % 256;

    //checking for carry
    if (A+R > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    ++registers.pc;
}

// ADD A,(IY+d) add A and 8bit int at memory location (IY+d) and store in A
/*
here (IY+d) means the address specified by register pair IY and 2s complement 
displaced d
*/

void ADD_a_memoryOf_IYplusD() {
    int A = registers.a;
    int R = readMemory(address_of_IYplusD());
    
    // doing the addition
    registers.a = (registers.a + readMemory(address_of_IYplusD())) % 256;

    //checking for carry
    if (A+R > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    ++registers.pc;
}

/*
ADC A,r - add value in accumulator, the register r and the value of carry flag 
and store it in accumulator
*/
void ADC_a_a() {
    int A = registers.a;
    int R = registers.a;

    //doing addition
    int carry_value = bitExtracted(registers.f,1,0);
    registers.a = (registers.a + registers.a + carry_value)%256;

    //checking for carry
    if (A+R+carry_value > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

void ADC_a_b() {
    int A = registers.a;
    int R = registers.b;

    //doing addition
    int carry_value = bitExtracted(registers.f,1,0);
    registers.a = (registers.a + registers.b + carry_value)%256;

    //checking for carry
    if (A+R+carry_value > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

void ADC_a_c() {
    int A = registers.a;
    int R = registers.c;

    //doing addition
    int carry_value = bitExtracted(registers.f,1,0);
    registers.a = (registers.a + registers.c + carry_value)%256;

    //checking for carry
    if (A+R+carry_value > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

void ADC_a_d() {
    int A = registers.a;
    int R = registers.d;

    //doing addition
    int carry_value = bitExtracted(registers.f,1,0);
    registers.a = (registers.a + registers.d + carry_value)%256;

    //checking for carry
    if (A+R+carry_value > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

void ADC_a_e() {
    int A = registers.a;
    int R = registers.e;

    //doing addition
    int carry_value = bitExtracted(registers.f,1,0);
    registers.a = (registers.a + registers.e + carry_value)%256;

    //checking for carry
    if (A+R+carry_value > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

void ADC_a_f() {
    int A = registers.a;
    int R = registers.f;

    //doing addition
    int carry_value = bitExtracted(registers.f,1,0);
    registers.a = (registers.a + registers.f + carry_value)%256;

    //checking for carry
    if (A+R+carry_value > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

void ADC_a_l() {
    int A = registers.a;
    int R = registers.l;

    //doing addition
    int carry_value = bitExtracted(registers.f,1,0);
    registers.a = (registers.a + registers.l + carry_value)%256;

    //checking for carry
    if (A+R+carry_value > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

// ADC A,n add value at accumulator and 8bit n and store in accumulator
void ADC_a_8bit_n() {
    int A = registers.a;
    int R = readMemory(registers.pc);
    
    // doing the addition
    int carry_value = bitExtracted(registers.f,1,0);
    registers.a = (registers.a + readMemory(registers.pc)+carry_value) % 256;

    //checking for carry
    if (A+R+carry_value > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    ++registers.pc;

}

//ADC A,(HL) add accumulator, memory location HL and carry flag in accumulator 
void ADC_a_memoryOf_hl() {
    int A = registers.a;
    int R = readMemory(address_of_HL());
    
    // doing the addition
    int carry_value = bitExtracted(registers.f,1,0);
    registers.a = (registers.a + readMemory(address_of_HL())+carry_value) % 256;

    //checking for carry
    if (A+R+carry_value > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

}

//ADC A,(IX+d) add into accumlator-accumulator,memory location (IX+d),carry flag
void ADC_a_memoryOf_IXplusD() {
     int A = registers.a;
    int R = readMemory(address_of_IXplusD());
    
    // doing the addition
    int carry_value = bitExtracted(registers.f,1,0);
    registers.a = (registers.a + readMemory(address_of_IXplusD())+carry_value) % 256;

    //checking for carry
    if (A+R+carry_value > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    ++registers.pc;
}

//ADC A,(IY+d) add into accumlator-accumulator,memory location (IY+d),carry flag
void ADC_a_memoryOf_IYplusD() {
     int A = registers.a;
    int R = readMemory(address_of_IYplusD());
    
    // doing the addition
    int carry_value = bitExtracted(registers.f,1,0);
    registers.a = (registers.a + readMemory(address_of_IYplusD())+carry_value) % 256;

    //checking for carry
    if (A+R+carry_value > 255) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    // checking for add/subtract
    set_add_sub_flag(0);

    // checking for parity/overflow
    int P_V = twos_comp_displ_int(A) + twos_comp_displ_int(R);
    if((P_V < -128) || (P_V > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking for half carry
    int H = (A % 16) + (R % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // checking for zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking for sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    ++registers.pc;
}

/*
SUB r the value in register r is subtracted from accumulator and result stored 
in accumulator
*/
void SUB_a() {
    int A = registers.a;
    int R = registers.a;

    //doing the subtraction
    registers.a = (registers.a - registers.a) % 256;

    //checking carry - if difference < 0
    if(A - R < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    //checking add/subtract flag 
    set_add_sub_flag(1);

    //checking parity/overflow - if signed difference < -128 or > 127
    int signed_difference = twos_comp_displ_int(A) - twos_comp_displ_int(R);
    if((signed_difference < -128) || (signed_difference > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking half carry flag : if A%16 - R%16 < 0
    int half_carry_check = (A%16) - (R%16);
    if(half_carry_check < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //checking zero flag : if result is 0
    if(registers.a < 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking sign flag: if result > 127
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void SUB_b() {
    int A = registers.a;
    int R = registers.b;

    //doing the subtraction
    registers.a = (registers.a - registers.b) % 256;

    //checking carry - if difference < 0
    if(A - R < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    //checking add/subtract flag 
    set_add_sub_flag(1);

    //checking parity/overflow - if signed difference < -128 or > 127
    int signed_difference = twos_comp_displ_int(A) - twos_comp_displ_int(R);
    if((signed_difference < -128) || (signed_difference > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking half carry flag : if A%16 - R%16 < 0
    int half_carry_check = (A%16) - (R%16);
    if(half_carry_check < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //checking zero flag : if result is 0
    if(registers.a < 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking sign flag: if result > 127
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void SUB_c() {
    int A = registers.a;
    int R = registers.c;

    //doing the subtraction
    registers.a = (registers.a - registers.c) % 256;

    //checking carry - if difference < 0
    if(A - R < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    //checking add/subtract flag 
    set_add_sub_flag(1);

    //checking parity/overflow - if signed difference < -128 or > 127
    int signed_difference = twos_comp_displ_int(A) - twos_comp_displ_int(R);
    if((signed_difference < -128) || (signed_difference > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking half carry flag : if A%16 - R%16 < 0
    int half_carry_check = (A%16) - (R%16);
    if(half_carry_check < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //checking zero flag : if result is 0
    if(registers.a < 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking sign flag: if result > 127
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void SUB_d() {
    int A = registers.a;
    int R = registers.d;

    //doing the subtraction
    registers.a = (registers.a - registers.d) % 256;

    //checking carry - if difference < 0
    if(A - R < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    //checking add/subtract flag 
    set_add_sub_flag(1);

    //checking parity/overflow - if signed difference < -128 or > 127
    int signed_difference = twos_comp_displ_int(A) - twos_comp_displ_int(R);
    if((signed_difference < -128) || (signed_difference > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking half carry flag : if A%16 - R%16 < 0
    int half_carry_check = (A%16) - (R%16);
    if(half_carry_check < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //checking zero flag : if result is 0
    if(registers.a < 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking sign flag: if result > 127
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void SUB_e() {
    int A = registers.a;
    int R = registers.e;

    //doing the subtraction
    registers.a = (registers.a - registers.e) % 256;

    //checking carry - if difference < 0
    if(A - R < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    //checking add/subtract flag 
    set_add_sub_flag(1);

    //checking parity/overflow - if signed difference < -128 or > 127
    int signed_difference = twos_comp_displ_int(A) - twos_comp_displ_int(R);
    if((signed_difference < -128) || (signed_difference > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking half carry flag : if A%16 - R%16 < 0
    int half_carry_check = (A%16) - (R%16);
    if(half_carry_check < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //checking zero flag : if result is 0
    if(registers.a < 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking sign flag: if result > 127
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void SUB_f() {
    int A = registers.a;
    int R = registers.f;

    //doing the subtraction
    registers.a = (registers.a - registers.f) % 256;

    //checking carry - if difference < 0
    if(A - R < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    //checking add/subtract flag 
    set_add_sub_flag(1);

    //checking parity/overflow - if signed difference < -128 or > 127
    int signed_difference = twos_comp_displ_int(A) - twos_comp_displ_int(R);
    if((signed_difference < -128) || (signed_difference > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking half carry flag : if A%16 - R%16 < 0
    int half_carry_check = (A%16) - (R%16);
    if(half_carry_check < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //checking zero flag : if result is 0
    if(registers.a < 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking sign flag: if result > 127
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void SUB_l() {
    int A = registers.a;
    int R = registers.l;

    //doing the subtraction
    registers.a = (registers.a - registers.l) % 256;

    //checking carry - if difference < 0
    if(A - R < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    //checking add/subtract flag 
    set_add_sub_flag(1);

    //checking parity/overflow - if signed difference < -128 or > 127
    int signed_difference = twos_comp_displ_int(A) - twos_comp_displ_int(R);
    if((signed_difference < -128) || (signed_difference > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking half carry flag : if A%16 - R%16 < 0
    int half_carry_check = (A%16) - (R%16);
    if(half_carry_check < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //checking zero flag : if result is 0
    if(registers.a < 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking sign flag: if result > 127
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}
//SUB n subtract 8bit n from accumulator and store in accumulator
void SUB_8bit_n() {
    int A = registers.a;
    int R = readMemory(registers.pc);

    //doing the subtraction
    registers.a = (registers.a - readMemory(registers.pc)) % 256;

    //checking carry - if difference < 0
    if(A - R < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    //checking add/subtract flag 
    set_add_sub_flag(1);

    //checking parity/overflow - if signed difference < -128 or > 127
    int signed_difference = twos_comp_displ_int(A) - twos_comp_displ_int(R);
    if((signed_difference < -128) || (signed_difference > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking half carry flag : if A%16 - R%16 < 0
    int half_carry_check = (A%16) - (R%16);
    if(half_carry_check < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //checking zero flag : if result is 0
    if(registers.a < 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking sign flag: if result > 127
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    ++registers.pc;
}

//SUB (HL) subtract from accumulator value at (HL) and store in accumulator
void SUB_memoryOf_hl() {
    int A = registers.a;
    int R = readMemory(address_of_HL());

    //doing the subtraction
    registers.a = (registers.a - readMemory(address_of_HL())) % 256;

    //checking carry - if difference < 0
    if(A - R < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    //checking add/subtract flag 
    set_add_sub_flag(1);

    //checking parity/overflow - if signed difference < -128 or > 127
    int signed_difference = twos_comp_displ_int(A) - twos_comp_displ_int(R);
    if((signed_difference < -128) || (signed_difference > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking half carry flag : if A%16 - R%16 < 0
    int half_carry_check = (A%16) - (R%16);
    if(half_carry_check < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //checking zero flag : if result is 0
    if(registers.a < 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking sign flag: if result > 127
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

//SUB (IX+d) subtract from accumulator value at (IX+d), store in accumulator
//d is a twos complement displaced integer
void SUB_IXplusD() {
    int A = registers.a;
    int R = readMemory(address_of_IXplusD());

    //doing the subtraction
    registers.a = (registers.a - readMemory(address_of_IXplusD())) % 256;

    //checking carry - if difference < 0
    if(A - R < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    //checking add/subtract flag 
    set_add_sub_flag(1);

    //checking parity/overflow - if signed difference < -128 or > 127
    int signed_difference = twos_comp_displ_int(A) - twos_comp_displ_int(R);
    if((signed_difference < -128) || (signed_difference > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking half carry flag : if A%16 - R%16 < 0
    int half_carry_check = (A%16) - (R%16);
    if(half_carry_check < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //checking zero flag : if result is 0
    if(registers.a < 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking sign flag: if result > 127
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    ++registers.pc;
}

//SUB (IY+d) subtract from accumulator value at (IY+d), store in accumulator
//d is a twos complement displaced integer
void SUB_IYplusD() {
    int A = registers.a;
    int R = readMemory(address_of_IYplusD());

    //doing the subtraction
    registers.a = (registers.a - readMemory(address_of_IYplusD())) % 256;

    //checking carry - if difference < 0
    if(A - R < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    //checking add/subtract flag 
    set_add_sub_flag(1);

    //checking parity/overflow - if signed difference < -128 or > 127
    int signed_difference = twos_comp_displ_int(A) - twos_comp_displ_int(R);
    if((signed_difference < -128) || (signed_difference > 127)) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //checking half carry flag : if A%16 - R%16 < 0
    int half_carry_check = (A%16) - (R%16);
    if(half_carry_check < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //checking zero flag : if result is 0
    if(registers.a < 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //checking sign flag: if result > 127
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    ++registers.pc;
}

/*
SBC A,s where s is a operand. the operand and carry flag is subtacted from 
the accumulator and the result stored in accumulator
*/
//TODO : implement SBC
void SBC_a_a() {
    int A = registers.a;
    int R = registers.a;

    int carry_value = bitExtracted(registers.f,1,0);
    //doing the subtraction
    int diff = (A - R - carry_value);
    registers.a = diff % 256;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    int half_carry = (A%16) - (R%16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag
    int overflow = twos_comp_displ_int(A) - twos_comp_displ_int(R) - 1;
    if(overflow < -128 || overflow > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //add/sub flag
    set_add_sub_flag(1);

    //carry flag
    if(diff < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }


}

void SBC_a_b() {
    int A = registers.a;
    int R = registers.b;

    int carry_value = bitExtracted(registers.f,1,0);
    //doing the subtraction
    int diff = (A - R - carry_value);
    registers.a = diff % 256;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    int half_carry = (A%16) - (R%16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag
    int overflow = twos_comp_displ_int(A) - twos_comp_displ_int(R) - 1;
    if(overflow < -128 || overflow > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //add/sub flag
    set_add_sub_flag(1);

    //carry flag
    if(diff < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }


}

void SBC_a_c() {
    int A = registers.a;
    int R = registers.c;

    int carry_value = bitExtracted(registers.f,1,0);
    //doing the subtraction
    int diff = (A - R - carry_value);
    registers.a = diff % 256;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    int half_carry = (A%16) - (R%16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag
    int overflow = twos_comp_displ_int(A) - twos_comp_displ_int(R) - 1;
    if(overflow < -128 || overflow > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //add/sub flag
    set_add_sub_flag(1);

    //carry flag
    if(diff < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }


}

void SBC_a_d() {
    int A = registers.a;
    int R = registers.d;

    int carry_value = bitExtracted(registers.f,1,0);
    //doing the subtraction
    int diff = (A - R - carry_value);
    registers.a = diff % 256;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    int half_carry = (A%16) - (R%16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag
    int overflow = twos_comp_displ_int(A) - twos_comp_displ_int(R) - 1;
    if(overflow < -128 || overflow > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //add/sub flag
    set_add_sub_flag(1);

    //carry flag
    if(diff < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }


}

void SBC_a_e() {
    int A = registers.a;
    int R = registers.e;

    int carry_value = bitExtracted(registers.f,1,0);
    //doing the subtraction
    int diff = (A - R - carry_value);
    registers.a = diff % 256;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    int half_carry = (A%16) - (R%16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag
    int overflow = twos_comp_displ_int(A) - twos_comp_displ_int(R) - 1;
    if(overflow < -128 || overflow > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //add/sub flag
    set_add_sub_flag(1);

    //carry flag
    if(diff < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }


}

void SBC_a_h() {
    int A = registers.a;
    int R = registers.h;

    int carry_value = bitExtracted(registers.f,1,0);
    //doing the subtraction
    int diff = (A - R - carry_value);
    registers.a = diff % 256;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    int half_carry = (A%16) - (R%16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag
    int overflow = twos_comp_displ_int(A) - twos_comp_displ_int(R) - 1;
    if(overflow < -128 || overflow > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //add/sub flag
    set_add_sub_flag(1);

    //carry flag
    if(diff < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }


}

void SBC_a_l() {
    int A = registers.a;
    int R = registers.l;

    int carry_value = bitExtracted(registers.f,1,0);
    //doing the subtraction
    int diff = (A - R - carry_value);
    registers.a = diff % 256;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    int half_carry = (A%16) - (R%16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag
    int overflow = twos_comp_displ_int(A) - twos_comp_displ_int(R) - 1;
    if(overflow < -128 || overflow > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //add/sub flag
    set_add_sub_flag(1);

    //carry flag
    if(diff < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }


}

void SBC_a_8bit_n() {
    uint8_t A = registers.a;
    uint8_t R =readMemory(registers.pc); 

    int carry_value = bitExtracted(registers.f,1,0);
    //doing the subtraction
    int diff = (A - R - carry_value);
    registers.a = diff % 256;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    int half_carry = (A%16) - (R%16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag
    int overflow = twos_comp_displ_int(A) - twos_comp_displ_int(R) - 1;
    if(overflow < -128 || overflow > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //add/sub flag
    set_add_sub_flag(1);

    //carry flag
    if(diff < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }

    ++registers.pc;
}


void SBC_a_memoryOf_HL() {
    uint8_t A = registers.a;
    uint8_t R =readMemory(address_of_HL()); 

    int carry_value = bitExtracted(registers.f,1,0);
    //doing the subtraction
    int diff = (A - R - carry_value);
    registers.a = diff % 256;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    int half_carry = (A%16) - (R%16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag
    int overflow = twos_comp_displ_int(A) - twos_comp_displ_int(R) - 1;
    if(overflow < -128 || overflow > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //add/sub flag
    set_add_sub_flag(1);

    //carry flag
    if(diff < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }


}


void SBC_a_memoryOf_IXplusD() {
    uint8_t A = registers.a;
    uint8_t R =readMemory(address_of_IXplusD()); 

    int carry_value = bitExtracted(registers.f,1,0);
    //doing the subtraction
    int diff = (A - R - carry_value);
    registers.a = diff % 256;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    int half_carry = (A%16) - (R%16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag
    int overflow = twos_comp_displ_int(A) - twos_comp_displ_int(R) - 1;
    if(overflow < -128 || overflow > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //add/sub flag
    set_add_sub_flag(1);

    //carry flag
    if(diff < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }


}

void SBC_a_memoryOf_IYplusD() {
    uint8_t A = registers.a;
    uint8_t R =readMemory(address_of_IYplusD()); 

    int carry_value = bitExtracted(registers.f,1,0);
    //doing the subtraction
    int diff = (A - R - carry_value);
    registers.a = diff % 256;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    int half_carry = (A%16) - (R%16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag
    int overflow = twos_comp_displ_int(A) - twos_comp_displ_int(R) - 1;
    if(overflow < -128 || overflow > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //add/sub flag
    set_add_sub_flag(1);

    //carry flag
    if(diff < 0) {
        set_carry_flag(1);
    }
    else {
        set_carry_flag(0);
    }


}
// INC r - register r is incremented by 1
void INC_a() {
    int r = registers.a;
    ++registers.a;

    //add/sub flag
    set_add_sub_flag(0);
    //parity overflow flag
    int signed_result = twos_comp_displ_int(registers.a);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry flag 
    if(r % 16 > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.a == 0){
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
    
}

void INC_b() {
    int r = registers.b;
    ++registers.b;

    //add/sub flag
    set_add_sub_flag(0);
    //parity overflow flag
    int signed_result = twos_comp_displ_int(registers.b);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry flag 
    if(r % 16 > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.b == 0){
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.b > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
    
}

void INC_c() {
    int r = registers.c;
    ++registers.c;

    //add/sub flag
    set_add_sub_flag(0);
    //parity overflow flag
    int signed_result = twos_comp_displ_int(registers.c);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry flag 
    if(r % 16 > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.c == 0){
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.c > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
    
}

void INC_d() {
    int r = registers.d;
    ++registers.d;

    //add/sub flag
    set_add_sub_flag(0);
    //parity overflow flag
    int signed_result = twos_comp_displ_int(registers.d);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry flag 
    if(r % 16 > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.d == 0){
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.d > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
    
}

void INC_e() {
    int r = registers.e;
    ++registers.e;

    //add/sub flag
    set_add_sub_flag(0);
    //parity overflow flag
    int signed_result = twos_comp_displ_int(registers.e);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry flag 
    if(r % 16 > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.e == 0){
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.e > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
    
}

void INC_h() {
    int r = registers.h;
    ++registers.h;

    //add/sub flag
    set_add_sub_flag(0);
    //parity overflow flag
    int signed_result = twos_comp_displ_int(registers.h);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry flag 
    if(r % 16 > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.h == 0){
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.h > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
    
}

void INC_l() {
    int r = registers.l;
    ++registers.l;

    //add/sub flag
    set_add_sub_flag(0);
    //parity overflow flag
    int signed_result = twos_comp_displ_int(registers.l);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry flag 
    if(r % 16 > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.l == 0){
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.l > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
    
}

//increment the value stored at location (HL)
void INC_memoryOf_HL() {
    uint8_t value = readMemory(address_of_HL());
    ++value;
    writeMemory(address_of_HL(),value);

    //add/sub flag
    set_add_sub_flag(0);

    //parity/overflow
    int signed_result = twos_comp_displ_int(value);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    //TODO : does this work????
    //half carry
    if(((value-1) % 16)> 15) {
        set_half_carry_flag(1);
    }   
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(value == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(value > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void INC_memoryOf_IXplusD() {
    uint8_t value = readMemory(address_of_IXplusD());
    ++value;
    writeMemory(address_of_IXplusD(),value);

    //add/sub flag
    set_add_sub_flag(0);

    //parity/overflow
    int signed_result = twos_comp_displ_int(value);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    
    //half carry
    if(((value-1) % 16)> 15) {
        set_half_carry_flag(1);
    }   
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(value == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(value > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
    ++registers.pc;
}

void INC_memoryOf_IYplusD() {
    uint8_t value = readMemory(address_of_IYplusD());
    ++value;
    writeMemory(address_of_IXplusD(),value);

    //add/sub flag
    set_add_sub_flag(0);

    //parity/overflow
    int signed_result = twos_comp_displ_int(value);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    
    //half carry
    if(((value-1) % 16)> 15) {
        set_half_carry_flag(1);
    }   
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(value == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(value > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
    ++registers.pc;
}

//DEC r, value in register r is decremented by 1
void DEC_a() {
    int r = registers.a;
    --registers.a;

    //add/sub flag
    set_add_sub_flag(1);

    //parity/overflow
    int signed_result = twos_comp_displ_int(registers.a);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry
    if(r%16 < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }  
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void DEC_b() {
    int r = registers.b;
    --registers.b;

    //add/sub flag
    set_add_sub_flag(1);

    //parity/overflow
    int signed_result = twos_comp_displ_int(registers.b);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry
    if(r%16 < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.b == 0) {
        set_zero_flag(1);
    }  
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.b > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void DEC_c() {
    int r = registers.c;
    --registers.c;

    //add/sub flag
    set_add_sub_flag(1);

    //parity/overflow
    int signed_result = twos_comp_displ_int(registers.c);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry
    if(r%16 < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.c == 0) {
        set_zero_flag(1);
    }  
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.c > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void DEC_d() {
    int r = registers.d;
    --registers.d;

    //add/sub flag
    set_add_sub_flag(1);

    //parity/overflow
    int signed_result = twos_comp_displ_int(registers.d);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry
    if(r%16 < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.d == 0) {
        set_zero_flag(1);
    }  
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.d > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void DEC_e() {
    int r = registers.e;
    --registers.e;

    //add/sub flag
    set_add_sub_flag(1);

    //parity/overflow
    int signed_result = twos_comp_displ_int(registers.e);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry
    if(r%16 < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.e == 0) {
        set_zero_flag(1);
    }  
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.e > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void DEC_f() {
    int r = registers.f;
    --registers.f;

    //add/sub flag
    set_add_sub_flag(1);

    //parity/overflow
    int signed_result = twos_comp_displ_int(registers.f);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry
    if(r%16 < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.f == 0) {
        set_zero_flag(1);
    }  
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.f > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void DEC_h() {
    int r = registers.h;
    --registers.h;

    //add/sub flag
    set_add_sub_flag(1);

    //parity/overflow
    int signed_result = twos_comp_displ_int(registers.h);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry
    if(r%16 < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.h == 0) {
        set_zero_flag(1);
    }  
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.h > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

void DEC_l() {
    int r = registers.l;
    --registers.l;

    //add/sub flag
    set_add_sub_flag(1);

    //parity/overflow
    int signed_result = twos_comp_displ_int(registers.l);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry
    if(r%16 < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(registers.l == 0) {
        set_zero_flag(1);
    }  
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(registers.l > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

//DEC (HL) decrement the value stored at (HL) by 1
void DEC_memoryOf_HL() {
    uint8_t value = readMemory(address_of_HL());
    int intial_value = readMemory(address_of_HL());
    --value;
    writeMemory(address_of_HL(),value);

    //parity/overflow
    int signed_result = twos_comp_displ_int(value);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry
    if(intial_value %16 < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(value == 0) {
        set_zero_flag(1);
    }  
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(value > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
}

//DEC (IX+d) decrement the value stored at (IX+d) by 1
void DEC_memoryOf_IXplusD() {
    uint8_t value = readMemory(address_of_IXplusD());
    int intial_value = readMemory(address_of_IXplusD());
    --value;
    writeMemory(address_of_IXplusD(),value);

    //parity/overflow
    int signed_result = twos_comp_displ_int(value);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry
    if(intial_value %16 < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(value == 0) {
        set_zero_flag(1);
    }  
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(value > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
    ++registers.pc;
}

//DEC (IX+d) decrement the value stored at (IX+d) by 1
void DEC_memoryOf_IYplusD() {
    uint8_t value = readMemory(address_of_IYplusD());
    int intial_value = readMemory(address_of_IYplusD());
    --value;
    writeMemory(address_of_IYplusD(),value);

    //parity/overflow
    int signed_result = twos_comp_displ_int(value);
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //half carry
    if(intial_value %16 < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //zero flag
    if(value == 0) {
        set_zero_flag(1);
    }  
    else {
        set_zero_flag(0);
    }

    //sign flag
    if(value > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }
    ++registers.pc;
}

/*
AND s where s is any register r, (HL), (IX+d), (IY+d), 8bit number n
AND does a bitwise AND on the accumulator and s
result is stored in accumulator
*/
void AND_a() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.a;

    //doing bitwise and
    registers.a = A_in & opr;

    //sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a ==0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(1);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry flag
    set_carry_flag(0);
}

void AND_b() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.b;

    //doing bitwise and
    registers.a = A_in & opr;

    //sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a ==0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(1);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry flag
    set_carry_flag(0);
}

void AND_c() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.c;

    //doing bitwise and
    registers.a = A_in & opr;

    //sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a ==0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(1);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry flag
    set_carry_flag(0);
}

void AND_d() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.d;

    //doing bitwise and
    registers.a = A_in & opr;

    //sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a ==0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(1);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry flag
    set_carry_flag(0);
}

void AND_e() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.e;

    //doing bitwise and
    registers.a = A_in & opr;

    //sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a ==0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(1);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry flag
    set_carry_flag(0);
}

void AND_h() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.h;

    //doing bitwise and
    registers.a = A_in & opr;

    //sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a ==0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(1);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry flag
    set_carry_flag(0);
}

void AND_l() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.l;

    //doing bitwise and
    registers.a = A_in & opr;

    //sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a ==0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(1);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry flag
    set_carry_flag(0);
}

void AND_memoryOf_HL() {
    uint8_t A_in = registers.a;
    uint8_t opr = readMemory(address_of_HL());

    //doing bitwise and
    registers.a = A_in & opr;

    //sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a ==0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(1);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry flag
    set_carry_flag(0);

}

void AND_memoryOf_IXplusD() {
    uint8_t A_in = registers.a;
    uint8_t opr = readMemory(address_of_IXplusD());

    //doing bitwise and
    registers.a = A_in & opr;

    //sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a ==0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(1);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry flag
    set_carry_flag(0);

}

void AND_memoryOf_IYplusD() {
    uint8_t A_in = registers.a;
    uint8_t opr = readMemory(address_of_IYplusD());

    //doing bitwise and
    registers.a = A_in & opr;

    //sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a ==0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(1);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry flag
    set_carry_flag(0);

}

void AND_8bit_n() {
    uint8_t A_in = registers.a;
    uint8_t opr = readMemory(registers.pc);

    //doing bitwise and
    registers.a = A_in & opr;

    //sign bit
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a ==0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(1);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry flag
    set_carry_flag(0);
}

/*
OR s where s is any register r, 8bit n, (HL), (IX+d), (IY+d)
OR performs the bitwise OR of the values in accumulator and s
the result is stored in accumulator
Behaviour of flags -
S - value of MSB of accumulator
Z - if accumulator == 0
H - 0
P/V - Parity, even = 1, odd = 0
N - 0
C - 0
*/
void OR_a() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.a;

    //performing bitwise OR
    registers.a = A_in | opr;

    //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);
}

void OR_b() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.b;

    //performing bitwise OR
    registers.a = A_in | opr;

    //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);
}

void OR_c() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.c;

    //performing bitwise OR
    registers.a = A_in | opr;

    //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);
}

void OR_d() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.d;

    //performing bitwise OR
    registers.a = A_in | opr;

    //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);
}

void OR_e() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.e;

    //performing bitwise OR
    registers.a = A_in | opr;

    //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);
}

void OR_h() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.h;

    //performing bitwise OR
    registers.a = A_in | opr;

    //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);
}

void OR_l() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.l;

    //performing bitwise OR
    registers.a = A_in | opr;

    //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);
}

void OR_8bit_n() {
    uint8_t A_in = registers.a;
    uint8_t opr = readMemory(registers.pc);

    //performing bitwise OR
    registers.a = A_in | opr;

    //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);

    ++registers.pc;

}

void OR_memoryOf_HL() {
    uint8_t A_in = registers.a;
    uint8_t opr = readMemory(address_of_HL());

    //performing bitwise OR
    registers.a = A_in | opr;

    //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);
}

void OR_memoryOf_IXplusD() {
    uint8_t A_in = registers.a;
    uint8_t opr = readMemory(address_of_IXplusD());

    //performing bitwise OR
    registers.a = A_in | opr;

    //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);

    ++registers.pc;
}

void OR_memoryOf_IYplusD() {
    uint8_t A_in = registers.a;
    uint8_t opr = readMemory(address_of_IYplusD());

    //performing bitwise OR
    registers.a = A_in | opr;

    //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);

    ++registers.pc;
}

/*
XOR s where s is 8bit n, register r, (HL), (IX+d), (IY+d)
XOR performs bitwise XOR on accumulator and s
result is stored in accumulator
behaviour of flags -
S = MSB of accumulator
Z = set if result 0
H = 0
P/V = parity of result
N = 0
C = 0
*/
void XOR_8bit_n() {
    uint8_t A_in = registers.a;
    uint8_t opr = readMemory(registers.pc);

    //perfroming XOR
    registers.a = A_in ^ opr;

     //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);

    ++registers.pc;

}

void XOR_a() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.a;

    //perfroming XOR
    registers.a = A_in ^ opr;

     //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);


}

void XOR_b() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.b;

    //perfroming XOR
    registers.a = A_in ^ opr;

     //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);


}

void XOR_c() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.c;

    //perfroming XOR
    registers.a = A_in ^ opr;

     //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);


}

void XOR_d() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.d;

    //perfroming XOR
    registers.a = A_in ^ opr;

     //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);


}

void XOR_e() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.e;

    //perfroming XOR
    registers.a = A_in ^ opr;

     //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);


}

void XOR_h() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.h;

    //perfroming XOR
    registers.a = A_in ^ opr;

     //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);


}

void XOR_l() {
    uint8_t A_in = registers.a;
    uint8_t opr = registers.l;

    //perfroming XOR
    registers.a = A_in ^ opr;

     //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);


}

void XOR_memoryOf_HL() {
    uint8_t A_in = registers.a;
    uint8_t opr = readMemory(address_of_HL());

    //perfroming XOR
    registers.a = A_in ^ opr;

     //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);

    

}

void XOR_memoryOf_IXplusD() {
    uint8_t A_in = registers.a;
    uint8_t opr = readMemory(address_of_IXplusD());

    //perfroming XOR
    registers.a = A_in ^ opr;

     //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);

    ++registers.pc;

}

void XOR_memoryOf_IYplusD() {
    uint8_t A_in = registers.a;
    uint8_t opr = readMemory(address_of_IYplusD());

    //perfroming XOR
    registers.a = A_in ^ opr;

     //sign flag
    set_sign_flag(bitExtracted(registers.a,1,7));

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity/overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

    //carry
    set_carry_flag(0);

    ++registers.pc;

}

/*
CP s where s is 8bit n, register r, (HL), (IX+d), (IY+d)
CP compares the values of accumulator and s by subtracting s from accumulator
this instruction results in different changes in various flags 
The result on the flags is same as that of subtraction
*/
void CP_8bit_n() {
    uint8_t A = registers.a;
    uint8_t opr = readMemory(registers.pc);

    //comparing by subtraction
    int result = (A - opr) % 256;
    int signed_result = twos_comp_displ_int(A) - twos_comp_displ_int(opr);
    //sign flag
    if(result > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(result == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    } 
    //half carry flag
    int half_carry = (A % 16) - (opr % 16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag 
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    //add/sub flag
    set_add_sub_flag(1);
    //carry flag
    if(result < 0) {
        set_carry_flag(1);
    }
    else{
        set_carry_flag(0);
    }

    ++registers.pc;
}

void CP_a() {
    uint8_t A = registers.a;
    uint8_t opr = registers.a;

    //comparing by subtraction
    int result = (A - opr) % 256;
    int signed_result = twos_comp_displ_int(A) - twos_comp_displ_int(opr);
    //sign flag
    if(result > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(result == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    } 
    //half carry flag
    int half_carry = (A % 16) - (opr % 16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag 
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    //add/sub flag
    set_add_sub_flag(1);
    //carry flag
    if(result < 0) {
        set_carry_flag(1);
    }
    else{
        set_carry_flag(0);
    }
}
void CP_b() {
    uint8_t A = registers.a;
    uint8_t opr = registers.b;

    //comparing by subtraction
    int result = (A - opr) % 256;
    int signed_result = twos_comp_displ_int(A) - twos_comp_displ_int(opr);
    //sign flag
    if(result > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(result == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    } 
    //half carry flag
    int half_carry = (A % 16) - (opr % 16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag 
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    //add/sub flag
    set_add_sub_flag(1);
    //carry flag
    if(result < 0) {
        set_carry_flag(1);
    }
    else{
        set_carry_flag(0);
    }
}

void CP_c() {
    uint8_t A = registers.a;
    uint8_t opr = registers.c;

    //comparing by subtraction
    int result = (A - opr) % 256;
    int signed_result = twos_comp_displ_int(A) - twos_comp_displ_int(opr);
    //sign flag
    if(result > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(result == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    } 
    //half carry flag
    int half_carry = (A % 16) - (opr % 16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag 
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    //add/sub flag
    set_add_sub_flag(1);
    //carry flag
    if(result < 0) {
        set_carry_flag(1);
    }
    else{
        set_carry_flag(0);
    }
}

void CP_d() {
    uint8_t A = registers.a;
    uint8_t opr = registers.d;

    //comparing by subtraction
    int result = (A - opr) % 256;
    int signed_result = twos_comp_displ_int(A) - twos_comp_displ_int(opr);
    //sign flag
    if(result > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(result == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    } 
    //half carry flag
    int half_carry = (A % 16) - (opr % 16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag 
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    //add/sub flag
    set_add_sub_flag(1);
    //carry flag
    if(result < 0) {
        set_carry_flag(1);
    }
    else{
        set_carry_flag(0);
    }

}

void CP_e() {
    uint8_t A = registers.a;
    uint8_t opr = registers.e;

    //comparing by subtraction
    int result = (A - opr) % 256;
    int signed_result = twos_comp_displ_int(A) - twos_comp_displ_int(opr);
    //sign flag
    if(result > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(result == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    } 
    //half carry flag
    int half_carry = (A % 16) - (opr % 16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag 
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    //add/sub flag
    set_add_sub_flag(1);
    //carry flag
    if(result < 0) {
        set_carry_flag(1);
    }
    else{
        set_carry_flag(0);
    }
}

void CP_h() {
    uint8_t A = registers.a;
    uint8_t opr = registers.h;

    //comparing by subtraction
    int result = (A - opr) % 256;
    int signed_result = twos_comp_displ_int(A) - twos_comp_displ_int(opr);
    //sign flag
    if(result > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(result == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    } 
    //half carry flag
    int half_carry = (A % 16) - (opr % 16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag 
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    //add/sub flag
    set_add_sub_flag(1);
    //carry flag
    if(result < 0) {
        set_carry_flag(1);
    }
    else{
        set_carry_flag(0);
    }
}

void CP_l() {
    uint8_t A = registers.a;
    uint8_t opr = registers.l;

    //comparing by subtraction
    int result = (A - opr) % 256;
    int signed_result = twos_comp_displ_int(A) - twos_comp_displ_int(opr);
    //sign flag
    if(result > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(result == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    } 
    //half carry flag
    int half_carry = (A % 16) - (opr % 16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag 
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    //add/sub flag
    set_add_sub_flag(1);
    //carry flag
    if(result < 0) {
        set_carry_flag(1);
    }
    else{
        set_carry_flag(0);
    }
}

void CP_memoryOf_HL() {
    uint8_t A = registers.a;
    uint8_t opr = readMemory(address_of_HL());

    //comparing by subtraction
    int result = (A - opr) % 256;
    int signed_result = twos_comp_displ_int(A) - twos_comp_displ_int(opr);
    //sign flag
    if(result > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(result == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    } 
    //half carry flag
    int half_carry = (A % 16) - (opr % 16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag 
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    //add/sub flag
    set_add_sub_flag(1);
    //carry flag
    if(result < 0) {
        set_carry_flag(1);
    }
    else{
        set_carry_flag(0);
    }

}

void CP_memoryOf_IXplusD() {
    uint8_t A = registers.a;
    uint8_t opr = readMemory(address_of_IXplusD());

    //comparing by subtraction
    int result = (A - opr) % 256;
    int signed_result = twos_comp_displ_int(A) - twos_comp_displ_int(opr);
    //sign flag
    if(result > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(result == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    } 
    //half carry flag
    int half_carry = (A % 16) - (opr % 16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag 
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    //add/sub flag
    set_add_sub_flag(1);
    //carry flag
    if(result < 0) {
        set_carry_flag(1);
    }
    else{
        set_carry_flag(0);
    }

    ++registers.pc;
}


void CP_memoryOf_IYplusD() {
    uint8_t A = registers.a;
    uint8_t opr = readMemory(address_of_IYplusD());

    //comparing by subtraction
    int result = (A - opr) % 256;
    int signed_result = twos_comp_displ_int(A) - twos_comp_displ_int(opr);
    //sign flag
    if(result > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(result == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    } 
    //half carry flag
    int half_carry = (A % 16) - (opr % 16);
    if(half_carry < 0) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    //parity/overflow flag 
    if(signed_result < -128 || signed_result > 127) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }
    //add/sub flag
    set_add_sub_flag(1);
    //carry flag
    if(result < 0) {
        set_carry_flag(1);
    }
    else{
        set_carry_flag(0);
    }

    ++registers.pc;
}

// General Purpose arithmetic and CPU control groups

// NEG - contents of accumulator are negated (two's complement)
void NEG() {
    uint8_t original = registers.a;
    uint result = twos_complement(registers.a);
    registers.a = result;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }
    //TODO : bc kaise implement hoga?? also check in DEC and INC
    //half carry

    //parity/overflow flag
    if(original == 0x80) {
        set_parity_overflow_flag(1);
    }
    else {
        set_parity_overflow_flag(0);
    }

    //add/sub flag
    set_add_sub_flag(1);

    //carry flag
    if(original == 0) {
        set_carry_flag(0);
    }
    else {
        set_carry_flag(1);
    }
}


//CPL - 1's complement of the accumulator is taken
void CPL() {
    uint8_t original = registers.a;
    uint8_t ones_complement = 255 - registers.a;
    registers.a = ones_complement;

    //S,Z,P/V,C not affected
    set_half_carry_flag(1);
    set_add_sub_flag(1);
}

//CCF - complement carry flag 
// half carry is set to the value of previous carry
void CCF() {
    uint8_t carry_bit = bitExtracted(registers.f,1,0);
    set_half_carry_flag(carry_bit);
    carry_bit ^= 1; //toggle using XOR
    set_carry_flag(carry_bit);

    //S,Z,P/V are not affected

    //add/sub flag
    set_add_sub_flag(0);

}

//NOP - no operation
void NOP() {
    //do nothing
}
//ROTATE AND SHIFT GROUP

//RLCA - contents of accumulator are rotated left by one bit and the seventh 
//bit is stored in carry flag and zeroth bit
void RLCA() {
    uint8_t seventh_bit = bitExtracted(registers.a,1,7);
    set_carry_flag(seventh_bit);
    registers.a = (registers.a << 1) + seventh_bit;

    //S,Z,P/V not changed
    set_half_carry_flag(0);
    set_add_sub_flag(0);
}
/*
RLA - The contents of the Accumulator (register A) are rotated
left one bit position through the Carry Flag. The
previous content of the Carry Flag is copied into bit O.
Bit 0 is the least significant bit.
*/
void RLA() {
    uint8_t carry_value = bitExtracted(registers.f,1,0);
    uint8_t seventh_bit = bitExtracted(registers.a,1,7);

    set_carry_flag(seventh_bit);
    registers.a = (registers.a << 1) + carry_value;

    //S,Z,P/V not affected
    set_half_carry_flag(0);
    set_add_sub_flag(0);

}

/*
RRCA - The contents of the Accumulator (register A) are rotated
right one bit position. Bit 0 is copied into the Carry
Flag and also into bit 7. Bit 0 is the least
significant bit.
*/
void RRCA() {
    uint8_t zeroth_bit = bitExtracted(registers.a,1,0);
    registers.a = (registers.a >> 1) + (zeroth_bit << 7);
    set_carry_flag(zeroth_bit);

    //S,Z,P/V not affected
    set_half_carry_flag(0);
    set_add_sub_flag(0);
}

/*
RRA - rotate accumulator one bit to the right storing bit 0 in carry and 
carry flag value in bit 7
*/
void RRA() {
    uint8_t carry_value = bitExtracted(registers.f,1,0);
    set_carry_flag(bitExtracted(registers.a,1,0));
    registers.a = (registers.a >> 1) + (carry_value << 7);
    //H and N reset
    set_half_carry_flag(0);
    set_add_sub_flag(0);
}

/*
RLC r - r is any 8bit register. the contents of R are shifted to left by
one digit and bit 7 is stored in carry flag and bit 0 of register r
*/
void RLC_a(){
    uint8_t seventh_bit = bitExtracted(registers.a,1,7);
    set_carry_flag(seventh_bit);
    registers.a = (registers.a << 1) + seventh_bit;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

}

void RLC_b(){
    uint8_t seventh_bit = bitExtracted(registers.b,1,7);
    set_carry_flag(seventh_bit);
    registers.b = (registers.b << 1) + seventh_bit;

    //sign flag
    if(registers.b > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.b == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.b]);

    //add/sub flag
    set_add_sub_flag(0);

}
void RLC_c(){
    uint8_t seventh_bit = bitExtracted(registers.c,1,7);
    set_carry_flag(seventh_bit);
    registers.c = (registers.c << 1) + seventh_bit;

    //sign flag
    if(registers.c > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.c == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.c]);

    //add/sub flag
    set_add_sub_flag(0);

}
void RLC_d(){
    uint8_t seventh_bit = bitExtracted(registers.d,1,7);
    set_carry_flag(seventh_bit);
    registers.d = (registers.d << 1) + seventh_bit;

    //sign flag
    if(registers.d > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.d == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.d]);

    //add/sub flag
    set_add_sub_flag(0);

}
void RLC_e(){
    uint8_t seventh_bit = bitExtracted(registers.e,1,7);
    set_carry_flag(seventh_bit);
    registers.e = (registers.e << 1) + seventh_bit;

    //sign flag
    if(registers.e > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.e == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.e]);

    //add/sub flag
    set_add_sub_flag(0);

}
void RLC_h(){
    uint8_t seventh_bit = bitExtracted(registers.h,1,7);
    set_carry_flag(seventh_bit);
    registers.h = (registers.h << 1) + seventh_bit;

    //sign flag
    if(registers.h > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.h == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.h]);

    //add/sub flag
    set_add_sub_flag(0);

}
void RLC_l(){
    uint8_t seventh_bit = bitExtracted(registers.l,1,7);
    set_carry_flag(seventh_bit);
    registers.l = (registers.l << 1) + seventh_bit;

    //sign flag
    if(registers.l > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.l == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity overflow flag
    set_parity_overflow_flag(parityLookUpTable[registers.l]);

    //add/sub flag
    set_add_sub_flag(0);

}

/*
RLC (HL) - The contents of the memory address specified by the
contents of register pair HL are rotated left one bit
position. The content of bit 7 is copied into the Carry
Flag and also into bit O. Bit 0 is the least
significant bit.
*/

void RLC_memoryOf_HL() {
    uint8_t value_at_HL = readMemory(address_of_HL());
    uint8_t seventh_bit = bitExtracted(value_at_HL,1,7);
    set_carry_flag(seventh_bit);
    value_at_HL = (value_at_HL << 1) + seventh_bit;
    writeMemory(address_of_HL(),value_at_HL);

    //sign flag
    if(value_at_HL > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(value_at_HL == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry flag
    set_half_carry_flag(0);

    //parity overflow flag
    set_parity_overflow_flag(parityLookUpTable[value_at_HL]);

    //add/sub flag
    set_add_sub_flag(0);



}
/*
RL m - The m operand is any of r,(HL), (IX+d) or (IY+d), as
defined for the analogous RLe instructions. These
various possible opcode-operand combinations are
specified as follows in the assembled object code
*/

void RL_a() {
    uint8_t carry_value = bitExtracted(registers.f,1,0);
    uint8_t seventh_bit = bitExtracted(registers.a,1,7);
    set_carry_flag(seventh_bit);
    registers.a = (registers.a << 1) + carry_value;

    //sign flag
    if(registers.a > 127) {
        set_sign_flag(1);
    }
    else {
        set_sign_flag(0);
    }

    //zero flag
    if(registers.a == 0) {
        set_zero_flag(1);
    }
    else {
        set_zero_flag(0);
    }

    //half carry 
    set_half_carry_flag(0);

    //parity/value
    set_parity_overflow_flag(parityLookUpTable[registers.a]);

    //add/sub flag
    set_add_sub_flag(0);

}

//FUNCTIONS END

// All functions using (IX+d) go here
void functions_using_IXplusD() {
    uint8_t current_opcode = fetchInstruction();
    switch(current_opcode) {

        //LD r,(IX+d)
        case 0x7E: LD_a_IXplusD();break;
        case 0x46: LD_b_IXplusD();break;
        case 0x4E: LD_c_IXplusD();break;
        case 0x56: LD_d_IXplusD();break;
        case 0x5E: LD_e_IXplusD();break;
        case 0x66: LD_h_IXplusD();break;
        case 0x6E: LD_l_IXplusD();break;

        //LD (IX+d),r
        case 0x77: LD_memoryof_IXplusD_a();break;
        case 0x70: LD_memoryof_IXplusD_a();break;
        case 0x71: LD_memoryof_IXplusD_a();break;
        case 0x72: LD_memoryof_IXplusD_a();break;
        case 0x73: LD_memoryof_IXplusD_a();break;
        case 0x74: LD_memoryof_IXplusD_a();break;
        case 0x75: LD_memoryof_IXplusD_a();break;

        //LD (IX+d),n
        case 0x36: LD_memoryof_IXplusD_n();break;

        //16bit load group
        //LD IX,nn
        case 0x21: LD_IX_16bit_nn();break;

        //LD IX,(nn)
        case 0x2A: LD_ix_memoryof_nn();break;

        // LD (nn),IX
        case 0x22: LD_memoryof_nn_ix();break;

        //LD sp,ix
        case 0xF9: LD_sp_ix();break;

        //ADD a, (IX+d)
        case 0x86: ADD_a_memoryOf_IXplusD();break;

        //ADC a, (IX+d)
        case 0x8E: ADC_a_memoryOf_IXplusD();break;

        //SUB (IX+d)
        case 0x96: SUB_IXplusD();break;

        //SBC (IX+d)
        case 0x9F: SBC_a_memoryOf_IYplusD();break;

        //INC (IX+d)
        case 0x34: INC_memoryOf_IXplusD();break;

        //DEC (IX+d)
        case 0x35: DEC_memoryOf_IXplusD();break;

        //AND (IX+d)
        case 0xA6: AND_memoryOf_IXplusD();break;

        //OR (IX+d)
        case 0xB6: OR_memoryOf_IXplusD();break;

        //XOR (IX+d)
        case 0xAE: XOR_memoryOf_IXplusD();break;

        //CP (IX+d)
        case 0xBE: CP_memoryOf_IXplusD();break;
        
    }
}


// All functions using (IY+d) go here
void functions_using_IYplusD() {
    uint8_t current_opcode = fetchInstruction();
    switch(current_opcode) {

        //LD r,(IY+d)
        case 0x7E: LD_a_IYplusD();break;
        case 0x46: LD_b_IYplusD();break;
        case 0x4E: LD_c_IYplusD();break;
        case 0x56: LD_d_IYplusD();break;
        case 0x5E: LD_e_IYplusD();break;
        case 0x66: LD_h_IYplusD();break;
        case 0x6E: LD_l_IYplusD();break;

        //LD (IY+d),r
        case 0x77: LD_memoryof_IYplusD_a();break;
        case 0x70: LD_memoryof_IYplusD_b();break;
        case 0x71: LD_memoryof_IYplusD_c();break;
        case 0x72: LD_memoryof_IYplusD_d();break;
        case 0x73: LD_memoryof_IYplusD_e();break;
        case 0x74: LD_memoryof_IYplusD_f();break;
        case 0x75: LD_memoryof_IYplusD_l();break;

        //LD (IY+d),n
        case 0x36: LD_memoryof_IYplusD_n();break;

        //16 bit load group
        //LD IY,nn
        case 0x21: LD_IY_16bit_nn();break;

        //LD IY,(nn)
        case 0x2A: LD_iy_memoryof_nn();break;

        // LD (nn),IY
        case 0x22: LD_memoryof_nn_iy();break;

        //LD sp,iy
        case 0xF9: LD_sp_iy();break;

        //ADD a, (IY+d)
        case 0x86: ADD_a_memoryOf_IYplusD();break;

        //ADC a, (IX+d)
        case 0x8E: ADC_a_memoryOf_IYplusD();break;

        //SUB (IY+d)
        case 0x96: SUB_IYplusD();break;

        case 0x9E: SBC_a_memoryOf_IYplusD();break;

        //INC (IY+d)
        case 0x34: INC_memoryOf_IYplusD();break;

        //DEC (IY+d)
        case 0x35: DEC_memoryOf_IYplusD();break;

        //AND (IY+d)
        case 0xA6: AND_memoryOf_IYplusD();break;

        //OR (IY+d)
        case 0xB6: OR_memoryOf_IYplusD();break;

        //XOR (IY+d)
        case 0xAE: XOR_memoryOf_IYplusD();break;

        //CP (IY+d)
        case 0xBE: CP_memoryOf_IYplusD();break;
    }
}

void functions_using_ED_opcode() {
    uint8_t current_opcode = fetchInstruction();
    switch(current_opcode) {
        //LD BC,(nn)
        case 0x4B: LD_bc_memoryof_nn();break;
        
        //LD DE,(nn)
        case 0x5B: LD_de_memoryof_nn();break;

        //LD SP,(nn)
        case 0x7B: LD_sp_memoryof_nn();break;

        //LD (nn),bc
        case 0x43: LD_memoryof_nn_bc();break;

        //LD (nn),de
        case 0x53: LD_memoryof_nn_de();break;

        //LD (nn),sp
        case 0x73: LD_memoryof_nn_sp();break;

        //NEG
        case 0x44: NEG();
    }
} 

void functions_using_CB() {

}

void decodeInstruction(uint8_t opcode) {
    switch(opcode) {
        // LD a,r'
        case 0x7F: LD_a_a();break;
        case 0x78: LD_a_b();break;
        case 0x79: LD_a_c();break;
        case 0x7A: LD_a_d();break;
        case 0x7B: LD_a_e();break;
        case 0x7C: LD_a_f();break;
        case 0x7D: LD_a_l();break;

        //LD b,r'
        case 0x47: LD_b_a();break;
        case 0x40: LD_b_b();break;
        case 0x41: LD_b_c();break;
        case 0x42: LD_b_d();break;
        case 0x43: LD_b_e();break;
        case 0x44: LD_b_f();break;
        case 0x45: LD_b_l();break;

        //LD c,r'
        case 0x4F: LD_c_a();break;
        case 0x48: LD_c_b();break;
        case 0x49: LD_c_c();break;
        case 0x4A: LD_c_d();break;
        case 0x4B: LD_c_e();break;
        case 0x4C: LD_c_f();break;
        case 0x4D: LD_c_l();break;

        //LD d,r'
        case 0x57: LD_d_a();break;
        case 0x50: LD_d_b();break;
        case 0x51: LD_d_c();break;
        case 0x52: LD_d_d();break;
        case 0x53: LD_d_e();break;
        case 0x54: LD_d_f();break;
        case 0x55: LD_d_l();break;

        //LD e,r'
        case 0x5F: LD_e_a();break;
        case 0x58: LD_e_b();break;
        case 0x59: LD_e_c();break;
        case 0x5A: LD_e_d();break;
        case 0x5B: LD_e_e();break;
        case 0x5C: LD_e_f();break;
        case 0x5D: LD_e_l();break;

        //LD h,r'
        case 0x67: LD_h_a();break;
        case 0x60: LD_h_b();break;
        case 0x61: LD_h_c();break;
        case 0x62: LD_h_d();break;
        case 0x63: LD_h_e();break;
        case 0x64: LD_h_f();break;
        case 0x65: LD_h_l();break;

        //LD l,r'
        case 0x6F: LD_l_a();break;
        case 0x68: LD_l_b();break;
        case 0x69: LD_l_c();break;
        case 0x6A: LD_l_d();break;
        case 0x6B: LD_l_e();break;
        case 0x6C: LD_l_f();break;
        case 0x6D: LD_l_l();break;

        //LD r,n
        case 0x3E: LD_a_n();break;
        case 0x06: LD_b_n();break;
        case 0x0E: LD_c_n();break;
        case 0x16: LD_d_n();break;
        case 0x1E: LD_e_n();break;
        case 0x26: LD_h_n();break;
        case 0x2E: LD_l_n();break;

        // LD r,(HL)
        case 0x7E: LD_a_memoryof_hl();break;
        case 0x46: LD_b_memoryof_hl();break;
        case 0x4E: LD_c_memoryof_hl();break;
        case 0x56: LD_d_memoryof_hl();break;
        case 0x5E: LD_e_memoryof_hl();break;
        case 0x66: LD_h_memoryof_hl();break;
        case 0x6E: LD_l_memoryof_hl();break;

        // execute the functions associated with (IX+d)
        case 0xDD: functions_using_IXplusD();break;
        // execute the functions associated with (IY+d)
        case 0xFD: functions_using_IYplusD();break;

        // LD (HL),r
        case 0x77: LD_memoryof_hl_a();break;
        case 0x70: LD_memoryof_hl_b();break;
        case 0x71: LD_memoryof_hl_c();break;
        case 0x72: LD_memoryof_hl_d();break;
        case 0x73: LD_memoryof_hl_e();break;
        case 0x74: LD_memoryof_hl_f();break;
        case 0x75: LD_memoryof_hl_l();break;

        // LD (HL),n
        case 0x36: LD_memoryof_hl_n();break;

        //LD A,(BC)
        case 0x0A: LD_a_memoryof_bc();break;

        //LD A,(DE)
        case 0x1A: LD_a_memoryof_de();break;

        //LD A,(nn)
        case 0x3A: LD_a_memoryof_nn();break;

        //LD (BC),A
        case 0x02: LD_memoryof_bc_a();break;

        //LD (DE),A
        case 0x12: LD_memoryof_de_a();break;

        //LD (nn),A
        case 0x32: LD_memoryof_nn_a();break;

        // 16bit load group
        //LD BC,nn
        case 0x01: LD_BC_16bit_nn();break;

        //LD DE,nn
        case 0x11: LD_DE_16bit_nn();break;

        //LD HL.nn
        case 0x21: LD_HL_16bit_nn();break;

        //LD SP,nn
        case 0x31: LD_SP_16bit_nn();break;

        //LD HL,(nn)
        case 0x2A: LD_hl_memoryof_nn();break;

        // execute the functions using the ED opcode
        case 0xED: functions_using_ED_opcode();break;

        //LD (nn),HL
        case 0x22: LD_memoryof_nn_hl();break;

        //LD SP,HL
        case 0xF9: LD_sp_hl();break;

        //8bit arithmetic and logic

        //ADD A,A
        case 0x87: ADD_a_a();break;

        //ADD A,B
        case 0x80: ADD_a_b();break;

        //ADD A,C
        case 0x81: ADD_a_c();break;

        //ADD A,D
        case 0x82: ADD_a_d();break;

        //ADD A,E
        case 0x83: ADD_a_e();break;

        //ADD A,F
        case 0x84: ADD_a_f();break;

        //ADD A,L
        case 0x85: ADD_a_l();break;

        //ADD A,n
        case 0xC6: ADD_a_8bit_n();break;

        //ADD A,(HL)
        case 0x86: ADD_a_memoryOf_HL();break;

        //ADC A,A
        case 0x8F: ADC_a_a();break;

        //ADC A,B
        case 0x88: ADC_a_b();break;

        // ADC A,C
        case 0x89: ADC_a_c();break;

        // ADC A,D
        case 0x8A: ADC_a_d();break;

        //ADC A,E
        case 0x8B: ADC_a_e();break;

        //ADC A,F
        case 0x8C: ADC_a_f();break;

        //ADC A,L
        case 0x8D: ADC_a_l();break;

        //ADC A,n
        case 0xCE: ADC_a_8bit_n();break;

        //ADC A,(HL)
        case 0x8E: ADC_a_memoryOf_hl();break;

        //SUB A
        case 0x97: SUB_a();break;

        //SUB B
        case 0x90: SUB_b();break;

        //SUB C
        case 0x91: SUB_c();break;

        //SUB D
        case 0x92: SUB_c();break;

        //SUB E
        case 0x93: SUB_e();break;

        //SUB F
        case 0x94: SUB_f();break;

        //SUB L
        case 0x95: SUB_l();break;

        //SUB (HL)
        case 0x96: SUB_memoryOf_hl();break;

        //SUB n
        case 0xD6: SUB_8bit_n();break;

        //SBC m where m is 8bit n, register r, (HL)
        case 0xDE: SBC_a_8bit_n();break;
        case 0x9F: SBC_a_a();break;
        case 0x98: SBC_a_b();break;
        case 0x99: SBC_a_c();break;
        case 0x9A: SBC_a_d();break;
        case 0x9B: SBC_a_e();break;
        case 0x9C: SBC_a_h();break;
        case 0x9D: SBC_a_l();break;
        case 0x9E: SBC_a_memoryOf_HL();break;

        //INC a
        case 0x3C: INC_a();break;

        //INC b
        case 0x04: INC_b();break;

        //INC c
        case 0x0C: INC_c();break;

        //INC d
        case 0x14: INC_d();break;

        //INC e
        case 0x1C: INC_e();break;

        //TODO : INC f?? wtf??

        //INC h
        case 0x24: INC_h();break;

        //INC l
        case 0x2C: INC_l();break;

        //INC (HL)
        case 0x34: INC_memoryOf_HL();break;

        //DEC a
        case 0x3D: DEC_a();break;

        //DEC b
        case 0x05: DEC_b();break;

        //DEC c
        case 0x0D: DEC_c();break;

        //DEC d
        case 0x15: DEC_d();break;

        //DEC e
        case 0x1D: DEC_e();break;

        //DEC h
        case 0x25: DEC_h();break;

        //DEC l
        case 0x2D: DEC_l();break;

        //DEC (HL)
        case 0x35: DEC_memoryOf_HL();break;

        //AND a
        case 0xA7: AND_a();break;

        //AND b
        case 0xA0: AND_b();break;

        //AND c
        case 0xA1: AND_c();break;

        //AND d
        case 0xA2: AND_d();break;

        //AND e
        case 0xA3: AND_e();break;

        //AND h
        case 0xA4: AND_h();break;

        //AND l
        case 0xA5: AND_l();break;

        //AND (HL)
        case 0xA6: AND_memoryOf_HL();break;

        //AND n
        case 0xE6: AND_8bit_n();break;

        //OR m where m is register r, (HL), 8bit n
        case 0xF6: OR_8bit_n();break;
        case 0xB7: OR_a();break;
        case 0xB0: OR_b();break;
        case 0xB1: OR_c();break;
        case 0xB2: OR_d();break;
        case 0xB3: OR_e();break;
        case 0xB4: OR_h();break;
        case 0xB5: OR_l();break;
        case 0xB6: OR_memoryOf_HL();break;

        //XOR m where m is register r, (HL), 8bit n
        case 0xEE: XOR_8bit_n();break;
        case 0xAF: XOR_a();break;
        case 0xA8: XOR_b();break;
        case 0xA9: XOR_c();break;
        case 0xAA: XOR_d();break;
        case 0xAB: XOR_e();break;
        case 0xAC: XOR_h();break;
        case 0xAD: XOR_l();break;
        case 0XAE: XOR_memoryOf_HL();break;
        
        //CP m where m is register r, (HL), 8bit n
        case 0xFE: CP_8bit_n();break;
        case 0xBF: CP_a();break;
        case 0xB8: CP_b();break;
        case 0xB9: CP_c();break;
        case 0xBA: CP_d();break;
        case 0xBB: CP_e();break;
        case 0xBC: CP_h();break;
        case 0xBD: CP_l();break;
        case 0xBE: CP_memoryOf_HL();break;
        
        //CPL
        case 0x2F: CPL();break;

        //CCF
        case 0x3F: CCF();break;

        //NOP
        case 0x00: NOP();break;

        //SCF - set carry flag
        case 0x37: set_carry_flag(1);
    }
}

// functions to check values of different registers and flags
// fancy print statements nothing else
/*
pass 0 for unsigned value
pass 1 for signed value 
pass 2 for both
*/
void register_a_val(int choice) {
    switch(choice) {
        case 0: 
            std::cout << "register A unsigned :" << static_cast<int>(registers.a) << std::endl;
            break;
        case 1:
            std::cout << "register A signed :" << static_cast<int>(twos_comp_displ_int(registers.a)) << std::endl;
            break;
        case 2:
            std::cout << "register A unsigned :" << static_cast<int>(registers.a) << std::endl;
            std::cout << "register A signed :" << static_cast<int>(twos_comp_displ_int(registers.a)) << std::endl;
            break;

    }
} 
void register_b_val(int choice) {
    switch(choice) {
        case 0: 
            std::cout << "register B unsigned :" << static_cast<int>(registers.b) << std::endl;
            break;
        case 1:
            std::cout << "register B signed :" << static_cast<int>(twos_comp_displ_int(registers.b)) << std::endl;
            break;
        case 2:
            std::cout << "register B unsigned :" << static_cast<int>(registers.b) << std::endl;
            std::cout << "register B signed :" << static_cast<int>(twos_comp_displ_int(registers.b)) << std::endl;
            break;

    }
} 

void register_c_val(int choice) {
    switch(choice) {
        case 0: 
            std::cout << "register C unsigned :" << static_cast<int>(registers.c) << std::endl;
            break;
        case 1:
            std::cout << "register C signed :" << static_cast<int>(twos_comp_displ_int(registers.c)) << std::endl;
            break;
        case 2:
            std::cout << "register C unsigned :" << static_cast<int>(registers.c) << std::endl;
            std::cout << "register C signed :" << static_cast<int>(twos_comp_displ_int(registers.c)) << std::endl;
            break;

    }
} 

void register_d_val(int choice) {
    switch(choice) {
        case 0: 
            std::cout << "register D unsigned :" << static_cast<int>(registers.d) << std::endl;
            break;
        case 1:
            std::cout << "register D signed :" << static_cast<int>(twos_comp_displ_int(registers.d)) << std::endl;
            break;
        case 2:
            std::cout << "register D unsigned :" << static_cast<int>(registers.d) << std::endl;
            std::cout << "register D signed :" << static_cast<int>(twos_comp_displ_int(registers.d)) << std::endl;
            break;

    }
} 

void register_e_val(int choice) {
    switch(choice) {
        case 0: 
            std::cout << "register E unsigned :" << static_cast<int>(registers.e) << std::endl;
            break;
        case 1:
            std::cout << "register E signed :" << static_cast<int>(twos_comp_displ_int(registers.e)) << std::endl;
            break;
        case 2:
            std::cout << "register E unsigned :" << static_cast<int>(registers.e) << std::endl;
            std::cout << "register E signed :" << static_cast<int>(twos_comp_displ_int(registers.e)) << std::endl;
            break;

    }
} 

void register_f_val(int choice) {
    switch(choice) {
        case 0: 
            std::cout << "register F unsigned :" << static_cast<int>(registers.f) << std::endl;
            break;
        case 1:
            std::cout << "register F signed :" << static_cast<int>(twos_comp_displ_int(registers.f)) << std::endl;
            break;
        case 2:
            std::cout << "register F unsigned :" << static_cast<int>(registers.f) << std::endl;
            std::cout << "register F signed :" << static_cast<int>(twos_comp_displ_int(registers.f)) << std::endl;
            break;

    }
} 

void register_h_val(int choice) {
    switch(choice) {
        case 0: 
            std::cout << "register H unsigned :" << static_cast<int>(registers.h) << std::endl;
            break;
        case 1:
            std::cout << "register H signed :" << static_cast<int>(twos_comp_displ_int(registers.h)) << std::endl;
            break;
        case 2:
            std::cout << "register H unsigned :" << static_cast<int>(registers.h) << std::endl;
            std::cout << "register H signed :" << static_cast<int>(twos_comp_displ_int(registers.h)) << std::endl;
            break;

    }
} 

void register_l_val(int choice) {
    switch(choice) {
        case 0: 
            std::cout << "register A unsigned :" << static_cast<int>(registers.l) << std::endl;
            break;
        case 1:
            std::cout << "register A signed :" << static_cast<int>(twos_comp_displ_int(registers.l)) << std::endl;
            break;
        case 2:
            std::cout << "register A unsigned :" << static_cast<int>(registers.l) << std::endl;
            std::cout << "register A signed :" << static_cast<int>(twos_comp_displ_int(registers.l)) << std::endl;
            break;

    }
} 

void program_counter_position() {
    std::cout << "program counter :"<< static_cast<int>(registers.pc) << std::endl;
}

//status of various flags from register f
void check_carry_flag() {
    int value = bitExtracted(registers.f,1,0);
    std::cout << "carry flag :"<<static_cast<int>(value) << std::endl;
}

void check_add_sub_flag() {
    int value = bitExtracted(registers.f,1,1);
    std::cout << "add/subtract flag :"<<static_cast<int>(value) << std::endl;
}

void check_parity_overflow_flag() {
    int value = bitExtracted(registers.f,1,2);
    std::cout << "parity/overflow flag :"<<static_cast<int>(value) << std::endl;
}

void check_half_carry_flag() {
    int value = bitExtracted(registers.f,1,4);
    std::cout << "half carry flag:"<<static_cast<int>(value) << std::endl;
}

void check_zero_flag() {
    int value = bitExtracted(registers.f,1,6);
    std::cout << "zero flag :"<<static_cast<int>(value) << std::endl;
}

void check_sign_flag() {
    int value = bitExtracted(registers.f,1,7);
    std::cout << "sign flag :"<<static_cast<int>(value) << std::endl;
}
/*
case 0 : all registers unsigned
case 1 : all registers signed
case 2 : all registers unsigned and signed
case 3 : all flags
case 4 : all flags and registers unsigned
case 5 : all flags and registers signed
case 6 : all flags and registers both signed and unsigned
*/
void check_all_registers_flags(int choice) {
    switch(choice) {
        case 0: //all registers unsigned
            register_a_val(0);
            register_b_val(0);
            register_c_val(0);
            register_d_val(0);
            register_e_val(0);
            register_f_val(0);
            register_h_val(0);
            register_l_val(0);
            std::cout << "----------------" << std::endl;
            break;
        case 1: //all registers signed
            register_a_val(1);
            register_b_val(1);
            register_c_val(1);
            register_d_val(1);
            register_e_val(1);
            register_f_val(1);
            register_h_val(1);
            register_l_val(1);
            std::cout << "----------------" << std::endl;
            break;
        case 2: //all registers both unsigned and signed
            register_a_val(2);
            register_b_val(2);
            register_c_val(2);
            register_d_val(2);
            register_e_val(2);
            register_f_val(2);
            register_h_val(2);
            register_l_val(2);
            std::cout << "----------------" << std::endl;
            break;
        case 3: //all flags
            check_carry_flag();
            check_add_sub_flag();
            check_parity_overflow_flag();
            check_half_carry_flag();
            check_zero_flag();
            check_sign_flag();
            std::cout << "----------------" << std::endl;
            break;
        case 4: //all flags + registers unsigned
            check_carry_flag();
            check_add_sub_flag();
            check_parity_overflow_flag();
            check_half_carry_flag();
            check_zero_flag();
            check_sign_flag();
            register_a_val(0);
            register_b_val(0);
            register_c_val(0);
            register_d_val(0);
            register_e_val(0);
            register_f_val(0);
            register_h_val(0);
            register_l_val(0);
            std::cout << "----------------" << std::endl;
            break;
        case 5: //all flags + registers signed
            check_carry_flag();
            check_add_sub_flag();
            check_parity_overflow_flag();
            check_half_carry_flag();
            check_zero_flag();
            check_sign_flag();
            register_a_val(1);
            register_b_val(1);
            register_c_val(1);
            register_d_val(1);
            register_e_val(1);
            register_f_val(1);
            register_h_val(1);
            register_l_val(1);
            std::cout << "----------------" << std::endl;
            break;
        case 6: //all flags + registers both unsigned and signed
            check_carry_flag();
            check_add_sub_flag();
            check_parity_overflow_flag();
            check_half_carry_flag();
            check_zero_flag();
            check_sign_flag();
            register_a_val(2);
            register_b_val(2);
            register_c_val(2);
            register_d_val(2);
            register_e_val(2);
            register_f_val(2);
            register_h_val(2);
            register_l_val(2);
            std::cout << "----------------" << std::endl;
            break;

    }
}
int main(int argc, char *argv[]) {
    int count = 1;
    loadProgram(argv[1],0x0000);
    while(registers.pc < 65535) {
        
        uint8_t opcode = fetchInstruction();
        
        decodeInstruction(opcode);
        std::cout << count << std::endl;
        ++count;
        //register_a_val(2);
        //register_b_val(2);
        //register_f_val(2);
        check_all_registers_flags(3);

    }  
    return 0;
}



