









# opcodes


halt

noop

ldi32

ldsi32

ldi64

lddbl

sti32

stsi32

sri64

stdbl

ldptr

stptr

lea

addi32

addi64

adddbl

subi32

subi64

subdbl

buc

beq

bneq

blt

blteq

bgt

bgteq

bgteqzeroi32

blteqzeroi32

bltzeroi32

bzeroi32

bnzeroi32

bzeroi64

bnzeroi64

bzerodbl

bnzerodbl

call

ret

swp

andni32

andni64

andi32

andi64

ori32

ori64

noti32

noti64

xori32

xori64

duptopi32

muli32

muli64

muldbl

divi32

divi64

divdbl

remi32

remi64

lti32

lti64

ltdbl

lteqi32

lteqi64

lteqdbl

gti32

gti64

gtdbl

gteqi32

gteqi64

gteqdbl

(eqi32:eqi64)

eqdbl

cpysi32

wrsi32

setframe

popi32

pushi32





# Scenario


## title
📎 Arithmetic: addi32



## Given
### text
asm{addi32 halt}





## And
### vm
text{13 0 0 0 0 0 0 0}

compute_stack{1 1}





When{}

## Then
### result
:heavy_check_mark: Passed







# Scenario


## title
📎 Arithmetic: addi64



## Given
### text
asm{addi64 halt}





## And
### vm
text{14 0 0 0 0 0 0 0}

compute_stack{1 0 1 0}





When{}

## Then
### result
:heavy_check_mark: Passed







# Scenario


## title
📎 Arithmetic: adddbl



## Given
### text
asm{adddbl halt}





## And
### vm
text{15 0 0 0 0 0 0 0}

compute_stack{1 1}





When{}

## Then
### result
:heavy_check_mark: Passed







# Scenario


## title
📎 Arithmetic: subi32



## Given
### text
asm{subi32 halt}





## And
### vm
text{16 0 0 0 0 0 0 0}

compute_stack{2 3}





When{}

## Then
### result
:heavy_check_mark: Passed







# Scenario


## title
📎 Arithmetic: subi32



## Given
### text
asm{subi32 halt}





## And
### vm
text{16 0 0 0 0 0 0 0}

compute_stack{3 2}





When{}

## Then
### result
:heavy_check_mark: Passed







# Scenario


## title
📎 Arithmetic: subi64



## Given
### text
asm{subi64 halt}





## And
### vm
text{17 0 0 0 0 0 0 0}

compute_stack{2 0 3 0}





When{}

## Then
### result
:heavy_check_mark: Passed







# Scenario


## title
📎 Arithmetic: subi64



## Given
### text
asm{subdbl halt}





## And
### vm
text{18 0 0 0 0 0 0 0}

compute_stack{2 3}





When{}

## Then
### result
:heavy_check_mark: Passed







# Scenario


## title
📎 Arithmetic: subi64



## Given
### text
asm{subdbl halt}





## And
### vm
text{18 0 0 0 0 0 0 0}

compute_stack{3 2}





When{}

## Then
### result
:heavy_check_mark: Passed









