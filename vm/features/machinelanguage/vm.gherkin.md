# Feature : A bytecode VM for the efficient execution of actions, guards etc.


## Scenario: A VM contains code, data, stack and a compute stack. 

*Given* an empty stack the_stack.


*And* an empty data segment the_data.


*And* an empty code segment the_code.


*And* an empty computation stack the_compute_stack.


*When* we create a new VM the_vm.


*Then* the newly created VM the_vm is equal to a VM formed of the_stack,the_data,the_code,the_compute_stack .