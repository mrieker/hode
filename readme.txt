
        HODE 16-BIT TRANSISTOR COMPUTER

Website:  https://www.outerworldapps.com/hode

Uses a RaspberryPi for memory and IO (not needed for simulator).
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
                $ cd library
                $ make
                $ cd ..
                $ make
                $ ./runit.sh circus.hex < test01.circ   ## runs in simlator (x86 or arm, no hode hardware required)
                $ ./runhw.sh circus.hex < test01.circ   ## runs on hode hardware that this raspi is plugged into

    de0      - DE0-Nano FPGA implementation (in verilog)
               requires de0-nano board, raspberry pi, kicads/de0raspi interconnect board
               use quartus to compile verilog in the de0 directory and program the de0-nano
               populate interconnect board with two 2x20 connectors and 26 33-ohm resistors
               plug raspi into the raspi connector and de0-nano gpio 1 into the de0 connector
               should be able to run any hode programs such as:

                # desktop calculator
                $ cd crtl
                $ ./runhw.sh calc.hex

                # rolling lights
                $ cd driver
                $ ./raspictl rollights.hex

    driver   - raspi control program and hardware test programs
                $ make
                $ cd km
                $ make                      ## on raspi only
                $ sudo insmod enabtsc.ko    ## on raspi only

                    note:  hexfile.hex is a placeholder for an actual hex file (see crtl directory)

                $ ./raspictl.armv7l       hexfile.hex   ## runs hexfile.hex on attached hode hardware
                                                        ## runs until hexfile.hex program calls exit()

                $ ./raspictl.armv7l -nohw hexfile.hex   ## simulator (no hode hardware required)

                $ ./raspictl.x86_64 -nohw hexfile.hex   ## simulator running on x86

    freecads - case component drawings

    goodpcbs - good pcb files (routing completed, including netlist and parts list files)

    kicads   - other kicad files
                de0raspi - interconnect for de0 nano and raspberry pi
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

HOW TO BUILD

    Each board takes a little under 1.5A at 5V * 9 boards = 13.5A + raspi (2.5A) = 16A.
    Get 40 2x20 stackable connectors (and a couple extra).
    Get 9 1x6 pin right angle connectors for power.

    Make the 4 paddles by making the circuit boards, populate with 10K 1/8 watt resistors,
    and get an IOWarrior-56 module for each.  Put a 2x20 stackable connector on each.
    Run drivers/iow56list.armv7l on the raspi to get the serial number for each one at a time.
    Label them A, C, I, D with little stickers and put the serial numbers in netgen/iow56sns.si.

    When populating the main boards, start with the diodes as they are uniquely marked.  Of course
    make sure the bands on the diodes match the bands on the board.  Next do the 2.7K resistors as
    they are among the diodes.  Next come the 680 resistors, they go near the transistor collector
    leads and then the 1K resistors that go near the transistor emitter leads.  Then do the bypass
    capacitors that go next to the transistor collector and emitter leads.  Finally put in the
    transistors then the edge connectors.

    When soldering the 2x20 stackable connectors to the boards, make sure they are mounted tightly
    on the boards so they will mate up when stacking the boards.  Be very careful when pulling the
    boards apart that you do not bend the pins on the stackable connectors.  Rock them little by
    little until each one comes apart.

    Optionally make an ledboard.pcb and populate.  It can be used for diagnostics as it shows the
    state of the bus connector pins.  It can be plugged into the other boards one at a time or in
    any combination.  It is not required for testing as the testing is all done via the paddles, but
    can make it easier to track down any problems.  Be careful to note 3 positions are populated
    with 2N3906s instead of the usual 2N3904s and the LEDs are reversed, as the _IR_WE, _PSW_IECLR,
    _PSW_REB signals are active low.

    Make a seqboard.pcb board and populate with diodes, resistors, capacitors, transistors, LEDs,
    connectors.  Connect to a variable power supply and turn slowly up to 5V, it should draw under
    1.5A.  Plug in the 4 paddles (actually just need C and I paddles).  Run the 'drivers/seqtester
    -cpuhz 200 -loop' script to test it.  The paddles are limited to 200Hz so that's why the '-cpuhz
    200' option is needed.

    Make a rasboard.pcb board and populate as usual.  You will also need a 2x20 connector for the
    raspi and a raspi 3 or 4 to plug into it.  The little 4x4 connector pins near the raspi connector
    jumpers the 5V power from the board's main power to the raspi.  With the jumpers removed, the
    raspi must be powered via its usual USB connector, otherwise the raspi will be powered from the
    board's 5V supply.  Once constructed, leave the raspi off the board and test it with a variable
    power supply like with the seqboard and see that it takes under 1.5A.  Then it should be ok to
    plug the raspi in.  You can use 7/16" spacers on 3/4" 4-40 screws to hold the raspi in place.
    Plug all 4 paddles into the edge of the rasboard and the USB cables into the raspi.  Run the
    'drivers/rastester.sh -cpuhz 200 -loop' script to test it.

    Plug the seqboard and rasboard together.  They should be oriented so the power connectors are at
    same position.  Run the 'drivers/raseqtest.sh -cpuhz 200 -loop' script to test it.  It assumes
    the 4 paddles are plugged in, if not, use the -nopads option on the command line and it will test
    as much as it can without it.  With -nopads, the two boards should run fast, so try leaving off the
    '-cpuhz 200' option.

    Make aluboards one at a time.  Jumper one as hi-byte and use 'drivers/alutester.sh -cpuhz 200 -loop
    hi' to test it with the 4 paddles.  Likewise make a lo-byte board and test it.

    Stack both aluboards and plug into the seqboard/rasboard stack.  Test with all 4 paddles plugged in
    to the stack and the raspi using the 'drivers/raseqtest.sh -alu -cpuhz 200 -loop' command.

    Finally come the register boards.  They can be tested individually with the 'drivers/regtester.sh 
    -cpuhz 200 -loop <nn>' script.  <nn> is 01, 23, 45, 67 depending on how the register board is
    jumpered.  When the board tests ok with the regtester script, it can be added to the seq/ras/alu
    stack and tested with 'drivers/raseqtest.sh -alu -cpuhz 200 -loop -reg<nn>' command.  Use a
    -reg01, -reg23, -reg45, -reg67 option for each register board you have put in the stack.

    When all 8 boards are completed, they can be tested at full speed with the 'drivers/raspictl.armv7l
    -randmem -mintimes' command.  It should run for days and days.  You can experiment with the -cpuhz
    option to specify different clocking frequencies.

    Then you can go to the crtl directory and run a program with the ./runhw.sh script, such as
    './runhw.sh printpi.hex' or './runhw.sh circus.hex < test01.circ'.

    Note that it doesn't matter what order the boards are stacked in, electrically speaking.  You can
    shuffle them around for testing and debugging.  But it is best to put the sequencer board on the
    front when you get it all working so you can see the blinking state LEDs.  If you make the optional
    LED board, it goes in front of the sequencer board and does not block any of the sequencer board's
    LEDs.

