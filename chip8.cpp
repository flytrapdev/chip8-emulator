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

#include "chip8.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

Chip8::Chip8() {
    //ROM loaded
    loaded = false;

    //CHIP extensions quirks
    loadStoreQuirk = false;     //FX55 FX65 SCHIP behavior
    shiftQuirk = false;         //SCHIP shift behavior
    hiresClearQuirk = true;     //Clear screen when changing resolutions
    wrapQuirk = false;          //Sprites do not wrap around by default

    //Default palette
    palette[0][0] = 0x00;
    palette[0][1] = 0x00;
    palette[0][2] = 0x00;
    palette[1][0] = 0x54;
    palette[1][1] = 0x54;
    palette[1][2] = 0x54;
    palette[2][0] = 0xa8;
    palette[2][1] = 0xa8;
    palette[2][2] = 0xa8;
    palette[3][0] = 0xfc;
    palette[3][1] = 0xfc;
    palette[3][2] = 0xfc;

    initialize();
};

//Initialize CHIP-8
void Chip8::initialize() {
    pc = 0x200; //Program counter
    I = 0;      //Index register
    sp = 0;     //Stack pointer
    opcode = 0; //Current opcode

    hiRes = false;
    drawFlag = true;

    //Stop flag used by SUPERCHIP
    stopped = false;

    //Clear graphics bit planes
    memset(gfx[0], false, SCHIP_WH);
    memset(gfx[1], false, SCHIP_WH);

    bitPlane = 1;

    delayTimer = 0;
    soundTimer = 0;

    memset(keys, false, 16);
    memset(v, false, 16);

    //Font set
    uint8_t fontSet[180] = {
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

        // Hi-res font (0-9) (SCHIP)
        0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C,
        0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C,
        0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF,
        0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C,
        0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06,
        0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C,
        0x3E, 0x7C, 0xC0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C,
        0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60,
        0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C,
        0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C
    };

    memcpy(memory, fontSet, 180);
};

//Print unknown opcode error
void Chip8::unknownOpcode(uint16_t opcode) {
    std::cout << "Unknown opcode 0x" << std::hex << std::setfill('0') << std::setw(4) << opcode << std::endl;
};

//Load program
uint8_t Chip8::loadROM(std::string filename) {

    std::cout << "Loading ROM " << filename << std::endl;

    #ifdef _WIN32
    int fd = open(filename.c_str(), O_RDONLY | O_BINARY);
    #else
    int fd = open(filename.c_str(), O_RDONLY);
    #endif

    if(fd == -1) {
        std::cout << "Could not read file " << filename << std::endl;
        return -1;
    }

    int len;
    int pos = 0;

    uint8_t byte;

    len = read(fd, &byte, 1);

    while(len > 0) {
        memory[0x200 + pos] = byte;
        len = read(fd, &byte, 1);
        pos ++;
    }

    close(fd);

    std::cout << "Loaded : " << pos << " bytes" << std::endl;

    loaded = true;

    return 0;

};

//Load palette file
uint8_t Chip8::loadPalette(std::string filename) {

    std::cout << "Loading palette file " << filename << std::endl;

    std::ifstream file(filename.c_str());

    if(!file.is_open()) {
        std::cout << "Could not read palette file " << filename << std::endl;
        return -1;
    }

    uint8_t colorCount = 0;
    uint32_t color = 0;

    std::string line;

    //Read line by line
    while(std::getline(file, line) && colorCount < 4) {

        if(line.size() != 0) {
            std::stringstream sstream;
            sstream << std::hex << std::uppercase << line;

            sstream >> color;

            palette[colorCount][0] = (color & 0xff0000) >> 16;
            palette[colorCount][1] = (color & 0x00ff00) >> 8;
            palette[colorCount][2] = (color & 0x0000ff);

            colorCount ++;
        }
    }


    return 0;
}

//Check pressed keys
uint8_t Chip8::checkKeys() {

    for(int i = 0 ; i < 16 ; i++)
        if(keys[i])
            return i;

    return 255;
};

