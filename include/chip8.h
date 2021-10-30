#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>

#define MAX_WIDTH 64
#define MAX_HEIGHT 32
#define MAX_KEYS 16
#define MAX_RAM 4096
#define MAX_REGISTERS 16
#define MAX_STACK_LEVELS 16
#define FONT_START_ADDR 0x0
#define PC_START_ADDR 0x200
#define NOOP 0x00
#define SPEED 500

typedef struct CHIP8
{
    // Represents 4096 bytes (4KB) of addressable memory.
    unsigned char RAM[MAX_RAM];

    // Represents 16 general-purpose 8-bit registers (V0-VF).
    unsigned char V[MAX_REGISTERS];

    // Delay timer, sound timer and stack pointer 8-bit registers.
    unsigned char DT, ST, SP;

    // Program counter and index 16-bit registers.
    unsigned int PC, I;

    // The call stack. CHIP-8 allows a max of 16 nested calls.
    unsigned int stack[MAX_STACK_LEVELS];

    /* A monochrome display of 64x32 pixels.
    A pixel can be either only on or off, no color. */
    bool display[MAX_HEIGHT][MAX_WIDTH];

    // Represents keys 0-F and if key is not pressed (0), pressed (1), or released (2)
    int keypad[MAX_KEYS];
} CHIP8;

void chip8_init(CHIP8 *chip8);
void chip8_load_font(CHIP8 *chip8);
bool chip8_load_rom(CHIP8 *chip8, char *filename);
void chip8_execute(CHIP8 *chip8);
void chip8_handle_timers(CHIP8 *chip8);
void chip8_beep(CHIP8 *chip8);
void chip8_reset_RAM(CHIP8 *chip8);
void chip8_reset_keypad(CHIP8 *chip8);
void chip8_reset_display(CHIP8 *chip8);
void chip8_load_instr(CHIP8 *chip8, unsigned int instr);

#endif