# CHIP-8 emulator
CHIP-8, SUPERCHIP and XO-CHIP emulator written in C++.

![civiliz8n](https://user-images.githubusercontent.com/68333938/138595405-00fb069d-9c93-46d9-bff2-e961dcef3aac.png?v=1&s=10)


This emulator can run regular CHIP-8 programs, as well as SUPERCHIP and XO-CHIP (Octo) programs.  
It relies on SDL2 for the display and input handling.

At the moment, the emulator lacks a GUI and doesn't emulate sound.

This was developed for fun as a hobby project, and to better understand how emulators work.

## Usage
To launch the emulator, drag and drop a rom file onto the application executable.

If you need to change certain settings, you can launch the emulator from a command prompt :  
`chip-8 rom_file [options]`  

Where `[options]` can include :  
`-k [azerty qwerty]` : Selects the keyboard layout.  
`-m [auto chip8 schip xochip]` : Selects the machine type, which toggles specific emulation quirks.  
`-c cycles` : Emulated instructions per frame.  
`-p palette_file` : Hex palette file to use.  
`-t cycles` : Enable headless testing mode.  

### Palette files
You can use palette files with this emulator.
This is particularly useful with XO-CHIP games, which can display up to 4 different colors.

Palette files are plain text files containing hex values for each color :
```
141421
754D27
738C3A
E2CB8A

```

The top color is the background color, followed by fill colors 1 and 2 and the blend color.

### Key layout
The key layout is as follows (on a QWERTY keyboard) :
```
1 2 3 4
Q W E R
A S D F
Z X C V
```

There are also certain keys you can use while the emulator is running :
| Key               | Function
|:------------------|:-----------------
| Escape            | Quit the emulator
| F2                | Reset the program
| F5                | Decrease emulation speed
| F6                | Increase emulation speed
| P                 | Pause / resume emulation
| O                 | Execute and display next instruction (step)

### Headless testing mode
This mode is intended to help with automated testing.  
When it's enabled, the emulator will run for a set number of cycles before exiting and printing the screen output to the terminal.  
The delay timer will also be decremented every cycle instead of every frame, to force fixed-framerate games to update.  
You cannot automate key presses in headless mode at the moment.  

To enable this mode, use the following command line option :  
`-t cycles` where `cycles` is the number of cycles you want to run.

## Quirks
Multiple CHIP-8 extensions are supported, however they are not fully backwards-compatible with each other.  
Certain programs will expect a specific behavior from certain instructions.

At the moment, these behavior "quirks" cannot be enabled or disabled individually.  
Each machine type toggles specific quirks which should correspond to the behavior of said machine.

| Quirks                                  | CHIP-8  | SUPERCHIP | XO-CHIP |
|:----------------------------------------|:--------|:----------|:--------|
| FX55 and FX65 don't increment I         |         |     ✓     |         |
| 8XY6 and 8XYE don't shift VY            |         |     ✓     |         |
| Switching resolutions clears the screen |         |           |    ✓    |
| Sprites wrap around screen boundaries   |    ✓    |     ✓     |         |

## Compatibility
The emulator is accurate enough to run most CHIP-8, SUPERCHIP and XO-CHIP programs.  

It passes the following test roms :
- BC_TEST by BestCoder (SUPERCHIP mode)
- SCTEST by Sergey Naydenov (SUPERCHIP mode)
- test_opcode by corax89 (all modes)

However, certain games do not work properly :  
| Games                 | Issues                                                       |
|:----------------------|:-------------------------------------------------------------|
| Skyward               | Requires loadStoreQuirk to be enabled, glitches near the end |
| Super Neat Boy        | Sprites wrap around the screen, otherwise no leaf counter    |

## Issues
- The program is singlethreaded, meaning rendering can slow the emulation down.
- Certain elements disappear when sprite wrapping is disabled (e.g. super neat boy leaf counter).
- main.cpp contains SDL2 rendering code, which should be placed in a separate file.
- Certain C functions should be replaced with equivalent C++ ones.

## Licencing
I've licenced the emulator under the terms of the MIT Licence
