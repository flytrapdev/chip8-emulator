/**
MIT License

Copyright (c) 2021 Matthieu Le Gallic

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef CHIP8_HPP_INCLUDED
#define CHIP8_HPP_INCLUDED

#include <string>

#define CHIP_W 64
#define CHIP_H 32
#define CHIP_WH 2048
#define SCHIP_W 128
#define SCHIP_H 64
#define SCHIP_WH 8192

class Chip8 {

public:

    //Opcode
    uint16_t opcode;

    //Memory
    uint8_t memory[65535];

    //Registers
    uint8_t v[16];
    uint16_t I;
    uint16_t pc;

    //Graphics bitplanes
    bool gfx[2][SCHIP_WH];
    uint8_t bitPlane;

    //Color palette
    uint8_t palette[4][3];

    //Timers
    uint8_t delayTimer;
    uint8_t soundTimer;

    //XO-Chip audio buffer
    uint8_t audioBuffer[16];

    //Stack
    uint16_t stck[16];
    uint8_t sp;

    //SCHIP user flags
    uint8_t userFlags[8];

    //Keys
    bool keys[16];

    //SCHIP hi-res mode
    bool hiRes;

    //Draw flag
    bool drawFlag;

    //Interpreter stopped
    bool stopped;

    //ROM loaded
    bool loaded;

    //CHIP-8 font sprites
    uint8_t *fontSet;

    //CHIP extensions quirks
    bool loadStoreQuirk;    //FX55 FX65 behavior
    bool shiftQuirk;        //Shift instructions behavior
    bool hiresClearQuirk;   //Clear screen on resolution change (SCHIP and XOCHIP only)
    bool wrapQuirk;         //Sprites wrap around screen boundariess

    Chip8();
    void initialize();
    void unknownOpcode(uint16_t);
    uint8_t loadROM(std::string);
    uint8_t loadPalette(std::string);
    uint8_t checkKeys();
    void updateTimers();
    uint8_t nextByte();
    uint16_t nextWord();
    void skipNextInstruction();
    void scrollLeft(uint8_t);
    void scrollRight(uint8_t);
    void scrollUp(uint8_t);
    void scrollDown(uint8_t);
    void pixel(uint8_t, uint8_t, uint8_t);
    void emulateInstruction();
    void printInstruction(uint16_t, uint16_t);


};

#endif // CHIP8_HPP_INCLUDED
