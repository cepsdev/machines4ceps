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
* Clone, checkout 5.x branch, and build cryptopp:
  * git clone https://github.com/weidai11/cryptopp.git 
  * cd cryptopp
  * git checkout CRYPTOPP_5_6_5
  * make
  * cd ..
* Clone and build  machines4ceps
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

Let's fire three __CAPS_LOCK__ events and look how the state machine behaves.

```Pascal
Simulation{
 Start{basic_example;};
 
 CAPS_LOCK;
 CAPS_LOCK;
 CAPS_LOCK;
 };
```

To run this simulation type (assuming you are in a shell/terminal and your working directory is machines4ceps/examples/first_steps):
* ../../bin/__ceps__ basic_uml_state_diagram.ceps simulation_2.ceps

This should produce the following output:
```Pascal
basic_example.Initial- basic_example.default+
basic_example.default- basic_example.caps_locked+
basic_example.default+ basic_example.caps_locked- 
basic_example.default- basic_example.caps_locked+
```

The default behaviour of __ceps__ is: for each event, and after all transitions triggered by this particular event had been taken, print on a single line the set of all changed states (i.e. a state diff). Each state printed is annotated by a __+__ or __-__, indicating whether the state is active or not. We have three events in our last simulation, but four lines of output.That's because of the transition __t{Initial;default;};__ which has no associated event and is therefore triggered simply by starting the state machine (epsilon transition).

### Visualization

The following requires *graphviz* to be installed on your machine (see https://graphviz.org).

With the option __--dot_gen__ set, __ceps__ writes a *dot* representation of all top level state machines into the file __out.dot__.

The commands

* ../../bin/ceps basic_uml_state_diagram.ceps --dot_gen
* dot -Tpng out.dot -o img/basic_uml_state_diagram.png

produce the following graphical representation of the state machine __basic_example__.

![](examples/first_steps/img/basic_uml_state_diagram.png)

### Completing the basic example: Adding Actions

We complete the ceps version of the basic state machine, by adding the missing transitions under the __ANY_KEY__ event together with the associated actions.

```javascript
kind Event;

Event CAPS_LOCK, ANY_KEY;

sm{
 basic_example;
 states{Initial; default; caps_locked;};

Actions{
  send_lower_case_scan_code {print("basic_example.send_lower_case_scan_code()\n");};
  send_upper_case_scan_code{print("basic_example.send_upper_case_scan_code()\n");};
 };

 t{Initial;default;};
 t{default;caps_locked;CAPS_LOCK;};
 t{caps_locked;default;CAPS_LOCK;};

 t{default;default;ANY_KEY;send_lower_case_scan_code;};
 t{caps_locked;caps_locked;ANY_KEY;send_upper_case_scan_code;};
};

```

The code can be found in __examples/first_steps/basic_uml_state_diagram_with_actions.ceps__.

The extended version still works with our simulations (*simulation_1.ceps* and *simulation_2.ceps*) and exhibits exactly the same behaviour.
We need to trigger at least one __ANY_KEY__ event to activate our new transitions.

```javascript
Simulation{
 Start{basic_example;};

 ANY_KEY;
 CAPS_LOCK;
 ANY_KEY;
 CAPS_LOCK;
 ANY_KEY;

 };
```
The code can be found in __examples/first_steps/simulation_3.ceps__.

If we run 

* ../../bin/ceps basic_uml_state_diagram_with_actions.ceps simulation_3.ceps

we get the following output.

```javascript
basic_example.Initial- basic_example.default+ 
basic_example.send_lower_case_scan_code()
basic_example.default- basic_example.caps_locked+ 
basic_example.send_upper_case_scan_code()
basic_example.default+ basic_example.caps_locked- 
basic_example.send_lower_case_scan_code()
```






