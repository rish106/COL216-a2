# MIPS Processor
Simulates the state of registers and changes in memory after every clock cycle of a processor based on MIPS architecture executing a program. Created as part of course COL216 (Computer Architecture).

## Files
- `5stage.hpp` contains the implementation of a pipelined processor without bypassing.
- `5stage_bypass.hpp` contains the implementation of a pipelined processor with bypassing.
- `sample.asm` contains a sample mips program that can be run on the processor.
- `input.asm` should contain the mips program for which the simulation needs to be run.

## Run the simulation
1. Download the [boost library](https://github.com/boostorg/boost) on your device

2. Run the followind command to compile the C++ files
```
make compile
```

3. Put your MIPS assembly code in a new file named `input.asm` in the same directory as rest of the code. 

3. Run the `make` command for 5 stage without bypassing or with bypassing simulation
```
make run_5stage
make run_5stage_bypass
```
