#include<iostream>
#include<cstdint>
#include<cassert>

// TODO: add functions for 8bit LD interrupt vector and memory refresh
// TODO: add stack pointers and stack 
// TODO: implement exchange traansfer and search group

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

void set_carry_flag(int i) {
    switch(i) {
        case 1: registers.f |= 0b1;break;
        case 0: registers.f &= ~0b1;break;
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
    //checking for half carry
    int H = (registers.a % 16) + (registers.a % 16);
    if(H > 15) {
        set_half_carry_flag(1);
    }
    else {
        set_half_carry_flag(0);
    }

    // doing the addition
    registers.a = (registers.a + registers.a) % 256;

    //checking for sign bit
    int S = twos_comp_displ_int(registers.a);
}

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
    }
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
    }
}

int main(int argc, char *argv[]) {
    loadProgram(argv[1],0x0000);
    while(registers.pc < 65535) {
        
        uint8_t opcode = fetchInstruction();
        decodeInstruction(opcode);
        std::cout << "program counter :"<< static_cast<int>(registers.pc) << std::endl;
        std::cout << "accumulator :"<< static_cast<int>(registers.a) << std::endl;
    }

    //std::cout << "memory at 0100 :" << static_cast<int>(memory[100]) << std::endl;
    
    return 0;
}



