# Skill: ceps language and ceps tool
If you have to write ceps programs or ceps specifications you find here the description of the language ceps and the tools that executes ceps programs/specification.

[TOOL DESCRIPTION]
- Tool Name: ceps interpreter
- How to invoke: Run `ceps FILE [FILE...] `
- Expected Output: An execution trace intermingled with possible output of the pogram/specification and possible warnings and errors, execution trace is discussed in section [EXECUTION TRACE].

[LANGUAGE DESCRIPTION]
- Language Name : ceps or cepS
- Paradigm: Language for executable specifications, procedural, functional and object oriented allowing partial programs, i.e. wellformed syntactical units containing unbound entities are largely ignored.

[LANGUAGE DESCRIPTION - BASIC STUCTURE]
The minimal ceps program is the empty string, i.e. 
```
```
is a wellformed ceps program.

A ceps program is a sequence of expressions, named or unnamed blocks, and declarations. An expression is followd by a semicolon, a named block is followed by a semicolon, an unnamed block doesn't need to be terminated by a semicolon. Blocks, named or unnamed, can be nested. Unnamed blocks are also called scopes.
Example: 
```
1 + A;
1 +1;
{
    1;2;GAGA;f(); g(f(1,2));
}

named_block{
    another_named_block{

    };
};
```

In the given example the first line contains an unbound identifier A, this is not an error. The second line contains 1+1 this is equivalent to 2. 
Important: the expression statement 1+1; is not optimized away like in the C programming language, it is a distinct feature of ceps that a well formed expression statement has a normalized (term will be explained later) form which is its meaning after the normalization phase of the execution has terminated and before the operational phase of the execution has started. The execution of a specification has up to four phases:
- raw phase
- normalization phase
- operational phase
- information gather phase

