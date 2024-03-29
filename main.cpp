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

#include <iostream>
#include <cstdio>
#include <SDL2/SDL.h>
#include <ctime>
#include <cmath>
#include <string>
#include <cstring>
#include <unistd.h>

#include "chip8.hpp"

#define CYCLES_STEP 5
#define CYCLES_DEFAULT 200

#define ARG_CYCLES "-c"
#define ARG_MACHINE "-m"
#define ARG_KEYBOARD "-k"
#define ARG_PALETTE "-p"
#define ARG_TEST "-t"
#define ARGLEN 2
#define ARG_AUTO "auto"
#define ARG_CHIP8 "chip8"
#define ARG_SCHIP "schip"
#define ARG_XOCHIP "xochip"
#define ARG_SKYWARD "skyward"
#define ARG_QWERTY "qwerty"
#define ARG_AZERTY "azerty"

#define MACHINE_AUTO 0
#define MACHINE_CHIP8 1
#define MACHINE_SCHIP 2
#define MACHINE_XOCHIP 3
#define MACHINE_SKYWARD 4

#define KB_QWERTY 0
#define KB_AZERTY 1

#define KB_DEFAULT KB_QWERTY
#define MACHINE_DEFAULT MACHINE_AUTO

using namespace std;

int main(int argc, char** argv)
{
    SDL_Keycode keyBindings[] = {
        //QWERTY
        SDLK_x, SDLK_1, SDLK_2, SDLK_3,
        SDLK_q, SDLK_w, SDLK_e, SDLK_a,
        SDLK_s, SDLK_d, SDLK_z, SDLK_c,
        SDLK_4, SDLK_r, SDLK_f, SDLK_v,

        //AZERTY
        SDLK_x, SDLK_1, SDLK_2, SDLK_3,
        SDLK_a, SDLK_z, SDLK_e, SDLK_q,
        SDLK_s, SDLK_d, SDLK_w, SDLK_c,
        SDLK_4, SDLK_r, SDLK_f, SDLK_v
    };

    SDL_Keycode keyShortcuts[] = {
        0, 0, 0, 0,
        0, SDLK_UP, SDLK_SPACE, SDLK_LEFT,
        SDLK_DOWN, SDLK_RIGHT, 0, 0,
        0, 0, 0, 0,
    };

    //Emulation properties
    uint8_t keySet = KB_DEFAULT;  // 0-QWERTY 1-AZERTY
    bool paused = false;          // Emulation paused
    int machine = MACHINE_DEFAULT;// 0: auto 1: chip8 2:schip 3:xochip
    int testCycles = 0;           // Run a set number of cycles for testing

    //Display argument help
    if(argc < 2) {
        cout << "usage: chip-8 rom_file [options]" << endl;
        cout << " options :" << endl;
        cout << "  -k [azerty qwerty]    keyboard layout" << endl;
        cout << "  -m [auto chip8 schip xochip]    machine type" << endl;
        cout << "  -c cycles    instructions per frame" << endl;
        cout << " testing : " << endl;
        cout << "  -t cycles    run headless for n cycles and exit" << endl;

        return 0;
    }

    //Create CHIP-8 instance
    Chip8* chip8 = new Chip8();

    //Read command line arguments
    int i;
    for(i = 1 ; i < argc ; i++) {
        //Number of cycles
        if(strncmp(ARG_CYCLES, argv[i], ARGLEN) == 0) {
            int newCycles;

            if(argc <= i+1) {
                cout << "ERROR : cycles value not provided" << endl;
                return 1;
            }

            if(sscanf(argv[i+1], "%d", &newCycles) != 1) {
                cout << "ERROR : cycles argument must be an integer number" << endl;
                return 1;
            }
            else if(newCycles <= 0) {
                cout << "ERROR : cycles argument must be greater than 0" << endl;
                return 1;
            }
            else {
                chip8->tickRate = newCycles;
            }
        }

        //Emulated machine
        if(strncmp(ARG_MACHINE, argv[i], ARGLEN) == 0) {
            char *values[] = {(char*)ARG_AUTO, (char*)ARG_CHIP8, (char*)ARG_SCHIP, (char*)ARG_XOCHIP, (char*)ARG_SKYWARD};
            int machines[] = {MACHINE_AUTO, MACHINE_CHIP8, MACHINE_SCHIP, MACHINE_XOCHIP, MACHINE_SKYWARD};
            bool machineFound = false;

            if(argc <= i+1) {
                cout << "ERROR : machine type not provided" << endl;
                return 1;
            }

            for(int j = 0 ; j < sizeof(values) ; j++) {
                if(strncmp(values[j], argv[i+1], strlen(values[j])) == 0) {
                    machine = machines[j];
                    machineFound = true;
                    break;
                }
            }

            if(!machineFound){
                cout << "Unknown machine type " << argv[i+1] << endl;
                return 1;
            }
        }

        //Keyboard layout
        if(strncmp(ARG_KEYBOARD, argv[i], ARGLEN) == 0) {
            char *values[] = {(char*)ARG_QWERTY, (char*)ARG_AZERTY};
            int keysets[] = {0, 1};
            bool keysetFound = false;

            if(argc <= i+1) {
                cout << "ERROR : keyboard layout not provided" << endl;
                return 1;
            }

            for(int j = 0 ; j < 2 ; j++) {
                if(strncmp(values[j], argv[i+1], strlen(values[j])) == 0) {
                    keySet = keysets[j];
                    keysetFound = true;
                    break;
                }
            }

            if(!keysetFound){
                cout << "Unknown keyboard layout " << argv[i+1] << endl;
                return 1;
            }
        }

        if(strncmp(ARG_PALETTE, argv[i], ARGLEN) == 0) {
            if(argc <= i+1) {
                cout << "ERROR : palette file not provided" << endl;
                return 1;
            }

            if(chip8->loadPalette(argv[i+1]) != 0) {
                return 1;
            }
        }

        // Test mode
        if(strncmp(ARG_TEST, argv[i], ARGLEN) == 0) {

            if(argc <= i+1) {
                cout << "ERROR : cycles value not provided" << endl;
                return 1;
            }

            if(sscanf(argv[i+1], "%d", &testCycles) != 1) {
                cout << "ERROR : cycles argument must be an integer number" << endl;
                return 1;
            }
        }
    }

    switch(machine) {
        default: break;

        case MACHINE_AUTO :
        case MACHINE_CHIP8 : {
            //CHIP-8
            chip8->loadStoreQuirk = false;
            chip8->shiftQuirk = false;
            chip8->wrapQuirk = true;
            break;
        }

        case MACHINE_SCHIP : {
            //SCHIP
            chip8->loadStoreQuirk = true;
            chip8->shiftQuirk = true;
            chip8->hiresClearQuirk = false;
            chip8->wrapQuirk = true;
            break;
        }

        case MACHINE_XOCHIP : {
            //XO-CHIP
            chip8->loadStoreQuirk = false;
            chip8->shiftQuirk = false;
            chip8->hiresClearQuirk = true;
            chip8->wrapQuirk = false;
            break;
        }

        case MACHINE_SKYWARD : {
            //XO-CHIP with load quirk enabled
            //Fixes Skyward
            chip8->loadStoreQuirk = true;
            chip8->shiftQuirk = false;
            chip8->hiresClearQuirk = true;
            chip8->wrapQuirk = false;
            break;
        }
    }


    bool running = false;

    //Load ROM
    if(chip8->loadROM(argv[1]) == 0) {
        running = true;
    } else {
        return 1;
    }

    // Headless testing mode
    // Execute set number of cycles and exit
    if (testCycles > 0) {

        cout << "Emulating " << (int)testCycles << " cycles" << endl;

        for (int i = 0 ; i < testCycles ; i++) {

            chip8->emulateInstruction();

            // Force timer update
            chip8 -> updateTimers();
        }

        // Print video output to terminal
        cout << endl << "RESULTS" << endl;

        for (int p = 0 ; p < 2 ; p++) {
            cout << "Plane " << (int)p << " :" << endl;

            for (int i = 0 ; i < SCHIP_H ; i++) {
                for (int j = 0 ; j < SCHIP_W ; j++) {
                    cout << ((chip8->gfx[p][j + i * SCHIP_W] > 0) ? "0" : ".");
                }

                cout << endl;
            }

        }

        return 0;

    }

    //Window title
    char cyclesBuff[256];
    snprintf(cyclesBuff, 256, "%i", chip8->tickRate);

    string title = "CHIP-8 Interpreter - " + (string)cyclesBuff + " instructions per frame";

    cout << "Program started" << endl;

    srand(time(NULL));

    if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        cout << "Error initializing SDL" << endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 512, SDL_WINDOW_OPENGL);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Event event;
    SDL_Rect rect;

	//Get window size
	SDL_GetWindowSize(window, &(rect.w), &(rect.h));

	rect.w /= SCHIP_W;
	rect.h /= SCHIP_H;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_RenderPresent(renderer);

    Uint32 lastTime, currentTime;

    if(window == NULL) {
        cout << "Could not initialize window" << endl;
        return 1;
    }

    lastTime = SDL_GetTicks();


    while(running) {

        //Poll events
        while(SDL_PollEvent(&event)) {

            switch(event.type){

                case SDL_QUIT: {
                    running = false;
                    break;
                }

                case SDL_KEYDOWN: {
                    uint16_t i;
                    SDL_Keycode sdlSym = event.key.keysym.sym;

                    for(i = 0 ; i < 16 ; i++) {
                        if(sdlSym == keyBindings[i + 16*keySet] || sdlSym == keyShortcuts[i]) {
                            chip8 -> keys[i] = true;
                            break;
                        }
                    }

                    switch(event.key.keysym.sym) {

                        case SDLK_ESCAPE: {
                            running = false;
                            break;
                        }

                        case SDLK_F2: {
                            chip8 -> initialize();
                            break;
                        }

                        case SDLK_F5: {
                            if(chip8->tickRate > CYCLES_STEP)
                                chip8->tickRate -= CYCLES_STEP;

                            snprintf(cyclesBuff, 256, "%i", chip8->tickRate);
                            title = "CHIP-8 Interpreter - " + (string)cyclesBuff + " instructions per frame";
                            SDL_SetWindowTitle(window, title.c_str());
                            break;
                        }

                        case SDLK_F6: {
                            chip8->tickRate += CYCLES_STEP;

                            snprintf(cyclesBuff, 256, "%i", chip8->tickRate);
                            title = "CHIP-8 Interpreter - " + (string)cyclesBuff + " instructions per frame";
                            SDL_SetWindowTitle(window, title.c_str());
                            break;
                        }


                        case SDLK_p: {
                            paused ^= 1;
                            break;
                        }

                        case SDLK_o: {
                            uint16_t pc = chip8->pc;
                            chip8->emulateInstruction();
                            chip8->printInstruction(chip8->opcode, pc);
                            break;
                        }

                        default : break;
                    }

                    break;
                }

                case SDL_KEYUP: {
                    uint16_t i;
                    SDL_Keycode sdlSym = event.key.keysym.sym;

                    for(i = 0 ; i < 16 ; i++) {
                        if(sdlSym == keyBindings[i + 16*keySet] || sdlSym == keyShortcuts[i]) {
                            chip8->keys[i] = false;

                            if(chip8->waiting) {
                                chip8->v[chip8->waitRegister] = i;
                                chip8->waiting = false;
                            }

                            break;
                        }                            
                    }

                    break;
                }

                default :
                    break;
            }
        }

        //Emulate cycles
        if(!paused && !chip8->stopped){

            for(int i = 0 ; i < chip8->tickRate ; i++)
                chip8 -> emulateInstruction();

        }

        //Update Chip-8 timers
        chip8 -> updateTimers();


        //Update display

        //Clear surface
        SDL_SetRenderDrawColor(renderer, chip8->palette[0][0], chip8->palette[0][1], chip8->palette[0][2], 255);
        SDL_RenderClear(renderer);

        //Color (XO-CHIP)
        uint8_t col;

        //Memory location
        uint16_t memLoc = 0;

        //Draw pixels
        for(uint8_t y = 0 ; y < SCHIP_H ; y++) {

            for(uint8_t x = 0 ; x < SCHIP_W ; x++) {

                if(chip8->gfx[0][memLoc] != 0 || chip8->gfx[1][memLoc] != 0) {

                    //Use full palette on XOCHIP, only two colors on other machines
                    if(machine != MACHINE_CHIP8 && machine != MACHINE_SCHIP)
                        col = ((chip8->gfx[1][memLoc] << 1) + chip8->gfx[0][memLoc]);
                    else
                        col = ((chip8->gfx[1][memLoc] << 1) + chip8->gfx[0][memLoc] > 0) ? 3 : 0;

                    SDL_SetRenderDrawColor(renderer, chip8->palette[col][0], chip8->palette[col][1], chip8->palette[col][2], 255);

                    rect.x = x * rect.w;
                    rect.y = y * rect.h;

                    SDL_RenderFillRect(renderer, &rect);
                }



                memLoc ++;

            }

        }

        //Update display
        SDL_RenderPresent(renderer);
    }


    //60fps delay
    currentTime = SDL_GetTicks();

    if(currentTime - lastTime < 1000/60) {
        SDL_Delay(1000/60 - (currentTime - lastTime));
    }

    lastTime = currentTime;


    SDL_DestroyWindow(window);
    SDL_Quit();


    return 0;
}
