









opcodes{

halt noop ldi32 ldsi32 ldi64 lddbl sti32 stsi32 sri64 stdbl ldptr stptr lea addi32 addi64 adddbl subi32 subi64 subdbl buc beq bneq blt blteq bgt bgteq bgteqzeroi32 blteqzeroi32 bltzeroi32 bzeroi32 bnzeroi32 bzeroi64 bnzeroi64 bzerodbl bnzerodbl call ret swp andni32 andni64 andi32 andi64 ori32 ori64 noti32 noti64 xori32 xori64 duptopi32 muli32 muli64 muldbl divi32 divi64 divdbl remi32 remi64 lti32 lti64 ltdbl lteqi32 lteqi64 lteqdbl gti32 gti64 gtdbl gteqi32 gteqi64 gteqdbl eqi32 eqi64 eqdbl cpysi32 wrsi32 setframe popi32 pushi32}

# Scenario


## title
📎 Basic objects controlled by the Oblectamenta-VM.



## Given
A stack



## And
statically allocated data 



## And
code (text segment) 



## And
 the stack where computations are performed



## And
 the register file



## Then
These pieces form a complete environment for running Oblectamenta machine code

### result
:heavy_check_mark: Passed







# Scenario


## title
📎 A VM contains code, data, stack, a compute stack and a registers file.



## Given
An empty stack

stack{}



## And
statically allocated data with no entries

data{}



## And
code (text segment) with no code

text{}



## And
 the stack where computations are performed without any data

compute_stack{}



## And
 the register file

### registers
CSP{0}

FP{0}

PC{0}

SP{4096}





## When
 A default constructed VM



## Then
 The constructed VM contains stack,data, code, and the compute stack which are all empty.

### result
:heavy_check_mark: Passed







# Scenario


## title
📎 The data, stack and compute stack of a VM can be initialized with data.



## Given
A stack initialized with data

stack{3 0 0 0 2 0 0 0 1 0 0 0}



## And
non empty statically allocated data

data{9 0 0 0 4 0 0 0 1 0 0 0}



## And
code (text segment) with no code

text{}



## And
compute stack with data

compute_stack{81 0 0 0 16 0 0 0 1 0 0 0}



## When
 Creating a VM with initialized stack, data and compute stack



## Then
 The created VM's stack, data, and compute stack are initialized with the given data.

### result
:heavy_check_mark: Passed







# Scenario


## title
📎 Data entries can contain labels.



## Given
non empty statically allocated data with labels

data{l1 9 0 0 0 4 0 0 0 l2 1 0 0 0 l3 103 97 114 103 97 109 101 108}



## When
 Creating a VM with initialized data 



## Then
 The created VM's stack, data, and compute stack are initialized with the given data.

### result
:heavy_check_mark: Passed







# Scenario


## title
📎 The text segment contains a sequence of opcodes (machine language).
Oblectamenta's assembler translates assembler code, i.e. symbols
representing opcodes, into machine language.
Assembler code has to be placed within an asm-struct.



## Given
code (text segment) with 5 noops

### text
asm{noop noop noop noop noop}





## When
 Creating a VM with the code.

### vm
text{1 0 0 0 1 0 0 0 1 0 0 0 1 0 0 0 1 0 0 0}





## Then
 The created VM's text segment contains the machine code version of the code.

### result
:heavy_check_mark: Passed







# Scenario


## title
📎 Running the program stored in a VM changes the registers.



## Given
code (text segment) with 5 noops

### text
asm{noop noop noop noop noop halt}





## And
 Creating a VM with the code.

### vm
text{1 0 0 0 1 0 0 0 1 0 0 0 1 0 0 0 1 0 0 0 0 0 0 0}





## When
 Running the VM.



## Then
 The PC register points to the halt instruction which terminated the execution.

### result
:heavy_check_mark: Passed







==================================== SUMMARY ======================================


# Summary


## result
:heavy_check_mark: Passed



## result
:heavy_check_mark: Passed



## result
:heavy_check_mark: Passed



## result
:heavy_check_mark: Passed



## result
:heavy_check_mark: Passed



## result
:heavy_check_mark: Passed