The raw phase parses the input documents, by applying (if necessary) user defined lexers and parsers (this document doesn't explain user defined lexers/parsers). The result of the raw phase is an unevaluated Abstract Syntax Tree (uAST).
The normalization phase is the phase where the uAST from the preceding phase is evaluated. The result is an evaluated or normalized AST (eAST or nAST). Evaluation of an AST means basically the execution of a functional program defined by the uAST. The expression 1+1; is such a (trivial) functional program.

Example:

```
numbers{1;2;3;4;5;};
val sum = 0;
for (e: root.numbers.content()){let sum = sum + e;}
sum;
```

The meaning of this specification is identical to the result of the normalization phase  which is the eAST, the eAST is:
```
numbers{1;2;3;4;5;};
15;
```
We now explain the operational phase. Once the eAST has been generated it is executed like a C program would be executed, line by line, block by block.
The ceps interpreter searches for named blocks or other eAST entities that have operational semantics. The most useful named block is the state machine
definition block. Any state machine definition block has the following form:
```
sm{
    Identifier;
    ....
};
```
Where Identifier is the unique name of the the state machine defined.
Another block with operational semantics is the Simulation block:
```
Simulation{
    [Start{List of state machine identifiers};]
    [EVENT1;]
    [EVENT2;]
    ....
}
```

Hence a specification of a simple sensor with a non trivial operational phase looks like this example:

```
kind Event;
Event POWER_ON, POWER_OFF;

sm{
    Sensor;
    states{Initial;Boot;Ready;};
    t{Initial;Boot;POWER_ON;};
    t{Boot;Ready;};
    t{Ready;Initial;POWER_OFF;};
};
Simulation{
 Start{Sensor;};
 POWER_ON;
};
```
This example shows a very basic state machine, transitions are t-blocks, the first transition changes the default state Initial into the state Boot under
the event POWER_ON. The second transition is a so called epsiolon transition it has no conditions, so it is taken each time the system is in state Boot, the
resulting state is Ready. The last transition describes what happens if the event POWER_OFF occurs.
If you run this spec with ```ceps spec.ceps``` the resulting execution traces is:
```
Sensor.Initial- Sensor.Boot+ 
Sensor.Boot- Sensor.Ready+ 
```
Two lines, this means that Sensor.Ready+ for example strictly happend after Sensor.Initial-.
State machines can be nested, the state machines supported by ceps is a generalizaiton of so called Harel-Charts. Details in a later section.
Remark: Execution traces are described in section [EXECUTION TRACE]
IMPORTANT: Only state machines defined at the lexical top level can be included in a Start-directive.

[LANGUAGE DESCRIPTION - THE SM BLOCK]
As already mentioned, state machines are defined via the sm block. Here is a list summarizing the main features of state machines:
- Atomic states are the states listed in the states block, each sm block contains one or none states block. The location of which is not important.
- Sub State Machines. State machines can be nested, i.e. a sm block can contain arbirtrarily many sm blocks, the identifiers of the sub sm blocks must be unique. A sub machine is referenced by an id, see the rule for id resolution below: from inside A the sub machine B is referenced by B, from anywhere by A.B . Execution traces and reports always display the fully qualified form A.B .
- Transitions. A sm block can contain multiple t-blocks, the order of which is not important, hence side effects of guard evaluations and actions
that affect other guards or actions in the same scope lead to undefined behaviour. The minimal transitions define a pair of state ids, which must occur at the very beginning of the t-block, the first id is the source state-id, the second id is the destination source-id. State ids can be composite, i.e. a sourec/destination state can have the form A.B.C. State ids can reference atomic states and sub states, by using composite ids you can reference states deeper down
the sm-block induced hierarchy of composite states.
ID RESOLUTION: an id is a dot-separated path. It is first resolved relative to the enclosing sm-block (lexical scope); if it does not resolve there it is resolved from the top level. Both forms are therefore accepted. Given
```
sm{A; states{Initial;}; sm{B; states{Initial;}; sm{C; states{Initial;};}; }; };
```
each of these transitions, written inside A, is valid and names the same destination:
```
t{Initial;B;}        // relative, one level
t{Initial;A.B;}      // from the top level
```
and likewise for the machine C nested in B:
```
t{Initial;B.C;}      // relative, two levels
t{Initial;A.B.C;}    // from the top level, two levels
```
A path that resolves neither way is an error, e.g. A.A.B written inside A.
Optional parts of transitions are: a guard expression or a guard id, an event-id, up to three actions. The optional parts can appear in any order.
Some Examples for transitions:
```
sm{
    S1;
    states{Initial;a;};
    t{Initial;a;};
};
```
This example defines a single state machine with he name S1 and two atomic states S1.Initial and S1.a. A state machine doesn't need to have any states, but only state machines with an atomic state Initial are considered by the execution engine for scheduling. The example defines one transitions, if S1 gets exewcuted it immediately changes into state a, there it reamins for the entirety of the operational phase.
```
kind Event;
Event E;

sm{
    S2;
    states{Initial;a;};
    t{Initial;a;E;};
};
```
This example is identical to the previous one with the only exception that the tranistion from Initial to a must be triggered by an event E. Events
are declared as symbols of kind Event, kinds are a weak form of typing. The kind Event has to be made available in the surrounding lexical scope, this
is what the declaration ```kind Event;```does. If we append a Simulation block we can execute S2:
```
kind Event;
Event E;

sm{
    S2;
    states{Initial;a;};
    t{Initial;a;E;};
};
Simulation{
 Start{S2;};
 E;E;E;
};
```
Running this example with the ceps interpreter gives the following output:
```
S2.Initial- S2.a+
```
It is good practice to keep simulation blocks in different files from the actual specification, in this case the definition of the state machine S1 should be placed in a file s1.ceps and the simulation block in a file sim.,ceps. To run the complete simulation the comman looks like this
```
ceps s1.ceps sim.ceps
```
The command
```
ceps s1.ceps
```
Stops with the generation of the eAST, the eAST can be inspected with
```
ceps s1.ceps --pe
```
The uAST can be inspected with 
```
ceps s1.ceps --pr
```
[STATE MACHINE ATIONS AND HIGH LEVEL ASSEMBLER]
[STATE MACHINE ATIONS]
State machines can have actions, actions can be called as a side effect of a transition, actions are defined in the Actions sub-block of an sm-block.
An Actions block can appear anywhere in the same lexical scope as the state machine's id as long as it appears after the id, there can be multiple
Actions blocks, a transition can reference up to three actions:
```
sm{
    S;
    Actions{
        action_1{

        };
        action_2{

        };
    };
    states{Initial;a;b;};
    Actions{
        action_3{

        };
        action_4{

        };
    };
    t{Initial;a;action_1;action_2;};
    t{a;b;action_4;action_3;action_2;};

};
```
Remark: The calling order of the actions referenced by a transition is the same as their order in the calling list, no order between actions defined
by different transistions is imposed, hence relying on the result of an action in second one is only defined within a transition.

[HIGH LEVEL ASSEMBLER: Oblectamenta]
ceps offers a high level assembler for the specification of operational semantics, the name of the assembler is Oblectamenta. Oblectamenta runs on
a virtual machine which has 64 bit wide register, a 64 bit address space, a computation stack, 16 general purpose registers R0-R15, a stack pointer register
SP (stack grows from higher to lower addresses), a frame register FP, a compute stack register CSP (CSP is intially 0 and grows towards higher addresses),
an instruction pointer PC, 6 argument registers AG0 - ARG5, a result register RES. ceps comes with a built-in just in time compiler for x86 and ARM architectures (remark: experimental). It might strike you as highly unusual to find a low level assembler combined with very high level concepts like
metaprograms operating on normalized ASTs and state machines, but there is a method to the madness. ceps is about incrementally deriving programs, and
it offers the full range of an algebraic expression with unbound identifiers to a full blown micro service defined down to the level of individual
machine registers. After giving a very detailed, very low level example of a very simple function (comuting the maximal element of an array), we'll show how a readable specification with precise semantics can be derived.

Example of a state machine with an action that finds the maximal element in an array:
```
kind Event;

kind OblectamentaOpcode;
kind OblectamentaDataLabel;
kind OblectamentaModifier;
kind OblectamentaCodeLabel;
kind OblectamentaReg;


OblectamentaModifier addr;
OblectamentaModifier reg;
OblectamentaReg SP, FP; 

oblectamenta{
 global{
   data{
        OblectamentaDataLabel array;
        OblectamentaDataLabel i;
        OblectamentaDataLabel n;
        OblectamentaDataLabel x;
        OblectamentaDataLabel j;
        OblectamentaDataLabel cur_m;
        OblectamentaDataLabel four, one;
        cur_m; -1;
        i; 0;
        array; 4;1;7;9;2;5;8;10;3;6;
        n; 10;
        four; 4;
        one; 1;
    };
 };
};

sm{
    S;
    states{Initial;A;};
    Actions{
        doFindMaxElement{
            oblectamenta{
                        text{
                            asm{
                                OblectamentaOpcode halt, noop, ldi32,blteqzeroi32,muli32,ui32toui64,ui32toui64,lea,addi64,ldsi32,sti32,buc,dbg_printlni32;

                                OblectamentaDataLabel array;
                                OblectamentaDataLabel i,four,one;
                                OblectamentaDataLabel cur_m, n;
                                OblectamentaCodeLabel done,loop,loop_tail;
                            loop;
                                ldi32(i); // push i32 value referenced by i onto compute stack
                                ldi32(n); // push i32 value referenced by n onto compute stack
                                subi32; // push_cs_i32(pop_cs_i32 - pop_cs_i32)
                                blteqzeroi32(done); // if (i >= n) goto done;

                                ldi32(i); // push int 32 value referenced by i onto compute stack
                                ldi32(four); // push int 32 value referenced by i onto compute stack
                                muli32; // pushi32()
                                ui32toui64;
                                lea(array);                            
                                addi64;
                                ldsi32; // array[i] on top of stack
                                ldi32(cur_m);
                                subi32;
                                bgteqzeroi32(loop_tail);
                                ldi32(i);
                                ldi32(four);
                                muli32;
                                ui32toui64; // convert i32 value on top of compute stack to unsigned int 64 value
                                lea(array); // push 64 bit address of array onto compute stack
                                addi64;
                                ldsi32; // array[i] on top of stack
                                sti32(cur_m); // pop and store i32 value to address referenced by cur_m 


                                loop_tail; // label                    
                                ldi32(one);
                                ldi32(i);
                                addi32;
                                sti32(i); // i = i +1 ;
                                buc(loop); // branch unconditionally

                            done;
                                dbg_printlni32(cur_m);//Pseudo opcode for printing i32 to stdout
                                halt;
                            };
                        };
            };
        };
    };
    t{Initial;A;doFindMaxElement;};
};

Simulation{
    Start{S;};
};
```


[EXECUTION TRACE]
The execution trace is a result of phase 3 (see the description of the different phases of executing a specification with ceps).
An execution trace describes the state changes of state machines during the execution of a ceps program. A ceps program doesn't have to have state machines, therefore the execution trace can be the empty string followed by a new-line character.
The general form of an execution trace is: 
list of state changes in step 1 NEWLINE list of state changes in step 2 NEWLINE list of state changes in step 3 NEWLINE ...
The order of states in each step has no meaning hence the order in which the states are printed between two NEWLINEs or the beginning of the trace and the first NEWLINE is of no relevance. However, the order in which two state changes appear in the execution trace matters if they are separated by at least one NEWLINE. In this case the relative order of the state changes in the execution trace is the relative order of the state changes during execution. A state change has the form  ID[.ID]*(+|-) (regular expression). A trailing '+' means the state was visited, a trailing '-' means the state was exited.
Example. This specification:
```
kind Event;
Event E;

sm{S;  states{Initial;Final;}; t{Initial;Final;};};
sm{S2; states{Initial;A;};     t{Initial;A;};};
sm{S3; states{Initial;};
   sm{Sub1; states{Initial;A;}; t{Initial;A;E;};};
   t{Initial;Sub1;};
};

Simulation{
 Start{S;S2;S3;};
 E;
};
```
produces this execution trace:
```
S.Initial- S.Final+ S2.Initial- S2.A+ S3.Initial- S3.Sub1+ S3.Sub1.Initial+ 
S3.Sub1.Initial- S3.Sub1.A+ 
```
Note that entering a sub state machine logs the machine itself as well as its initial state, hence S3.Sub1+ and S3.Sub1.Initial+ both appear.
It is very important to keep in mind that only changes are logged (delta log), in the example above the state machine S2 enters the state A in the first line and stays there for the rest of the logged execution.

[OBSERVER STATE MACHINES ARE THE CANONICAL WAY TO ENRICH EXECUTION TRACES]
Execution Traces are compact projections of the real execution onto the space of finite sequences of sets of state-changes (A+ or B- are examples of state changes), execution traces omit a lot of potentially interesting information like triggered events, taken transitions, evalutated guards etc. This is intentionally so, execution traces are not diagnositc traces. There is a very elegeant way to make any interesting diagnostic data, e.g. events, visible by introducing an observing state machine. 
Formal definition of a minimal observing state machine: A minimal observing state machine MOSM(E,S), where E is a Guard, i.e. a boolean expresssion, or an Event and S is an identifier, is 
``` 
sm{MOSM; states{Initial;S;}; t{Initial;S;E;}; t{S;Initial;}; };
```
Now the definition of Observer State Machine: An observer state machine OSM for E_1.E_2,...,E_with S_1,...S_n as in the definition before, is a state machine such that the projection of each execution trace T of OSM onto the state space of MOSM(E_i,T_i) constitutes a valid trace for the MOSM(E_i,T_i) for each i=1,...,n. 
We do not explicitly disallow observing state machines having side effects, but the property stated above requires the OSM(E_1,S_1,...,E_n,S_n) to be free of side effects relative to E_i, the observed entities.
Example:
```
kind Event;
kind Guard;
Event E;
Guard G;

sm{
 Observe_E_and_G;
 states{Initial;Observe_E;Observe_G;};
 t{Initial;Observe_E;E;};
 t{Initial;Observe_G;G;};
 t{Observe_E;Initial;};
 t{Observe_G;Initial;};
};
```  
Remark: It is a good practice to keep observers separate from the main specification, group observers thematically into different files, files with observers a best placed at the very end of a ceps session:
```
ceps common_stuff.ceps spec-part-1.ceps spec-part-2.ceps observers-1.ceps observers-2.ceps simulations.ceps
```
This is of course not required, there is no pattern which serves each scenario equally well, but keeping observers separate in different files allows for "observer injection": only the relevant observers for a particular session are started, observers are grouped thematically which also allows for later reuse and documentation.



