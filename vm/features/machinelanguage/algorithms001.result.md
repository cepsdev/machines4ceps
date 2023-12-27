









opcodes{

halt noop ldi32 ldsi32 ldi64 lddbl sti32 stsi32 sri64 stdbl ldptr stptr lea addi32 addi64 adddbl subi32 subi64 subdbl buc beq bneq blt blteq bgt bgteq bgteqzeroi32 blteqzeroi32 bltzeroi32 bzeroi32 bnzeroi32 bzeroi64 bnzeroi64 bzerodbl bnzerodbl call ret swp andni32 andni64 andi32 andi64 ori32 ori64 noti32 noti64 xori32 xori64 duptopi32 muli32 muli64 muldbl divi32 divi64 divdbl remi32 remi64 lti32 lti64 ltdbl lteqi32 lteqi64 lteqdbl gti32 gti64 gtdbl gteqi32 gteqi64 gteqdbl eqi32 eqi64 eqdbl cpysi32 wrsi32 setframe popi32 pushi32}



# Scenario


## title
📎 INSERTION-SORT (ASCENDING)



Given{}

When{}

## Then
### result
:heavy_check_mark: Passed







# Scenario


## title
📎 INSERTION-SORT (DESCENDING)



Given{}

When{}

## Then
### result
:heavy_check_mark: Passed







# Scenario


## title
📎 SUM-ARRAY



Given{}

When{}

## vm
stack{}

data{result 55 0 0 0 zero 0 0 0 0 one 1 0 0 0 int_width 4 0 0 0 n 10 0 0 0 array 1 0 0 0 2 0 0 0 3 0 0 0 4 0 0 0 5 0 0 0 6 0 0 0 7 0 0 0 8 0 0 0 9 0 0 0 10 0 0 0 i 10 0 0 0}

text{2 0 0 0 4 0 0 0 0 0 0 0 6 0 0 0 60 0 0 0 0 0 0 0 2 0 0 0 4 0 0 0 0 0 0 0 6 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 60 0 0 0 0 0 0 0 2 0 0 0 16 0 0 0 0 0 0 0 63 0 0 0 29 0 0 0 220 0 0 0 0 0 0 0 12 0 0 0 20 0 0 0 0 0 0 0 2 0 0 0 60 0 0 0 0 0 0 0 2 0 0 0 12 0 0 0 0 0 0 0 49 0 0 0 80 0 0 0 14 0 0 0 3 0 0 0 2 0 0 0 0 0 0 0 0 0 0 0 13 0 0 0 6 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 60 0 0 0 0 0 0 0 2 0 0 0 8 0 0 0 0 0 0 0 13 0 0 0 6 0 0 0 60 0 0 0 0 0 0 0 19 0 0 0 48 0 0 0 0 0 0 0 0 0 0 0}

compute_stack{}

### registers
CSP{0}

FP{0}

PC{220}

SP{4096}





## Then
### result
:heavy_check_mark: Passed







# Scenario


## title
📎 MAX-ELEMENT



Given{}

When{}

## Then
### result
:heavy_check_mark: Passed







# Scenario


## title
📎 MAX-HEAPIFY



Given{}

When{}

## Then
### vm
stack{}

data{A 16 0 0 0 14 0 0 0 10 0 0 0 8 0 0 0 7 0 0 0 9 0 0 0 3 0 0 0 2 0 0 0 4 0 0 0 1 0 0 0 result 0 0 0 0 one 1 0 0 0 two 2 0 0 0 four 4 0 0 0 0 0 0 0 n 10 0 0 0 i 8 0 0 0 heap_size 10 0 0 0 l 17 0 0 0 r 18 0 0 0 largest 8 0 0 0}

text{2 0 0 0 64 0 0 0 0 0 0 0 2 0 0 0 48 0 0 0 0 0 0 0 49 0 0 0 2 0 0 0 44 0 0 0 0 0 0 0 13 0 0 0 6 0 0 0 72 0 0 0 0 0 0 0 2 0 0 0 64 0 0 0 0 0 0 0 2 0 0 0 48 0 0 0 0 0 0 0 49 0 0 0 2 0 0 0 48 0 0 0 0 0 0 0 13 0 0 0 6 0 0 0 76 0 0 0 0 0 0 0 2 0 0 0 72 0 0 0 0 0 0 0 2 0 0 0 68 0 0 0 0 0 0 0 60 0 0 0 30 0 0 0 56 1 0 0 0 0 0 0 2 0 0 0 64 0 0 0 0 0 0 0 2 0 0 0 52 0 0 0 0 0 0 0 49 0 0 0 80 0 0 0 12 0 0 0 0 0 0 0 0 0 0 0 14 0 0 0 3 0 0 0 2 0 0 0 72 0 0 0 0 0 0 0 2 0 0 0 52 0 0 0 0 0 0 0 49 0 0 0 80 0 0 0 12 0 0 0 0 0 0 0 0 0 0 0 14 0 0 0 3 0 0 0 60 0 0 0 30 0 0 0 56 1 0 0 0 0 0 0 2 0 0 0 72 0 0 0 0 0 0 0 12 0 0 0 80 0 0 0 0 0 0 0 7 0 0 0 19 0 0 0 84 1 0 0 0 0 0 0 2 0 0 0 64 0 0 0 0 0 0 0 12 0 0 0 80 0 0 0 0 0 0 0 7 0 0 0 2 0 0 0 76 0 0 0 0 0 0 0 2 0 0 0 68 0 0 0 0 0 0 0 66 0 0 0 29 0 0 0 16 2 0 0 0 0 0 0 12 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 76 0 0 0 0 0 0 0 2 0 0 0 52 0 0 0 0 0 0 0 49 0 0 0 80 0 0 0 14 0 0 0 3 0 0 0 12 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 80 0 0 0 0 0 0 0 2 0 0 0 52 0 0 0 0 0 0 0 49 0 0 0 80 0 0 0 14 0 0 0 3 0 0 0 66 0 0 0 30 0 0 0 16 2 0 0 0 0 0 0 2 0 0 0 76 0 0 0 0 0 0 0 12 0 0 0 80 0 0 0 0 0 0 0 7 0 0 0 2 0 0 0 64 0 0 0 0 0 0 0 2 0 0 0 80 0 0 0 0 0 0 0 69 0 0 0 30 0 0 0 48 3 0 0 0 0 0 0 12 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 64 0 0 0 0 0 0 0 2 0 0 0 52 0 0 0 0 0 0 0 49 0 0 0 80 0 0 0 14 0 0 0 3 0 0 0 12 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 80 0 0 0 0 0 0 0 2 0 0 0 52 0 0 0 0 0 0 0 49 0 0 0 80 0 0 0 14 0 0 0 3 0 0 0 12 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 64 0 0 0 0 0 0 0 2 0 0 0 52 0 0 0 0 0 0 0 49 0 0 0 80 0 0 0 14 0 0 0 7 0 0 0 12 0 0 0 0 0 0 0 0 0 0 0 2 0 0 0 80 0 0 0 0 0 0 0 2 0 0 0 52 0 0 0 0 0 0 0 49 0 0 0 80 0 0 0 14 0 0 0 7 0 0 0 2 0 0 0 80 0 0 0 0 0 0 0 12 0 0 0 64 0 0 0 0 0 0 0 7 0 0 0 19 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0}

compute_stack{}

#### registers
CSP{0}

FP{0}

PC{816}

SP{4096}





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







