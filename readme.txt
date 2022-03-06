
        HODE 16-BIT TRANSISTOR COMPUTER

Website:  https://www.outerworldapps.com/hode

Requires a Linux computer (x86 or raspi) to build software.
KiCad used to do PCB layouts.
Must have GNU C++ compiler, Java 8 or better dev kit.

Directories:

    asm      - cross assembler and linker
                $ make

    cc       - cross compiler
                $ make

    crtl     - C runtime library and example programs
                # make asm, cc, driver first
                $ make
                $ ./runit.sh circus.hex     ## runs in simlator (x86 or arm, no hode hardware required)
                $ ./runhw.sh circus.hex     ## runs on hode hardware that this raspi is plugged into

    driver   - raspi control program and hardware test programs
                $ make
                $ cd km
                $ make                      ## on raspi only
                $ sudo insmod enabtsc.ko    ## on raspi only

                $ ./raspictl.armv7l       hexfile.hex   ## runs hexfile.hex on attached hode hardware
                                                        ## runs until hexfile.hex program calls exit()

                $ ./raspictl.armv7l -nohw hexfile.hex   ## simulator (no hode hardware required)

                $ ./raspictl.x86_64 -nohw hexfile.hex   ## simulator running on x86

    freecads - case component drawings

    goodpcbs - good pcb files (routing completed, including netlist and parts list files)

    kicads   - other kicad files
                IOW56Paddle - IO-Warrior-56 paddles for board-level testing
                PowerDist - power distribution board - connects edge power to power supply

    modules  - board modules (verilog-like code)
                # make netgen first
                $ ./testmaster.sh   ## runs regression test scripts
                $ ./pcball.sh       ## generate raw .pcb files, need trace routing via kicad

    netgen   - java program to compile modules for simulation and pcb generation
                # make sure jninc_x86_64 or jninc_armv7l points to directory where jni.h is located
                $ make

For those makefiles that create objects and executables, they will create two versions,
one for x86_64 and one for armv7l, depending on which computer type they are run on.
Typically this is just done for the driver directory, once on a PC (creating the .x86_64
files) and once on a raspi (creating the .armv7l files).  The other directories are
typically only done on the PC but can be done on a raspi (either instead of or as well as).

