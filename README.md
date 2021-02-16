# machines4ceps
Write,run and trace complex state machines (UML2 statecharts, harel statecharts, state diagrams). Supports composite states, orthogonal regions, events, actions, init/exit etc. Small, fast, portable. Handles large state spaces (100k+). Monitoring, tracing, and a variety of communication protocols e.g. websockets, CAN field bus etc.

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
* Clone machines4ceps
  * git clone https://github.com/cepsdev/machines4ceps.git
  * cd machines4ceps
  * mkdir bin
  * make TARGET=bin
## First Steps:
### A basic state machine (see https://en.wikipedia.org/wiki/UML_state_machine)
![Basic state machine](https://upload.wikimedia.org/wikipedia/en/thumb/4/45/UML_state_machine_Fig1.png/660px-UML_state_machine_Fig1.png)
*Source:Wikipedia*

In cepS notation:


kind Event;

Event CAPS_LOCK, ANY_KEY;

sm{
 basic_example;
 states{Initial; default; caps_locked;};
 t{Initial;default;};
 t{default;caps_locked;CAPS_LOCK;};
 t{caps_locked;default;CAPS_LOCK;}; 
};





Open a shell/terminal, change working directory to *machines4ceps*. Type:
* cd examples/first_steps
* ../../bin/ceps basic_uml_state_diagram.ceps simulation_1.ceps
  
  
