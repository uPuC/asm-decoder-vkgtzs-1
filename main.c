/******************************************************************************
Prac 2 - AVR ASM OpCode Decoder
*******************************************************************************/
/*
    0x2400        CLR       R0             Clear Register
    0xE0A0        LDI       R26,0x00       Load immediate
    0xE0B2        LDI       R27,0x02       Load immediate
    0x910D        LD        R16,X+         Load indirect and postincrement
    0x3000        CPI       R16,0x00       Compare with immediate
    0xF7E9        BRNE      PC-0x02        Branch if not equal
    0x9711        SBIW      R26,0x01       Subtract immediate from word
    0xE0C0        LDI       R28,0x00       Load immediate

    0xE0D2        LDI       R29,0x02       Load immediate
    0x9109        LD        R16,Y+         Load indirect and postincrement
    0x911E        LD        R17,-X         Load indirect and predecrement
    0x1701        CP        R16,R17        Compare
    0xF451        BRNE      PC+0x0B        Branch if not equal
    0x2F0A        MOV       R16,R26        Copy register
    0x950A        DEC       R16            Decrement
    0x2F1C        MOV       R17,R28        Copy register

    0x1701        CP        R16,R17        Compare
    0xF7B9        BRNE      PC-0x08        Branch if not equal
    0x2F0B        MOV       R16,R27        Copy register
    0x2F1D        MOV       R17,R29        Copy register
    0x1701        CP        R16,R17        Compare
    0xF799        BRNE      PC-0x0C        Branch if not equal
    0x9403        INC       R0             Increment
    0x0000        NOP                      No operation   
*/
#include <stdio.h>
#include <inttypes.h>

const uint8_t flash_mem[] ={ 
    0x00, 0x24, 0xA0, 0xE0, 0xB2, 0xE0, 0x0D, 0x91, 0x00, 0x30, 0xE9, 0xF7, 0x11, 0x97, 0xC0, 0xE0, 
    0xD2, 0xE0, 0x09, 0x91, 0x1E, 0x91, 0x01, 0x17, 0x51, 0xF4, 0x0A, 0x2F, 0x0A, 0x95, 0x1C, 0x2F, 
    0x01, 0x17, 0xB9, 0xF7, 0x0B, 0x2F, 0x1D, 0x2F, 0x01, 0x17, 0x99, 0xF7, 0x03, 0x94, 0x00, 0x00 };

const uint16_t inst16_table[] = {
    0x0000, //NOP
    0x2400, //CLR
    0xE000, //LDI
    0x900D, //LD X+
    0x9009, //LD Y+
    0x900E, //LD -X
    0x3000, //CPI
    0xF401, //BRNE
    0x9700, //SBIW
    0x1400, //CP
    0x2F00, //MOV
    0x940A, //DEC, 
    0x9403  //INC
};

enum{
    e_NOP,
    e_CLR,
    e_LDI,
    e_LD_XP,
    e_LD_YP,
    e_LD_MX,
    e_CPI,
    e_BRNE,
    e_SBIW,
    e_CP,
    e_MOV,
    e_DEC,
    e_INC
};

// Op Code struct
typedef union {
    uint16_t op16; // e.g.: watchdog, nop
    struct{
        uint16_t op4:4;
        uint16_t d5:5;
        uint16_t op7:7;
    }type1; // e.g: LSR

    struct{
        uint16_t r4:4;
        uint16_t d5:5;
        uint16_t r1:1;
        uint16_t op6:6;
    }type2; // e.g.: MOV, CP
    //      0010 11rd dddd rrrr <-MOV
    //      0001 01rd dddd rrrr <-CP
    // ---------------------------
    //  AND 1111 1100 0000 0000 | 0xFC00 <-Mascara
    
    struct{
        uint16_t d10:10;
        uint16_t op6:6;
    }type3; // CLR
    //      0010 010d dddd 0000 <-CLR
    // ---------------------------
    //  AND 1111 1110 0000 0000 | 0xFE00 <-Mascara

    struct{
        uint16_t k4:4;
        uint16_t d5:4;
        uint16_t k9:4;
        uint16_t op4:4;
    }type4; //LDI, CPI
    //      1110 KKKK dddd KKKK <-LDI
    //      0011 KKKK dddd KKKK <-CPI
    // ---------------------------
    //  AND 1111 0000 0000 0000 | 0xF000 <-Mascara

    struct{
        uint16_t op4:4;
        uint16_t d5:5;
        uint16_t op7:7;
    }type5;// LD X+, LD Y+, LD -X
    //      1001 000d dddd 1101 <-LD X+
    //      1001 000d dddd 1001 <-LD Y+
    //      1001 000d dddd 1110 <-LD -X
    // -------------------------------
    //  AND 1111 1110 0000 1111 | 0xFE0F <- Mascara

    struct{
        uint16_t op3:3;
        uint16_t k7:7;
        uint16_t op6:6;
    }type6; // BRNE
    //      1111 01kk kkkk k001 <-BRNE
    // -------------------------------
    //  AND 1111 1100 0000 0111 | 0xFC07 <- Mascara

    struct{
        uint16_t k4:4;
        uint16_t d2:2;
        uint16_t k2:2;
        uint16_t op8:8;
    }type7; // SBIW
    //      1001 011d dddd kkkk <-SBIW
    // -------------------------------
    //  AND 1111 1111 0000 0000 | 0xFF00 <- Mascara

    struct{
        uint16_t op4:4;
        uint16_t d5:5;
        uint16_t op7:7;
    }type8; // INC, DEC
    //      1001 010d dddd 1010 <-DEC
    //      1001 010d dddd 0011 <-INC
    // -------------------------------
    //  AND 1111 1110 0000 1111 | 0xFE0F <- Mascara
} Op_Code_t;


