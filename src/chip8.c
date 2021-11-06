#include <stdio.h>
#include <stdlib.h>
#include "chip8.h"

void chip8_init(CHIP8 *chip8, bool legacy_mode, uint16_t clock_speed, uint16_t pc_start_addr, bool q[])
{
    // Seed for the RND instruction.
    srand(time(NULL));

    chip8->legacy_mode = legacy_mode;
    chip8->q = q;

    if (clock_speed <= 0)
    {
        chip8->clock_speed = CLOCK_SPEED_DEFAULT;
    }
    else
    {
        chip8->clock_speed = clock_speed;
    }

    /* Divide by the number of microseconds in a second by the frequency
    to get number of miliseconds to wait between each tick. */
    chip8->timer_max_cum = 1000000 / 60;
    chip8->cpu_max_cum = 1000000 / chip8->clock_speed;

    chip8->pc_start_addr = pc_start_addr;

    chip8_reset(chip8);
}

void chip8_reset(CHIP8 *chip8)
{
    chip8->PC = chip8->pc_start_addr;
    chip8->SP = SP_START_ADDR;
    chip8->I = 0x00;
    chip8->DT = 0;
    chip8->ST = 0;

    // Have cycle times default to current time.
    gettimeofday(&chip8->cur_cycle_start, NULL);
    gettimeofday(&chip8->prev_cycle_start, NULL);

    chip8->cpu_cum = 0;
    chip8->sound_cum = 0;
    chip8->delay_cum = 0;

    chip8->display_updated = false;
    chip8->beep = false;
    chip8->exit = false;
    chip8->hires = false;

    chip8->ROM_path[0] = '\0';
    chip8->UF_path[0] = '\0';

    // S-CHIP did not initialize RAM (does it matter though?)
    if (chip8->legacy_mode || !chip8->q[0])
    {
        chip8_reset_RAM(chip8);
    }

    chip8_reset_registers(chip8);
    chip8_reset_keypad(chip8);
    chip8_reset_display(chip8);
}

void chip8_load_font(CHIP8 *chip8)
{
    uint8_t font_data[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80, // F

        // Big Hex (0-9):
        0xFF, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, // 0
        0x0C, 0x0C, 0x3C, 0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x3F, // 1
        0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, // 2
        0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 3
        0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0x03, 0x03, // 4
        0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 5
        0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, // 6
        0xFF, 0xFF, 0x03, 0x03, 0x0C, 0x0C, 0x30, 0x30, 0x30, 0x30, // 7
        0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, // 8
        0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 9

        /* Big Hex (A-F, which is not defined by original S-CHIP,
        but some programs assume they exist anyway) */
        0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3, // A
        0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, // B
        0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xFF, 0xFF, // C
        0xFC, 0xFC, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFC, 0xFC, // D
        0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, // E
        0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0  // F
    };

    for (uint8_t i = 0; i < sizeof(font_data); i++)
    {
        chip8->RAM[FONT_START_ADDR + i] = font_data[i];
    }
}

bool chip8_load_rom(CHIP8 *chip8, char *filename)
{
    FILE *rom = fopen(filename, "rb");
    if (rom)
    {
        size_t fr = fread(chip8->RAM + chip8->pc_start_addr,
                          MAX_RAM - chip8->pc_start_addr,
                          1,
                          rom);
        (void)fr; // Just to suppress fread unused return value warning.

        fclose(rom);

        sprintf(chip8->ROM_path, "%s", filename);
        sprintf(chip8->UF_path, "%s.uf", filename);
        sprintf(chip8->DMP_path, "%s.sav", filename);

        return true;
    }

    return false;
}

void chip8_cycle(CHIP8 *chip8)
{
    chip8_update_elapsed_time(chip8);

    // Slow the CPU down to match given clock speed.
    chip8->cpu_cum += chip8->total_cycle_time;
    if (chip8->cpu_cum >= chip8->cpu_max_cum)
    {
        chip8->cpu_cum = 0;
        chip8_execute(chip8);
    }

    chip8_handle_timers(chip8);
}