//Read next byte and increment pc
uint8_t Chip8::nextByte() {
    uint8_t byte = memory[pc];
    pc ++;

    return byte;
}

//Read next 2 bytes and increment pc
uint16_t Chip8::nextWord() {
    uint16_t word = (memory[pc] << 8) | memory[pc + 1];
    pc += 2;

    return word;
}

//Skip next instruction
void Chip8::skipNextInstruction() {

    //XO-CHIP has double-length F0000 NNNN instruction
    if(nextWord() == 0xF000)
        pc += 2;
}

//Decrement delay and sound timers
void Chip8::updateTimers() {
    if(delayTimer > 0)
        delayTimer --;

    if(soundTimer > 0)
        soundTimer --;
}

//Scroll left
void Chip8::scrollLeft(uint8_t pixels) {

    for(uint8_t y0 = 0 ; y0 < SCHIP_H ; y0 ++) {
        if((bitPlane & 0x1) != 0) {
            memmove(gfx[0] + y0*SCHIP_W,   gfx[0] + pixels + y0*SCHIP_W,   SCHIP_W - pixels);
            memset(gfx[0] + SCHIP_W - pixels + y0*SCHIP_W,   false,   pixels);
        }

        if((bitPlane & 0x2) != 0) {
            memmove(gfx[1] + y0*SCHIP_W,   gfx[1] + pixels + y0*SCHIP_W,   SCHIP_W - pixels);
            memset(gfx[1] + SCHIP_W - pixels + y0*SCHIP_W,   false,   pixels);
        }
    }

    drawFlag = true;
}

//Scroll right
void Chip8::scrollRight(uint8_t pixels) {

    for(uint8_t y0 = 0 ; y0 < SCHIP_H ; y0 ++) {
        if((bitPlane & 0x1) != 0) {
            memmove(gfx[0] + pixels + y0*SCHIP_W,   gfx[0] + y0*SCHIP_W,   SCHIP_W - pixels);
            memset(gfx[0] + y0*SCHIP_W,   false,   pixels);
        }

        if((bitPlane & 0x2) != 0) {
            memmove(gfx[1] + pixels + y0*SCHIP_W,   gfx[1] + y0*SCHIP_W,   SCHIP_W - pixels);
            memset(gfx[1] + y0*SCHIP_W,   false,   pixels);
        }
    }

    drawFlag = true;
}

//Scroll down
void Chip8::scrollDown(uint8_t pixels) {

    if((bitPlane & 0x1) != 0) {
        memmove(gfx[0] + SCHIP_W * pixels,   gfx[0],   SCHIP_W * (SCHIP_H - pixels));
        memset(gfx[0],   false,   SCHIP_W * pixels);
    }

    if((bitPlane & 0x2) != 0) {
        memmove(gfx[1] + SCHIP_W * pixels,   gfx[1],   SCHIP_W * (SCHIP_H - pixels));
        memset(gfx[1],   false,   SCHIP_W * pixels);
    }


    drawFlag = true;
}

//Scroll up
void Chip8::scrollUp(uint8_t pixels) {

    if((bitPlane & 0x1) != 0) {
        memmove(gfx[0],   gfx[0] + SCHIP_W * pixels,   SCHIP_W * (SCHIP_H - pixels));
        memset(gfx[0] + SCHIP_W * (SCHIP_H - pixels),   false,   SCHIP_W * pixels);
    }

    if((bitPlane & 0x2) != 0) {
        memmove(gfx[1],   gfx[1] + SCHIP_W * pixels,   SCHIP_W * (SCHIP_H - pixels));
        memset(gfx[1] + SCHIP_W * (SCHIP_H - pixels),   false,   SCHIP_W * pixels);
    }

    drawFlag = true;
}

//Draw pixel to the gfx buffer
void Chip8::pixel(uint8_t x, uint8_t y, uint8_t sprPlane) {

    for(uint8_t plane = 0 ; plane < 2 ; plane ++) {

        //XO-CHIP bitplanes
        if((sprPlane & (plane+1)) != 0) {

            //Memory location
            uint16_t addr = x + SCHIP_W * y;

            //Collision flag
            if(gfx[plane][addr])
                v[0xF] = 1;

            //VRAM
            gfx[plane][addr] ^= 1;
        }
    }
}