int main()
{
    Op_Code_t *instruction;
    printf("- Practica 2: AVR OpCode -\n");

    for (uint8_t idx = 0; idx < sizeof(flash_mem); idx+=2)
    {
        instruction = (Op_Code_t*) &flash_mem[idx];
        if (instruction->op16 == inst16_table[e_NOP])
        {
            printf("NOP\n");
        }
        else if ((instruction->op16 & 0xFE00) == (inst16_table[e_CLR] & 0xFE00))
        {
            uint8_t Rd = instruction->type3.d10 & 0x1F; 
            printf("CLR R%d\n", Rd);
        }
        else if ((instruction->op16 & 0xF000) == (inst16_table[e_LDI] & 0xF000)) 
        {
            uint8_t Rd = 16 + instruction->type4.d5; 
            uint8_t K  = (instruction->type4.k9 << 4) | instruction->type4.k4;
            printf("LDI R%d,0x%02X\n", Rd, K);
        }
         else if ((instruction->op16 & 0xFE0F) == (inst16_table[e_LD_XP] & 0x900D))
        {
            uint8_t Rd = instruction->type5.d5;
            printf("LD R%d,X+\n", Rd);
        }
        else if ((instruction->op16 & 0xFE0F) == (inst16_table[e_LD_YP] & 0x9009))
        {
            uint8_t Rd = instruction->type5.d5;
            printf("LD R%d,Y+\n", Rd);
        }
        else if ((instruction->op16 & 0xFE0F) == (inst16_table[e_LD_MX] & 0x900E))
        {
            uint8_t Rd = instruction->type5.d5;
            printf("LD R%d,-X\n", Rd);
        }
        else if ((instruction->op16 & 0xF000) == (inst16_table[e_CPI] & 0xF000)) {
            uint8_t Rd = 16 + instruction->type4.d5;
            uint8_t K  = (instruction->type4.k9 << 4) | instruction->type4.k4;
            printf("CPI R%d,0x%02X\n", Rd, K);
        }
        else if ((instruction->op16 & 0xFC07) == (inst16_table[e_BRNE] & 0xFC07))
        {
            int8_t k = (int8_t)(instruction->type6.k7 << 1) >> 1;
            k += 1;
            if (k >= 0)
                printf("BRNE PC+0x%02X\n", k);
            else
                printf("BRNE PC-0x%02X\n", -k);
        }
        else if ((instruction->op16 & 0xFF00) == (inst16_table[e_SBIW] & 0xFF00))
        {
            uint8_t d  = 24 + (instruction->type7.d2 * 2);
            uint8_t K  = (instruction->type7.k2 << 4) | instruction->type7.k4;
            printf("SBIW R%d,0x%02X\n", d, K);
        }
        else if ((instruction->op16 & 0xFC00) == (inst16_table[e_CP] & 0xFC00))
        {
            uint8_t Rd = instruction->type2.d5;
            uint8_t Rr = (instruction->type2.r1 << 4) | instruction->type2.r4;
            printf("CP R%d,R%d\n", Rd, Rr);
        }
        else if ((instruction->op16 & 0xFC00) == (inst16_table[e_MOV] & 0xFC00))
        {
            uint8_t Rd = instruction->type2.d5;
            uint8_t Rr = (instruction->type2.r1 << 4) | instruction->type2.r4;
            printf("MOV R%d,R%d\n", Rd, Rr);
        }
        else if ((instruction->op16 & 0xFE0F) == (inst16_table[e_INC] & 0xFE0F)) {
            uint8_t Rd = instruction->type8.d5;
            printf("INC R%d\n", Rd);
        }
        else if ((instruction->op16 & 0xFE0F) == (inst16_table[e_DEC] & 0xFE0F)) {
            uint8_t Rd = instruction->type8.d5;
            printf("DEC R%d\n", Rd);
        }
        else {
            printf("Unknown 0x%04X\n", instruction->op16);
        }
    }
    return 0;
}