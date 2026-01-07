# Conversational Programming Language

ceps is a conversational programming language, conversational in the sense that programs are the result of humans engaging in an active dialogue with machines. The ultimate goal of a ceps conversation is a running program which fulfills its purpose reliably and reasonably, i.e. the program meets its specification (is correct) and performance demands. Conversations are in a state of flux, this 'partialness' is reflected by the ceps concept of incomplete programs. A C++ program can be in one of two states: (syntactically) illformed - the compiler refuses to produce an object file, or (semantically) erroneous - the program compiles but doesn't match its specification. A ceps program has a third state: transient. To give an example:

The expression 

A + B;

Where A,B are not declared identifiers is a compiler error in C++, but perfectly valid in ceps. The meaning of the above expression is A + B;. 


## Portable Assembler + x64 

oblectamenta{
 text{
  asm{
    x64{
      I(P(...),O(...),ModRM(...),SIB(...),D(...),Imm(...));
    };  
    };
 };
}:


