#pragma once

enum { AY3_INACTIVE=0, AY3_READ=1, AY3_WRITE=2, AY3_LATCH=3 } BDIR_BC1;
enum { R0=0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10=8,R11,R12,R13,R14,R15 } AY3_REGS;

void ay3_write_reg( const uint8_t reg_addr, const uint8_t data );
void delay_us_ay3( const uint64_t us );
int init_ay3();
void end_ay3();