void chip8_execute(CHIP8 *chip8)
{
    /* Fetch */
    // The first and second byte of instruction respectively.
    uint8_t b1 = chip8->RAM[chip8->PC],
            b2 = chip8->RAM[chip8->PC + 1];

    /* Decode */
    // The code (first 4 bits) of instruction.
    uint8_t c = b1 >> 4;

    // The last 12 bits of instruction.
    uint16_t nnn = ((b1 & 0xF) << 8) | b2;

    // The last 4 bits of instruction.
    uint8_t n = b2 & 0xF;

    // The last 4 bits of first byte of instruction.
    uint8_t x = b1 & 0xF;

    // The first 4 bits of second byte of instruction.
    uint8_t y = b2 >> 4;

    // The last 8 bits of instruction.
    uint8_t kk = b2;

    /* Immediately set PC to next instruction
    after fetching and decoding the current one. */
    chip8->PC += 2;
    chip8->display_updated = false;

    /* Execute */
    switch (c)
    {
    case 0x00:
        switch (b2)
        {
        /* HALT (0000)
           Halt the program. Only used by Octo programs. */
        case 0x00:
            chip8->PC -= 2;
            break;

        /* CLS (00E0)
           Clear the display. */
        case 0xE0:
            chip8_reset_display(chip8);
            break;

        /* RET (00EE):
           Return from a subroutine. */
        case 0xEE:
            chip8->PC = (chip8->RAM[chip8->SP] << 8);
            chip8->PC |= chip8->RAM[chip8->SP + 1];
            chip8->SP -= 2;
            break;

        /* EXIT (00FD) (S-CHIP Only):
           Exit the interpreter. */
        case 0xFD:
            if (!chip8->legacy_mode)
            {
                chip8->exit = true;
            }

            break;

        /* SCRR (00FB) (S-CHIP Only):
           Scroll the display right by 4 pixels. */
        case 0xFB:
            if (!chip8->legacy_mode)
            {
                chip8_scroll(chip8, 1, 0, 4);
            }

            break;

        /* SCRL (00FC) (S-CHIP Only):
           Scroll the display left by 4 pixels. */
        case 0xFC:
            if (!chip8->legacy_mode)
            {
                chip8_scroll(chip8, -1, 0, 4);
            }

            break;

        /* LORES (00FE) (S-CHIP Only):
           Disable HI-RES mode. */
        case 0xFE:
            if (!chip8->legacy_mode)
            {
                chip8->hires = false;

                if (!chip8->q[5])
                {
                    chip8_reset_display(chip8);
                }
            }

            break;

        /* HIRES (00FF) (S-CHIP Only):
           Enable HI-RES mode. */
        case 0xFF:
            if (!chip8->legacy_mode)
            {
                chip8->hires = true;

                if (!chip8->q[5])
                {
                    chip8_reset_display(chip8);
                }
            }

            break;

        default:
            /* SCRD (00Cn) (S-CHIP Only):
               Scroll the display down by n pixels. */
            if (y == 0xC)
            {
                if (!chip8->legacy_mode)
                {
                    chip8_scroll(chip8, 0, 1, n);
                }
            }

            break;
        }

        break;

    /* JP addr (1nnn)
       Jump to location nnn. */
    case 0x01:
        chip8->PC = nnn;
        break;

    /* CALL addr (2nnn)
       Call subroutine at nnn. */
    case 0x02:
        chip8->SP += 2;
        chip8->RAM[chip8->SP] = chip8->PC >> 8;
        chip8->RAM[chip8->SP + 1] = chip8->PC & 0x00FF;
        chip8->PC = nnn;
        break;

    /* SE Vx, byte (3xkk)
       Skip next instruction if Vx = kk. */
    case 0x03:
        if (chip8->V[x] == kk)
        {
            chip8->PC += 2;
        }

        break;

    /* SNE Vx, byte (4xkk)
       Skip next instruction if Vx != kk. */
    case 0x04:
        if (chip8->V[x] != kk)
        {
            chip8->PC += 2;
        }

        break;

    /* SE Vx, Vy (5xy0)
       Skip next instruction if Vx = Vy. */
    case 0x05:
        if (chip8->V[x] == chip8->V[y])
        {
            chip8->PC += 2;
        }

        break;

    /* LD Vx, byte (6xkk)
       Set Vx = kk. */
    case 0x06:
        chip8->V[x] = kk;
        break;

    /* ADD Vx, byte (7xkk)
       Set Vx = Vx + kk. */
    case 0x07:
        chip8->V[x] += kk;
        break;

    case 0x08:
        switch (n)
        {
        /* LD Vx, Vy (8xy0)
           Set Vx = Vy. */
        case 0x00:
            chip8->V[x] = chip8->V[y];
            break;

        /* OR Vx, Vy (8xy1)
           Set Vx = Vx OR Vy. */
        case 0x01:
            chip8->V[x] |= chip8->V[y];
            break;

        /* AND Vx, Vy (8xy2)
           Set Vx = Vx AND Vy. */
        case 0x02:
            chip8->V[x] &= chip8->V[y];
            break;

        /* XOR Vx, Vy (8xy3)
           Set Vx = Vx XOR Vy. */
        case 0x03:
            chip8->V[x] ^= chip8->V[y];
            break;

        /* ADD Vx, Vy (8xy4)
           Set Vx = Vx + Vy, set VF = carry. */
        case 0x04:
        {
            bool carry = ((chip8->V[x] + chip8->V[y]) > 0xFF);
            chip8->V[x] += chip8->V[y];
            chip8->V[0x0F] = carry;
            break;
        }

        /* SUB Vx, Vy (8xy5)
           Set Vx = Vx - Vy, set VF = NOT borrow. */
        case 0x05:
        {
            bool no_borrow = (chip8->V[x] >= chip8->V[y]);
            chip8->V[x] = chip8->V[x] - chip8->V[y];
            chip8->V[0x0F] = no_borrow;
            break;
        }

        /* SHR Vx {, Vy} (8xy6)
           Legacy: Set Vx = Vy SHR 1.
           S-CHIP: Set Vx = Vx SHR 1. */
        case 0x06:
            if (chip8->legacy_mode || !chip8->q[1])
            {
                chip8->V[x] = chip8->V[y];
            }

            chip8->V[0x0F] = chip8->V[x] & 0x01;
            chip8->V[x] >>= 1;
            break;

        /* SUBN Vx, Vy (8xy7)
           Set Vx = Vy - Vx, set VF = NOT borrow. */
        case 0x07:
        {
            bool no_borrow = (chip8->V[y] >= chip8->V[x]);
            chip8->V[x] = chip8->V[y] - chip8->V[x];
            chip8->V[0x0F] = no_borrow;
            break;
        }

        /* SHL Vx {, Vy} (8xyE)
           Legacy: Set Vx = Vy SHL 1.
           S-CHIP: Set Vx = Vx SHL 1. */
        case 0x0E:
            if (chip8->legacy_mode || !chip8->q[1])
            {
                chip8->V[x] = chip8->V[y];
            }

            chip8->V[0x0F] = (chip8->V[x] & 0x80) >> 7;
            chip8->V[x] <<= 1;
            break;
        }

        break;

    /* SNE Vx, Vy (9xy0)
       Skip next instruction if Vx != Vy. */
    case 0x09:
        if (chip8->V[x] != chip8->V[y])
        {
            chip8->PC += 2;
        }

        break;

    /* LD I, addr (Annn)
       Set I = nnn. */
    case 0x0A:
        chip8->I = nnn;
        break;

    /* JP V0, addr (Bnnn)
       Legacy: Jump to location nnn + V0.
       S-CHIP: Jump to location nnn + Vx. */
    case 0x0B:
        chip8->PC = (chip8->legacy_mode || !chip8->q[3]) ? chip8->V[0] + nnn : chip8->V[x] + nnn;
        break;

    /* RND Vx, byte (Cxkk)
       Set Vx = random byte AND kk. */
    case 0x0C:
        chip8->V[x] = (rand() % 0x100) & kk;
        break;

    /* DRW Vx, Vy, n (Dxyn):
       Legacy: Display n-byte sprite starting at memory location I at (Vx, Vy),
       set VF = collision.
       S-CHIP: If hires=false: If n=0, display 8x16 sprite. Else:
       Same as Legacy. If hires=true: Same as Legacy, except
       set VF = num rows collision. If n=0: Display 16x16 sprite starting at
       memory location I at (Vx, Vy), set VF = num rows collision. */
    case 0x0D:
        chip8_draw(chip8, chip8->V[x], chip8->V[y], n);
        break;

    case 0x0E:
        switch (b2)
        {
        /* SKP Vx (Ex9E)
           Skip next instruction if key with the value of Vx is pressed. */
        case 0x9E:
            if (chip8->keypad[chip8->V[x]] == KEY_DOWN)
            {
                chip8->PC += 2;
            }

            break;

        /* SKNP Vx (ExA1)
           Skip next instruction if key with the value of Vx is not pressed. */
        case 0xA1:
            if (chip8->keypad[chip8->V[x]] == KEY_UP)
            {
                chip8->PC += 2;
            }

            break;
        }

        break;

    case 0x0F:
        switch (b2)
        {
        /* LD Vx, DT (Fx07)
           Set Vx = delay timer value. */
        case 0x07:
            chip8->V[x] = chip8->DT;
            break;

        /* LD Vx, K (Fx0A)
           Wait for a key press, store the value of the key in Vx. */
        case 0x0A:
        {
            chip8_wait_key(chip8, x);
            break;
        }

        /* LD DT, Vx (Fx15)
           Set delay timer = Vx. */
        case 0x15:
            chip8->DT = chip8->V[x];
            break;

        /* LD ST, Vx (Fx18)
           Set sound timer = Vx. */
        case 0x18:
            chip8->ST = chip8->V[x];
            break;

        /* ADD I, Vx (Fx1E):
           Set I = I + Vx. */
        case 0x1E:
            chip8->I += chip8->V[x];
            break;

        /* LD F, Vx (Fx29)
           Set I = location of 5-byte sprite for digit Vx. */
        case 0x29:
            chip8->I = FONT_START_ADDR + (chip8->V[x] * 0x05);
            break;

        /* LD HF, Vx (Fx30) (S-CHIP Only)
           Set I = location of 10-byte sprite for digit Vx. */
        case 0x30:
            if (!chip8->legacy_mode)
            {
                chip8->I = BIG_FONT_START_ADDR + (chip8->V[x] * 0x0A);
            }

            break;

        /* LD B, Vx (Fx33)
           Store BCD representation of Vx in memory locations:
           I, I+1, and I+2. */
        case 0x33:
            chip8->RAM[chip8->I] = (chip8->V[x] / 100) % 10;
            chip8->RAM[chip8->I + 1] = (chip8->V[x] / 10) % 10;
            chip8->RAM[chip8->I + 2] = chip8->V[x] % 10;
            break;

        /* LD [I], Vx (Fx55)
           Store registers V0 through Vx in memory starting at location I.
           Legacy: Set I=I+x+1 */
        case 0x55:
            for (int r = 0; r <= x; r++)
            {
                chip8->RAM[chip8->I + r] = chip8->V[r];
            }

            if (chip8->legacy_mode || !chip8->q[2])
            {
                chip8->I += (x + 1);
            }

            break;

        /* LD Vx, [I] (Fx65)
           Read registers V0 through Vx from memory starting at location I.
           Legacy: Set I=I+x+1 */
        case 0x65:
            for (int r = 0; r <= x; r++)
            {
                chip8->V[r] = chip8->RAM[chip8->I + r];
            }

            if (chip8->legacy_mode || !chip8->q[2])
            {
                chip8->I += (x + 1);
            }

            break;

        /* LD uflags_disk, V0..Vx (Fx75) (S-CHIP Only)
           Save user flags to disk. */
        case 0x75:
            if (!chip8->legacy_mode)
            {
                if (!chip8_handle_user_flags(chip8, x + 1, true))
                {
                    fprintf(stderr, "Unable to save user flags to %s\n", chip8->UF_path);
                }
            }

            break;

        /* LD V0..Vx, uflags_disk (Fx85) (S-CHIP Only)
           Load user flags from disk. */
        case 0x85:
            if (!chip8->legacy_mode)
            {
                if (!chip8_handle_user_flags(chip8, x + 1, false))
                {
                    fprintf(stderr, "Unable to load user flags from %s\n", chip8->UF_path);
                }
            }

            break;
        }

        break;
    }

    // Any key that was released previous frame gets turned off.
    chip8_reset_released_keys(chip8);
}

void chip8_handle_timers(CHIP8 *chip8)
{
    if (chip8->DT > 0)
    {
        chip8->delay_cum += chip8->total_cycle_time;

        if (chip8->delay_cum >= chip8->timer_max_cum)
        {
            chip8->DT--;
            chip8->delay_cum = 0;
        }
    }
    if (chip8->ST > 0)
    {
        // On original COSMAC VIP, sound is only produced when ST >= 2
        chip8->beep = chip8->ST >= 2;
        chip8->sound_cum += chip8->total_cycle_time;

        if (chip8->sound_cum >= chip8->timer_max_cum)
        {
            chip8->ST--;
            chip8->sound_cum = 0;
        }
    }
    else
    {
        chip8->beep = false;
    }
}

void chip8_update_elapsed_time(CHIP8 *chip8)
{
    chip8->prev_cycle_start.tv_sec = chip8->cur_cycle_start.tv_sec;
    chip8->prev_cycle_start.tv_usec = chip8->cur_cycle_start.tv_usec;
    gettimeofday(&chip8->cur_cycle_start, NULL);
    chip8->total_cycle_time = ((chip8->cur_cycle_start.tv_sec - chip8->prev_cycle_start.tv_sec) * 1000000);
    chip8->total_cycle_time += (chip8->cur_cycle_start.tv_usec - chip8->prev_cycle_start.tv_usec);
}

