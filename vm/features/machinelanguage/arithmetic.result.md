









# Scenario


## title
📎 Running the program stored in a VM changes the registers.



## Given
code (text segment) with an add instruction

### text
asm{addi32 halt}





## And
 Creating a VM with the code.

### vm
text{13 0 0 0 0 0 0 0}

compute_stack{1 1}





## When
 Running the VM.



## Then
 The PC register points to the halt instruction which terminated the execution.

### result
:heavy_check_mark: Passed