//Emulate CHIP-8 instruction
void Chip8::emulateInstruction() {

    opcode = nextWord();

    switch(opcode & 0xF000) {

        case 0x0000:{

            if((opcode & 0x00F0) == 0x00C0) {
                //0x00CN
                //(SCHIP) Scroll down by N pixels
                scrollDown(opcode & 0x000F);
            }
            else if ((opcode & 0x00F0) == 0x00D0) {
                //0x00DN
                //(XO-CHIP) Scroll up by N pixels
                scrollUp(opcode & 0x000F);
            }
            else
                switch(opcode & 0x00FF) {

                    case 0x00E0: {
                        //0x00E0
                        //Clear screen
                        if((bitPlane & 0x1) != 0)
                            memset(gfx[0], false, SCHIP_WH);

                        if((bitPlane & 0x2) != 0)
                            memset(gfx[1], false, SCHIP_WH);

                        drawFlag = true;
                        break;
                    }

                    case 0x00EE: {
                        //0x00EE
                        //Return
                        sp --;
                        pc = stck[sp];
                        break;
                    }

                    case 0x00FB: {
                        //0x00FB
                        //(SCHIP) Scroll right by 4 pixels
                        scrollRight(4);
                        break;
                    }

                    case 0x00FC: {
                        //0x00FC
                        //(SCHIP) Scroll left by 4 pixels
                        scrollLeft(4);
                        break;
                    }

                    case 0x00FD: {
                        //0x00FD
                        //(SCHIP) Stop
                        stopped = true;
                        pc -= 2;
                        break;
                    }

                    case 0x00FE: {
                        //0x00FE
                        //(SCHIP) disable hi-res mode
                        //TODO clears the screen in XO-CHIP
                        memset(gfx, false, SCHIP_WH * 2);

                        hiRes = false;
                        drawFlag = true;
                        break;
                    }

                    case 0x00FF: {
                        //0x00FF
                        //(SCHIP) enable hi-res mode
                        //TODO clears the screen in XO-CHIP
                        memset(gfx, false, SCHIP_WH * 2);

                        hiRes = true;
                        drawFlag = true;
                        break;
                    }

                    default: {
                        //Unknown opcode
                        unknownOpcode(opcode);
                        break;
                    }

                }

            break;
        }

        case 0x1000: {
            //0x1NNN
            //Jump to location NNN
            pc = opcode & 0xFFF;
            break;
        }

        case 0x2000: {
            //0x2NNN
            //Call location NNN
            stck[sp] = pc;
            sp ++;
            pc = (opcode & 0xFFF);
            break;
        }

        case 0x3000: {
            //0x3XNN
            //Skip next instruction if VX == NN
            if((opcode & 0x00FF) == v[(opcode & 0x0F00) >> 8])
                skipNextInstruction();

            break;
        }

        case 0x4000: {
            //0x4XNN
            //Skip next instruction if VX != NN
            if((opcode & 0x00FF) != v[(opcode & 0x0F00) >> 8])
                skipNextInstruction();

            break;
        }

        case 0x5000: {

            switch(opcode & 0x000F) {
                
                case 0x0000: {
                    //0x5XY0
                    //Skip next instruction if VX == VY
                    if(v[(opcode & 0x0F00) >> 8] == v[(opcode & 0x00F0) >> 4])
                        skipNextInstruction();
                    break;
                }

                case 0x0002: {
                    //0x5XY2
                    //(XO-CHIP) Save VX..VY to memory at location I
                    uint8_t x = ((opcode & 0x0F00) >> 8);
                    uint8_t y = ((opcode & 0x00F0) >> 4);

                    if(y >= x)
                        memcpy(memory + I, v + x, 1 + y - x);
                    else {
                        //Reverse
                        for(uint8_t i = 0 ; i <= x - y ; i ++)
                            memory[I + i] = v[x - i];
                    }

                    break;
                }

                case 0x0003: {
                    //0x5XY3
                    //(XO-CHIP) Load VX..VY from memory at location I
                    uint8_t x = ((opcode & 0x0F00) >> 8);
                    uint8_t y = ((opcode & 0x00F0) >> 4);

                    if(y >= x)
                        memcpy(v + x, memory + I, 1 + y - x);
                    else {
                        //Reverse
                        for(uint8_t i = 0 ; i <= x - y ; i ++)
                            v[x - i] = memory[I + i];
                    }

                    break;
                }

                default : break;
            }

            break;
        }

        case 0x6000: {
            //0x6XNN
            //Load NN into VX
            v[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
            break;
        }

        case 0x7000: {
            //0x7XNN
            //Add NN to VX
            v[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
            break;
        }

        case 0x8000: {
            uint8_t x = (opcode & 0x0F00) >> 8;
            uint8_t y = (opcode & 0x00F0) >> 4;

            switch(opcode & 0x000F) {

                case 0x0000: {
                    //0x8XY0
                    //Set VX = VY
                    v[x] = v[y];
                    break;
                }

                case 0x0001: {
                    //0x8XY1
                    //Set VX = VX OR VY
                    v[x] |= v[y];
                    break;
                }

                case 0x0002: {
                    //0x8XY2
                    //Set VX = VX AND VY
                    v[x] &= v[y];
                    break;
                }

                case 0x0003: {
                    //0x8XY3
                    //Set VX = VX XOR VY
                    v[x] ^= v[y];
                    break;
                }

                case 0x0004: {
                    //0x8XY4
                    //Set VX = VX + VY
                    //Set VF = carry
                    uint8_t carry = ((v[x] + v[y]) > 0xFF) ? 1 : 0;
                    v[x] += v[y];
                    v[0xF] = carry;
                    break;
                }

                case 0x0005: {
                    //0x8XY5
                    //Set VX = VX - VY
                    //Set VF = carry
                    uint8_t carry = (v[y] > v[x]) ? 0 : 1;
                    v[x] -= v[y];
                    v[0xF] = carry;
                    break;
                }

                case 0x0006: {
                    //0x8XY6
                    //Shift VY right, store result in VX
                    //Set VF = least significant bit of VY

                    //(SCHIP) Shift VX right
                    //Set VF = least significant bit of VX

                    if(shiftQuirk)
                        y = x;
                    
                    uint8_t carry = v[y] & 0x01;
                    v[x] = v[y] >> 1;
                    v[0xF] = carry;

                    break;
                }

                case 0x0007: {
                    //0x8XY7
                    //Set VX = VY - VX
                    //Set VF = Not borrow
                    uint8_t carry = (v[x] > v[y]) ? 0 : 1;
                    v[x] = v[y] - v[x];
                    v[0xF] = carry;

                    break;
                }

                case 0x000E: {
                    //0x8XYE
                    //Shift VY left, store result in VX
                    //Set VF = most significant bit of VY

                    //(SCHIP) Shift VX left
                    //Set VF = most significant bit of VX

                    if(shiftQuirk)
                        y = x;

                    uint8_t carry = v[y] >> 7;
                    v[x] = v[y] << 1;
                    v[0xF] = carry;

                    break;
                }

                default: {
                    //Unknown opcode
                    unknownOpcode(opcode);
                    break;
                }

            }

            break;
        }

        case 0x9000: {
            //0x9XY0
            //Skip next instruction if VX != VY
            if(v[(opcode & 0x0F00) >> 8] != v[(opcode & 0x00F0) >> 4])
                skipNextInstruction();
            break;
        }

        case 0xA000: {
            //0xANNN
            //Set I = NNN
            I = (opcode & 0xFFF);
            break;
        }

        case 0xB000: {
            //0xBNNN
            //Jump to address NNN + V0
            pc = ((opcode & 0xFFF) + v[0]);
            break;
        }

        case 0xC000: {
            //0xCNNN
            //Set VX = Random (0 -> 255) AND NN
            v[(opcode & 0x0F00) >> 8] = opcode & (rand() & 0xFF);
            break;
        }

        case 0xD000: {
            //0xDXYN
            //Draw sprite

            //Dot size on screen
            uint8_t pSize = hiRes ? 1 : 2;

            //Opcode parameters
            uint8_t x = v[(opcode & 0x0F00) >> 8];
            uint8_t y = v[(opcode & 0x00F0) >> 4];
            uint8_t n = opcode & 0x000F;

            //Pixel coordinates
            uint8_t x0, y0;

            //Bit mask
            uint16_t mask;

            //Collision flag
            v[0xF] = 0;

            uint8_t dX, dY;

            uint8_t height, memHeight;
            uint8_t sprPlane = bitPlane;

            //Octo quirk : DXY0 draws 16x16 sprite even in loRes mode
            if(n == 0) {

                //16x16 sprite
                height = 16;
                memHeight = 16;

                //XO-CHIP multicolor sprite
                if(bitPlane == 3) {
                    memHeight = 32;
                    sprPlane = 1;
                }

                for(dY = 0 ; dY < memHeight ; dY ++) {

                    y0 = ((y + (dY % height)) * pSize) % SCHIP_H;

                    //Multicolor sprites
                    if(dY >= height)
                        sprPlane = 2;

                    for(dX = 0 ; dX < 16 ; dX ++) {

                        mask = 0x8000 >> dX;
                        x0 = ((x + dX) * pSize) % SCHIP_W;

                        //Sprites don't wrap around the screen in XOCHIP mode
                        if(wrapQuirk || (((x + dX) * pSize < SCHIP_W) && (((y + (dY % height)) * pSize < SCHIP_H)))) {

                            if(((memory[I + 2*dY] << 8 | memory[I + 2*dY + 1]) & mask) != 0) {

                                pixel(x0, y0, sprPlane);

                                if(!hiRes) {
                                    pixel(x0 + 1, y0, sprPlane);
                                    pixel(x0, y0 + 1, sprPlane);
                                    pixel(x0 + 1, y0 + 1, sprPlane);
                                }
                            }
                        }
                    }

                }

            }
            else {
                //Regular sprite drawing

                height = n;
                memHeight = n;

                if(bitPlane == 3) {
                    memHeight = n * 2;
                    sprPlane = 1;
                }

                //Regular sprite
                for(dY = 0 ; dY < memHeight ; dY ++) {

                    y0 = ((y + (dY % height)) * pSize) % SCHIP_H;

                    if(dY >= height)
                        sprPlane = 2;

                    for(dX = 0 ; dX < 8 ; dX ++) {

                        mask = 0x80 >> dX;
                        x0 = ((x + dX) * pSize) % SCHIP_W;

                        if(wrapQuirk || (((x + dX) * pSize < SCHIP_W) && (((y + (dY % height)) * pSize < SCHIP_H)))) {
                            if((memory[I + dY] & mask) != 0) {

                                pixel(x0, y0, sprPlane);

                                if(!hiRes) {
                                    pixel(x0 + 1, y0, sprPlane);
                                    pixel(x0, y0 + 1, sprPlane);
                                    pixel(x0 + 1, y0 + 1, sprPlane);
                                }
                            }
                        }

                    }

                }

            }

            drawFlag = true;
            break;
        }

        case 0xE000: {
            uint8_t key = v[(opcode & 0x0F00) >> 8] & 0xF;
            switch(opcode & 0x00FF) {

                case 0x009E: {
                    //0xEX9E
                    //Skip next instruction if key VX is pressed
                    if(keys[key])
                        skipNextInstruction();

                    break;
                }

                case 0x00A1: {
                    //0xEXA1
                    //Skip next instruction if key VX is not pressed
                    if(!keys[key])
                        skipNextInstruction();

                    break;
                }

                default: {
                    //Unknown opcode
                    unknownOpcode(opcode);
                    break;
                }
            }
            break;
        }

        case 0xF000: {
            uint16_t x = (opcode & 0x0F00) >> 8;
            uint16_t opcodeLast = opcode & 0x00FF;

            switch(opcodeLast) {

                case 0x0000: {
                    //0xF000 NNNN
                    //(XO-CHIP) Load NNNN into I
                    I = nextWord();
                    break;
                }

                case 0x0001: {
                    //0xFN01
                    //(XO-CHIP) bitplane N select
                    bitPlane = (opcode & 0x0F00) >> 8;
                    break;
                }

                case 0x0002: {
                    //0xF002
                    //(XO-CHIP) Store to audio buffer
                    for(uint8_t i = 0 ; i < 16 ; i++)
                        audioBuffer[i] = memory[I + i];
                    break;
                }

                case 0x0007: {
                    //0xFX07
                    //Set VX = delay timer
                    v[x] = delayTimer;
                    break;
                }

                case 0x000A: {
                    //0xFX0A
                    //Wait for key press then store key into Vx
                    uint8_t key = checkKeys();

                    if(key < 16)
                        v[x] = key;
                    else
                        pc -= 2;

                    break;
                }

                case 0x0015: {
                    //0xFX15
                    //Set delay timer = VX
                    delayTimer = v[x];
                    break;
                }

                case 0x0018: {
                    //0xFX18
                    //Set sound timer = VX
                    soundTimer = v[x];
                    break;
                }

                case 0x001E: {
                    //0xFX1E
                    //Set I = I + VX
                    uint8_t carry = (I + v[x] > 0xFFF)? 1 : 0;

                    I += v[x];
                    v[0xF] = carry;

                    break;
                }

                case 0x0029: {
                    //0xFX29
                    //Set I to the location of sprite for digit VX
                    I = (v[x] * 5);
                    break;
                }

                case 0x0030: {
                    //0xFX30
                    //(SCHIP) Set I to the location of hi-res sprite for digit VX
                    I = (80 + v[x] * 10);
                    break;
                }

                case 0x0033: {
                    //0xFX33
                    //Store BCD representation of VX to I, I+1, I+2
                    uint8_t n = v[x];
                    memory[I] = n / 100;
                    memory[(I + 1) & 0xFFF] = (n / 10) % 10;
                    memory[(I + 2) & 0xFFF] = (n % 100) % 10;
                    break;
                }

                case 0x0055: {
                    //0xFX55
                    //Store V0..VX into memory at location I
                    memcpy(memory + I, v, x + 1);

                    if(!loadStoreQuirk)
                        I += x + 1;

                    break;
                }

                case 0x0065: {
                    //0xFX65
                    //Store memory at location I into V0..VX
                    memcpy(v, memory + I, x + 1);

                    if(!loadStoreQuirk)
                        I += x + 1;

                    break;
                }

                case 0x0075: {
                    //0xFX75
                    //(SCHIP) Store V0..VX into user flags
                    memcpy(userFlags, v, x + 1);

                    break;
                }

                case 0x0085: {
                    //0xFX85
                    //(SCHIP) Store user flags into V0..VX
                    memcpy(v, userFlags, x + 1);

                    break;
                }

                default : {
                    //Unknown opcode
                    unknownOpcode(opcode);
                    break;
                }
            }
            break;
        }

        default: {
            //Unknown opcode
            unknownOpcode(opcode);
            break;
        }

    }

}

void Chip8::printInstruction(uint16_t op, uint16_t p) {

    std::cout << std::hex << std::setfill('0') << std::setw(4) << p;
    std::cout << ": ";

    switch(op & 0xF000) {

        case 0x0000 : {

            if((op & 0x00F0) == 0x00C0)
                std::cout << "SCD " << std::dec << (int) (opcode & 0x000F) << std::endl;
            else if ((op & 0x00F0) == 0x00D0)
                std::cout << "SCU " << std::dec << (int) (opcode & 0x000F) << std::endl;
            else switch(op & 0x00FF) {
                case 0x00E0 : {
                    std::cout << "CLS" << std::endl;
                    break;
                }
                case 0x00EE : {
                    std::cout << "RET" << std::endl;
                    break;
                }
                case 0x00FB: {
                    std::cout << "SCR 4" << std::endl;
                    break;
                }

                case 0x00FC: {
                    std::cout << "SCL 4" << std::endl;
                    break;
                }

                case 0x00FD: {
                    std::cout << "EXIT" << std::endl;
                    break;
                }

                case 0x00FE: {
                    std::cout << "LORES" << std::endl;
                    break;
                }

                case 0x00FF: {
                    std::cout << "HIRES" << std::endl;
                    break;
                }

            }
            break;
        }
        case 0x1000 : {
            std::cout << "JP " << std::hex << (int)(op & 0x0FFF) << std::endl;
            break;
        }
        case 0x2000 : {
            std::cout << "CALL " << std::hex << (int)(op & 0x0FFF) << std::endl;
            break;
        }
        case 0x3000 : {
            std::cout << "SE V" << std::hex << (int)((op & 0x0F00) >> 8) << ", ";
            std::cout << std::setw(2) << std::hex << (int)(op & 0x00FF) << std::endl;
            std::setw(0);
            break;
        }
        case 0x4000 : {
            std::cout << "SNE V" << std::hex << (int)((op & 0x0F00) >> 8) << ", ";
            std::cout << std::setw(2) << std::hex << (int)(op & 0x00FF) << std::endl;
            std::setw(0);
            break;
        }
        case 0x5000 : {
            switch(opcode & 0x000F) {
                case 0x0000 : {
                    std::cout << "SE V" << std::hex << (int)((op & 0x0F00) >> 8) << ", V" << (int)((op & 0x00F0) >> 4) << std::endl;
                    break;
                }
                case 0x0002 : {
                    std::cout << "SAVE V" << std::hex << (int)((op & 0x0F00) >> 8) << ", V" << (int)((op & 0x00F0) >> 4) << std::endl;
                    break;
                }
                case 0x0003 : {
                    std::cout << "LOAD V" << std::hex << (int)((op & 0x0F00) >> 8) << ", V" << (int)((op & 0x00F0) >> 4) << std::endl;
                    break;
                }
            }
            break;
        }
        case 0x6000 : {
            std::cout << "LD V" << std::hex << (int)((op & 0x0F00) >> 8) << ", " << (int)(op & 0x00FF) << std::endl;
            break;
        }
        case 0x7000 : {
            std::cout << "ADD V" << std::hex << (int)((op & 0x0F00) >> 8) << ", " << (int)(op & 0x00FF) << std::endl;
            break;
        }
        case 0x8000 : {
            switch(op & 0x000F) {
                case 0x0 : {
                    std::cout << "LD V" << std::hex << (int)((op & 0x0F00) >> 8) << ", V" << (int)((op & 0x00F0) >> 4) << std::endl;
                    break;
                }
                case 0x1 : {
                    std::cout << "OR V" << std::hex << (int)((op & 0x0F00) >> 8) << ", V" << (int)((op & 0x00F0) >> 4) << std::endl;
                    break;
                }
                case 0x2 : {
                    std::cout << "AND V" << std::hex << (int)((op & 0x0F00) >> 8) << ", V" << (int)((op & 0x00F0) >> 4) << std::endl;
                    break;
                }
                case 0x3 : {
                    std::cout << "XOR V" << std::hex << (int)((op & 0x0F00) >> 8) << ", V" << (int)((op & 0x00F0) >> 4) << std::endl;
                    break;
                }
                case 0x4 : {
                    std::cout << "ADD V" << std::hex << (int)((op & 0x0F00) >> 8) << ", V" << (int)((op & 0x00F0) >> 4) << std::endl;
                    break;
                }
                case 0x5 : {
                    std::cout << "SUB V" << std::hex << (int)((op & 0x0F00) >> 8) << ", V" << (int)((op & 0x00F0) >> 4) << std::endl;
                    break;
                }
                case 0x6 : {
                    std::cout << "SHR V" << std::hex << (int)((op & 0x0F00) >> 8) << ", V" << (int)((op & 0x00F0) >> 4) << std::endl;
                    break;
                }
                case 0x7 : {
                    std::cout << "SUBN V" << std::hex << (int)((op & 0x0F00) >> 8) << ", V" << (int)((op & 0x00F0) >> 4) << std::endl;
                    break;
                }
                case 0xE : {
                    std::cout << "SHL V" << std::hex << (int)((op & 0x0F00) >> 8) << ", V" << (int)((op & 0x00F0) >> 4) << std::endl;
                    break;
                }
            }

            break;
        }
        case 0x9000 : {
            std::cout << "SNE V" << std::hex << (int)((op & 0x0F00) >> 8) << ", V" << (int)((op & 0x00F0) >> 4) << std::endl;
            break;
        }
        case 0xA000 : {
            std::cout << "LD I, " << std::hex << (int)(op & 0x0FFF) << std::endl;
            break;
        }
        case 0xB000 : {
            std::cout << "JP V0, " << std::hex << (int)(op & 0x0FFF) << std::endl;
            break;
        }
        case 0xC000 : {
            std::cout << "RAND V" << std::hex << (int)((op & 0x0F00) >> 8) << ", " << (int)(op & 0x00FF) << std::endl;
            break;
        }
        case 0xD000 : {
            std::cout << "DRAW V" << std::hex << (int)((op&0x0F00)>>8) << ", V" << (int)((op&0x00F0)>>4) << ", " << (int)(op&0x000F) << std::endl;
            break;
        }
        case 0xE000 : {
            switch(op & 0x00FF) {
                case 0x009E : {
                    std::cout << "SKP V" << std::hex << (int)((op&0x0F00)>>8) << std::endl;
                    break;
                }
                case 0x00A1 : {
                    std::cout << "SKNP V" << std::hex << (int)((op&0x0F00)>>8) << std::endl;
                    break;
                }
            }
            break;
        }
        case 0xF000 : {
            switch(op & 0x00FF) {
                case 0x00 : {
                    std::cout << "JP " << std::hex << (int) ((memory[pc - 1] << 8) | memory[pc - 2]) << std::endl;
                    break;
                }
                case 0x01 : {
                    std::cout << "PLANE " << (int)((op & 0x0F00) >> 8) << std::endl;
                    break;
                }
                case 0x02 : {
                    std::cout << "AUDIO" << std::endl;
                    break;
                }
                case 0x07 : {
                    std::cout << "LD V" << (int)((op & 0x0F00) >> 8) << ", DT" << std::endl;
                    break;
                }
                case 0x0A : {
                    std::cout << "LD V" << (int)((op & 0x0F00) >> 8) << ", K" << std::endl;
                    break;
                }
                case 0x15 : {
                    std::cout << "LD DT, V" << (int)((op & 0x0F00) >> 8) << std::endl;
                    break;
                }
                case 0x18 : {
                    std::cout << "LD ST, V" << (int)((op & 0x0F00) >> 8) << std::endl;
                    break;
                }
                case 0x1E : {
                    std::cout << "ADD I, V" << (int)((op & 0x0F00) >> 8) << std::endl;
                    break;
                }
                case 0x29 : {
                    std::cout << "LD I, CHAR V" << (int)((op & 0x0F00) >> 8) << std::endl;
                    break;
                }
                case 0x30 : {
                    std::cout << "LD I, HIRES CHAR V" << (int)((op & 0x0F00) >> 8) << std::endl;
                    break;
                }
                case 0x33 : {
                    std::cout << "LD [I], BCD V" << (int)((op & 0x0F00) >> 8) << std::endl;
                    break;
                }
                case 0x55 : {
                    std::cout << "LD [I], V0..V" << (int)((op & 0x0F00) >> 8) << std::endl;
                    break;
                }
                case 0x65 : {
                    std::cout << "LD V0..V" << (int)((op & 0x0F00) >> 8) << ", [I]" << std::endl;
                    break;
                }
                case 0x75 : {
                    std::cout << "LD [I], V0..V" << (int)((op & 0x0F00) >> 8) << std::endl;
                    break;
                }
                case 0x85 : {
                    std::cout << "LD V0..V" << (int)((op & 0x0F00) >> 8) << ", [I]" << std::endl;
                    break;
                }
            }
            break;
        }
    }
}
