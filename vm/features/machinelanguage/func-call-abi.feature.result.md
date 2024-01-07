











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







# Scenario


## title
📎 Building a stack frame.



## When
The caller pushes B=20 and A=29 onto the stack and reserves spece for the result and makes a call to AddTwoNumbers.



## Then
After moving the result onto the computation stack, the computation stack contains exactly four bytes, the values of which are 42;0;0;0.The stack is empty.

### result
:heavy_check_mark: Passed







# Summary


## result
:heavy_check_mark: Passed



## result
:heavy_check_mark: Passed







