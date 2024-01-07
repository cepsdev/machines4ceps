











# Scenario


## title
📎 The ldi64, sti64 instructions: Move values in and out of the register SP and basic manipulations of the stack.



## When
The value 4 is on the compute stack, and the operations 

### asm
ldi32(four)

ldi64((SP-4))

stsi32

ldi64((SP-4))

sti64(SP)



are executed



And{}

## Then
The very first four bytes of the stack are 4;0;0;0.

### result
:heavy_check_mark: Passed







# Summary


## result
:heavy_check_mark: Passed







