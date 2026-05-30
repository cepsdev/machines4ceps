# ceps language specification (NOT EVEN REMOTELY COMPLETE)
The main ideas underlying the ceps language are
- Bottom up is (mostly) better than top down
- Syntax is secondary
- Every part of the abstraction hierarchy is accessible to the programmer, i.e. the backend(s), the various intermediate representations, and the abstract syntax representation of the program are hackable.
- Hierarchical state machines are part of the core language

## Oblectamenta Assembler
This is the default intermediate representation, which is directly executed by the built-in VM or compiled into the host's native machine code
(if supported) during runtime ('Just In Time Compilation').
### Lexical Scoping
The oblectamenta assembler understands lexical scoping. All scopes with the exception of the default scope start with an opening curly brace { and end with the matching closing curly brace }. Scopes can be nested, hence scopes form a tree with the default scope at the root, a scope contains another scope if and only if it is an ancestor in the scope tree. All labels defined
```ceps
oblectamenta{
 text{
    asm{

    };
 };
};
```