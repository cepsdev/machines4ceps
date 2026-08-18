# Skill: ceps language and ceps tool
If you have to write ceps programs or ceps specifications you find here the description of the language ceps and the tools that executes ceps programs/specification.

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
Important: the expression statement 1+1; is not optimized away like in the C programming language, it is a distinct feature of ceps that a well formed expression statement has a normalized (term will be explained later) form which is its meaning after the normalization phase of the execution has terminated and befoer the operational phase of the execution has started. The execution of a specification has up to four phases:
- raw phase
- normalization phase
- operational phase
- information gather phase
The raw phase parses the input documents, by applying (if necessary) user defined lexers and parsers (this document doesn't explain user defined lexers/parsers). The result of the raw phase is an unevaluated Abstract Syntax Tree (uAST).
The normalization phase is the phase where the uAST from the preceding phase is evaluated. The result is an evaluated or normalized AST (eAST or nAST). Evaluation of an AST means basically the execution of a functional program operating on the uAST. The expression 1+1; is such a (trivial) functional program.Even more to the point: the functional program is the uAST of the preceding step.

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
IMPORTANT: Only state machines defined at the lexical top level can be included in a Start-directive.


[TOOL DESCRIPTION]
- Tool Name: ceps interpreter
- How to invoke: Run `ceps FILE [FILE...] `
- Expected Output: An execution trace intermingled with possible output of the pogram/specification and possible warnings and errors, execution trace is discussed in section [EXECUTION TRACE].

[EXECUTION TRACE]
An execution trace describes the state changes of state machines during the execution of a ceps program. A ceps program doesn't have to have state machines, therefore the execution trace can be the empty string followed by a new-line character. 
The general form of an execution trace is: 
list of state changes in step 1 NEWLINE list of state changes in step 2 NEWLINE list of state changes in step 3 NEWLINE ...
The order of states in each step has no meaning hence the order in which the states are printed between two NEWLINEs or the beginning of the trace and the first NEWLINE is of no relevance. However, the order in which two state changes appear in the execution trace matters if they are separated by at least one NEWLINE. In this case the relative order of the state changes in the execution trace is the relative order of the state changes during execution. A state change has the form  ID[.ID]*(+|-) (regular expression). A trailing '+' means the state was visited, a trailing '-' means the state was exited.
Examples for execution traces: 
```
S.Initial+ S2.Initial+
S.Initial- S.Final+ S2.Initial- S2.A+
S3.Initial+
S3.Initial- S3.Sub1.Initial+
S3.Sub1.Initial- S3.Sub1.A+
```
It is very important to keep in mind that only changes are logged (delta log), in the example above the state machine S2 enters in the second line the state A and stays there for the rest of the logged execution.