void chip8_reset_keypad(CHIP8 *chip8)
{
    for (int k = 0; k < MAX_KEYS; k++)
    {
        chip8->keypad[k] = KEY_UP;
    }
}

void chip8_reset_released_keys(CHIP8 *chip8)
{
    for (int k = 0; k < MAX_KEYS; k++)
    {
        if (chip8->keypad[k] == KEY_RELEASED)
        {
            chip8->keypad[k] = KEY_UP;
        }
    }
}

void chip8_reset_display(CHIP8 *chip8)
{
    for (int y = 0; y < MAX_HEIGHT; y++)
    {
        for (int x = 0; x < MAX_WIDTH; x++)
        {
            chip8->display[y][x] = false;
        }
    }

    chip8->display_updated = true;
}

void chip8_reset_RAM(CHIP8 *chip8)
{
    for (int i = 0; i < MAX_RAM; i++)
    {
        chip8->RAM[i] = 0x00;
    }
}

void chip8_reset_registers(CHIP8 *chip8)
{
    for (int i = 0; i < MAX_REGISTERS; i++)
    {
        chip8->V[i] = 0x00;
    }
}

void chip8_load_instr(CHIP8 *chip8, uint16_t instr)
{
    chip8->RAM[chip8->pc_start_addr] = instr >> 8;
    chip8->RAM[chip8->pc_start_addr + 1] = instr & 0x00FF;
}

