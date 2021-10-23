# CHIP-8 emulator
CHIP-8, SUPERCHIP and XO-CHIP emulator written in C++.

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
`-c cycles` : Target emulated instructions per second.  


## Key layout
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

## Quirks
This emulator supports multiple CHIP-8 extensions, however they are not fully backwards-compatible with each other.  
Certain programs will expect specific behaviors from certain instructions.

At the moment, these "quirks" cannot be enabled or disabled individually.  
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
