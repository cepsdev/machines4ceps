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


# Oblectamenta: a tutorial

Oblectamenta is the portable assembler ceps compiles its actions and guards to. This
chapter is a walk through its data model, starting from the one idea the whole assembler
rests on and ending with records.

Everything marked **today** below is implemented and runs; everything marked **planned**
is a design decision recorded here before implementation. The worked example is
`examples/sm-actions-asm/count2.ceps`, which runs to completion and prints `100`.

## 1. One idea

> A declared symbol's **kind** says what it is. Its **position** says where it is.

That is the whole assembler. Two kinds carry the weight:

| Kind | Marks a position in |
|---|---|
| `OblectamentaDataLabel` | the VM's memory |
| `OblectamentaCodeLabel` | the VM's text segment |

Declaring gives a name its kind; writing the name on its own line *places* it. Nothing
else is going on, and the rest of this chapter is consequences.

## 2. Data sections

Data lives in a `data{}` block inside `oblectamenta{global{...}}`:

    oblectamenta{
     global{
       data{
            OblectamentaDataLabel i,n,one;
            i; 0;     // counting index
            n; 100;   // number of iterations
            one; 1;
        };
     };
    };

(`examples/sm-actions-asm/count2.ceps:8-17`.)

The first line **declares** three labels. The lines after it **place** them. A label
placed between two values is a name for the offset the heap had reached when the label
was read (`core/src/vm/vm_ceps_core_helper.cpp:137`):

    smc->vm.data_labels()[name(...)] = smc->vm.mem.heap - smc->vm.mem.base;

Three things follow, and they are worth stating plainly because they explain everything
else:

1. **A label is an address, and nothing more.** It has no type field and no length field.
2. **A label occupies no space.** Two labels in a row name the same address.
3. **Placement is adjacency.** The datum a label names is the one written after it.

## 3. The type comes from the data

There is no type syntax. A datum's type is the kind of literal you wrote:

| You write | You get |
|---|---|
| `i; 0;` | `int32` |
| `x; 1.5;` | `double` |
| `s; "A text";` | the bytes of the string |
| `b; as_uint8(7);` | one byte |

This is the scribbling principle applied to types: the literal is already on the page, so
it is not restated in a second, declarative language. Where a literal underdetermines the
type, you annotate **the value** — `as_uint8(7)` — rather than the name somewhere else.
The correction stays on the same sheet.

Arrays need no syntax either, because a label is only an address:

    array; 4;1;7;9;2;5;8;10;3;6;
    i; 0;

`array` names the first element. The nine values after it are simply there. This works,
and `vm/features/bubblesort.ceps:28-31` relies on it.

## 4. Data sections are read first

All data sections in the document are processed **before** any assembler section is
assembled (`core/src/vm/vm_ceps_core_helper.cpp:114-160`). They accumulate into one heap
shared by the whole program.

The practical consequence is that **declaration before use is not required**. This program
runs and prints `5`:

    oblectamenta{ global{ data{ OblectamentaDataLabel b; b; 2; }; }; };

    sm{
        S;
        states{Initial;Done;};
        Actions{
            doSum{
                oblectamenta{ text{ asm{
                    OblectamentaDataLabel b,c,r;
                    ldi32(b);
                    ldi32(c);
                    addi32;
                    sti32(r);
                    dbg_printlni32(r);
                    halt;
                };};};
            };
        };
        t{Initial;Done;doSum;};
    };

    oblectamenta{ global{ data{ OblectamentaDataLabel c,r; c; 3; r; 0; }; }; };

    Simulation{ Start{S;}; };

The action reads `c`, which is defined below it, and writes `r`, likewise. This is legal. A
data section is a sheet you may add to the pile at any point; the assembler reads the whole
pile before it starts.

## 5. Using data from assembly

Inside an `asm{}` block, re-declare the labels the block refers to:

    asm{
        OblectamentaDataLabel i,one,n;
        OblectamentaCodeLabel done;

        ldi32(i);       // push i
        ldi32(n);       // push n
        blteq(done);    // branch to done iff i >= n
        ldi32(i);
        ldi32(one);
        addi32;         // i + 1
        sti32(i);       // store it back
        E;              // fire event E
        halt;
        done;           // <- placement of the code label
        F;              // fire event F
    };

(`examples/sm-actions-asm/count2.ceps:26-45`.)

Four things to notice:

- **The re-declaration is load-bearing.** It is what gives `i` the `OblectamentaDataLabel`
  kind inside this block. Without it the assembler sees a plain identifier rather than a
  kinded symbol, which is the same distinction that [`DEFECTS.md` D1](DEFECTS.md#d1) turns on.
- **Registers need no declaration.** `SP`, `FP`, `ARG0`–`ARG5`, `RES` and `R0`–`R15` are
  declared once in `core/include/oblectamenta_decls.ceps:46-47`.
- **Code labels are placed by adjacency too** — `done;` on its own line marks the position
  `blteq(done)` jumps to. Same idea as section 1, different segment.
- **An `Event` symbol on its own line fires that event.** `E;` is a statement.

## 6. The built-in expression compiler

You do not have to write all of that by hand. An expression appearing in an assembly
stream is compiled to mnemonics for you
(`core/src/vm/oblectamenta-assembler.cpp:1117-1137`). The idea is worth stating as a whole:

- **It is a source-to-source pass, not a back end.** Input is ceps; output is ceps —
  symbols of kind `OblectamentaOpcode` — spliced back into the same stream, which then
  goes through the ordinary assembler. There is no intermediate representation.
- **It is typed by the document, not by its syntax.** Operand types are resolved by
  looking up the data that follows the label, exactly as in section 3. This is the only way
  to put a type discipline on top of an assembler without inventing a declaration language.
- **Lowering is additive.** The generated mnemonics are inserted *ahead of* the source
  expression, which is left in place. Afterwards the stream holds both levels at once —
  the scribbling sheet is not thrown away when the fair copy is made.
- **Failure is loud.** An expression the compiler cannot handle is reported with the
  offending expression printed, not silently skipped.

**Status (today):** a data-label leaf and unary `!` are compiled. The grammar in the
comment at `oblectamenta-assembler.cpp:1123` — binary `+ - *`, `==`, `!=`, parentheses —
is the plan, and the code says so: *"Only supported type so far is int"*.

## 7. Extent: an alternative to adjacency — *planned*

Adjacency places a label, but it never records **how far the label reaches**. In

    array; 4;1;7;9;2;5;8;10;3;6;
    i; 0;

nothing says `array` has ten elements. The reader decides, and by convention the run ends
where the next label begins. That convention has three costs:

1. appending a value to a `data{}` block silently extends the last label's array;
2. inserting a label in the middle silently shortens the one before it;
3. a label that is declared but never placed cannot be told from one placed with no data.

Because ceps already parses `f(...)` where `f` is a symbol, extent can be made syntactic
with no new grammar — apply the label to its data:

    array(4,1,7,9,2,5,8,10,3,6);
    i(0);
    n(100);

One binding is now one node. It can be moved, copied or generated as a unit, appending to
the block cannot disturb it, and `b()` says *placed and empty* as distinct from *declared
and never placed*.

**Adjacency is not withdrawn.** The two notations coexist: adjacency is the quickest thing
to write, application is the one that survives editing. Value annotations compose
unchanged — `i(as_uint8(0))`.

## 8. Records, for free — *planned*

Once a label can be applied, nesting means what you would hope:

    a(b(0),c(0));

is equivalent to

    struct a { int b = 0; int c = 0; };

This is not an analogy. A C struct *is* a base address plus named offsets, and that is
precisely what the nested form produces: `a` names the base, `b` and `c` name positions
inside it, and each member's type comes from its literal exactly as in section 3. The
equivalence is exact because a data label was never anything but an address.

Nothing new is introduced to get this. There is no record syntax, no member-access
operator and no struct declaration — only application, applied twice. Arrays of records,
records of arrays and deeper nesting follow by writing them:

    points( p0(0,0), p1(3,4) );
    config( retries(3), name("fan"), limits(as_uint8(0),as_uint8(100)) );

## 9. Summary

| Form | Means | Status |
|---|---|---|
| `OblectamentaDataLabel d;` | declare `d`, giving it its kind | today |
| `d; 1;` | place `d` at an `int32` holding 1 | today |
| `d; 1;2;3;` | place `d` at the first of three `int32`s | today |
| `d; as_uint8(7);` | place `d` at a single byte | today |
| `d(1)` | the same binding, with its extent written down | planned |
| `d(1,2,3)` | an array of three, extent explicit | planned |
| `a(b(0),c(0))` | a record with members `b` and `c` | planned |

The through line is section 1. A kind says what a name is; a position says where it is.
Adjacency and application are two ways of writing the position — the second one says where
it stops.