void chip8_draw(CHIP8 *chip8, uint8_t x, uint8_t y, uint8_t n)
{
    chip8->V[0x0F] = 0;

    /* n==0 only has signifigance in S-CHIP mode,
    otherwise nothing should be drawn. */
    if (n == 0 && !chip8->legacy_mode)
    {
        /* Draw a 32-byte (16x16) sprite in hires or
        a 16-byte (8x16) sprite in lores. */
        n = (chip8->hires || !chip8->q[4]) ? 32 : 16;
    }

    if (chip8->hires && chip8->q[8])
    {
        int rows = (n == 32) ? 16 : n;
        chip8->V[0x0F] += ((y + rows) - (MAX_HEIGHT - 1));
    }

    bool prev_byte_collide = false;
    for (int i = 0; i < n; i++)
    {
        bool collide_row = false;

        for (int j = 0; j < 8; j++)
        {
            /* For big sprites, every odd byte needs to be drawn on the same
            row as the previous byte. This is achieved through integer division
            truncation. The odd byte also needs to be drawn 8 pixels to the
            right of the previous byte. */
            unsigned y_start = (n == 32) ? (i / 2) : i;
            unsigned x_start = (n == 32 && (i % 2 != 0)) ? (j + 8) : j;

            /* Now we have to scale the display if we are in lo-res mode
            by basically drawing each pixel twice. */
            int scale = chip8->hires ? 1 : 2;
            for (int h = 0; h < scale; h++)
            {
                for (int k = 0; k < scale; k++)
                {
                    int disp_x = (x * scale) + (x_start * scale) + k;
                    int disp_y = (y * scale) + (y_start * scale) + h;

                    // Allow out-of-bound sprite to wrap-around in legacy mode.
                    if (chip8->legacy_mode || !chip8->q[6])
                    {
                        disp_x %= MAX_WIDTH;
                        disp_y %= MAX_HEIGHT;
                    }
                    else
                    {
                        if (disp_x >= MAX_WIDTH || disp_y >= MAX_HEIGHT)
                        {
                            break;
                        }
                    }

                    // Get the pixel the loop is on and the corresponding bit.
                    bool pixel_on = chip8->display[disp_y][disp_x];
                    bool bit = (chip8->RAM[chip8->I + i] >> (7 - j)) & 0x01;

                    /* XOR the sprite onto display. 
                    If a pixel is erased, set the VF register to 1. */
                    chip8->display[disp_y][disp_x] = (pixel_on ^ bit);
                    if (pixel_on && bit)
                    {
                        if (chip8->hires && chip8->q[7])
                        {
                            if (!collide_row)
                            {
                                if (n <= 16 || (((i % 2 == 0) && n == 32) || !prev_byte_collide))
                                {
                                    chip8->V[0x0F]++;
                                    collide_row = true;
                                }
                            }
                        }
                        else
                        {
                            chip8->V[0x0F] = 1;
                        }
                    }
                }
            }
        }

        prev_byte_collide = collide_row;
    }

    chip8->display_updated = true;
}

