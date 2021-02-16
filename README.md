# machines4ceps
__Write, run__, and __trace__ complex __state machines__ (__UML2__ statecharts, __Harel__ statecharts, state diagrams). Supports composite states, orthogonal regions, events, actions, init/exit etc. Small, fast, portable. Handles large state spaces (100k+). Monitoring, tracing, and a variety of communication protocols e.g. websockets, CAN field bus etc.

## Installation

### Prerequisites:
* Linux (Kernel Version >= 2.6)
* g++ (Version >= 9.0)
* bison (Version >= 2.3)
* make
### Build Steps:
All repositories need to be in the same directory as machines4ceps.
* Clone pugixml:
  * git clone https://github.com/zeux/pugixml.git
* Clone and build ceps:
  * git clone https://github.com/cepsdev/ceps.git
  * cd ceps/core
  * make
  * cd ../..
* Clone log4ceps
  * git clone https://github.com/cepsdev/log4ceps.git
* Clone and build cryptopp:
  * git clone https://github.com/weidai11/cryptopp.git 
  * cd cryptopp
  * git checkout CRYPTOPP_5_6_5
  * make
* Cloneand build  machines4ceps
  * git clone https://github.com/cepsdev/machines4ceps.git
  * cd machines4ceps
  * mkdir bin
  * make TARGET=bin

This should produce a binary called __ceps__ in the directory machines4ceps/bin.

## Writing and running state machines - Quick Start
### A basic state machine (see https://en.wikipedia.org/wiki/UML_state_machine)
![Basic state machine](https://upload.wikimedia.org/wikipedia/en/thumb/4/45/UML_state_machine_Fig1.png/660px-UML_state_machine_Fig1.png)
*Source:Wikipedia*

#### __A basic state machine__: Notation

Written in a notation supported by ceps (the tool built in the previous section):  

```bash
kind Event;

Event CAPS_LOCK, ANY_KEY;

sm{
 basic_example;
 
 states{Initial; default; caps_locked;};

 t{Initial;default;}; 
 t{default;caps_locked;CAPS_LOCK;};
 t{caps_locked;default;CAPS_LOCK;}; 
};
```
The code can be found in __examples/first_steps/basic_uml_state_diagram.ceps__.

#### __A basic state machine__: Execution (Part I)

One way to execute a state machine is through *simulation*.
The most basic simulation is to simply start a state machine:
```C
Simulation{
 Start{basic_example;};
 };
```
To run this example, open a shell/terminal, change your working directory to the *machines4ceps* repo, and type:
* __cd__ examples/first_steps
* ../../bin/__ceps__ basic_uml_state_diagram.ceps simulation_1.ceps

After executing the last command, you should see the following output:

__basic_example.Initial- basic_example.default+__

*Meaning:* The machine __basic_example__ makes the transition from state *Initial* to the state *default*. 

#### __A basic state machine__: Execution (Part II)

Let's fire some events and look how the state machine behaves.

```Pascal
Simulation{
 Start{basic_example;}; // as above
 
 CAPS_LOCK;
 CAPS_LOCK;
 CAPS_LOCK;
 };
```

To run this simulation - assuming your working directory is machines4ceps/examples/first_steps:
* ../../bin/__ceps__ basic_uml_state_diagram.ceps simulation_2.ceps

This should produce the following output:
```Pascal
basic_example.Initial- basic_example.default+
basic_example.default- basic_example.caps_locked+
basic_example.default+ basic_example.caps_locked- 
basic_example.default- basic_example.caps_locked+
```

The default behaviour of __ceps__ is: for each event, after all transitions have been taken, print on single line the set of all changed states (i.e. a state diff). Each state printed is annotated by a __+__ or __-__, indicating whether the state is active or not.