void chip8_scroll(CHIP8 *chip8, int xdir, int ydir, int num_pixels)
{
    int x_start = 0;
    int x_end = MAX_WIDTH;
    int y_start = 0;
    int y_end = MAX_HEIGHT;

    if (xdir == 1)
    {
        x_end = MAX_WIDTH - num_pixels;
    }
    else if (xdir == -1)
    {
        x_start = num_pixels;
    }
    if (ydir == 1)
    {
        y_end = MAX_HEIGHT - num_pixels;
    }
    else if (ydir == -1)
    {
        y_start = num_pixels;
    }

    // Create updated display buffer.
    uint8_t disp_buf[MAX_HEIGHT][MAX_WIDTH];
    for (int i = 0; i < MAX_HEIGHT; i++)
    {
        for (int j = 0; j < MAX_WIDTH; j++)
        {
            disp_buf[i][j] = false;
        }
    }

    for (int y = y_start; y < y_end; y++)
    {
        for (int x = x_start; x < x_end; x++)
        {
            disp_buf[y + (ydir * num_pixels)][x + (xdir * num_pixels)] = chip8->display[y][x];
        }
    }

    // Copy updated display buffer into actual display.
    for (int y = 0; y < MAX_HEIGHT; y++)
    {
        for (int x = 0; x < MAX_WIDTH; x++)
        {
            chip8->display[y][x] = disp_buf[y][x];
        }
    }
}

void chip8_wait_key(CHIP8 *chip8, uint8_t x)
{
    bool key_released = false;

    for (int i = 0; i < MAX_KEYS; i++)
    {
        if (chip8->keypad[i] == KEY_RELEASED)
        {
            chip8->V[x] = i;
            key_released = true;
            break;
        }
    }

    if (!key_released)
    {
        chip8->PC -= 2;
    }
}

bool chip8_dump_RAM(CHIP8 *chip8)
{
    FILE *dmp = fopen(chip8->DMP_path, "wb");
    if (dmp)
    {
        size_t fw = fwrite(chip8->RAM, MAX_RAM, 1, dmp);
        (void)fw; // Just to suppress fwrite unused return value warning.

        fclose(dmp);

        return true;
    }

    return false;
}

bool chip8_handle_user_flags(CHIP8 *chip8, int num_flags, bool save)
{
    if (num_flags <= MAX_USER_FLAGS)
    {
        char mode[] = "xb";
        mode[0] = save ? 'w' : 'r';
        FILE *fflags = fopen(chip8->UF_path, mode);

        if (fflags)
        {
            size_t fop;
            if (save)
            {
                fop = fwrite(chip8->V, num_flags, 1, fflags);
            }
            else
            {
                fop = fread(chip8->V, num_flags, 1, fflags);
            }
            (void)fop; // Just to suppress unused return value warning.

            fclose(fflags);

            return true;
        }
    }
    else
    {
        return true; // Only return false on file open error.
    }

    return false;
}
