# Message Serialization
The Oblectamenta assembler offers built-in support in the form of the *msg* macro for the serialization/deseralization of messages.
## Serialization
### The msg macro
A *msg* section with a symbol of kind *OblectamentaMsgDefDirective* as its very first subelement defines a *message directive* if it is also an top-level element of an asm section.

```javascript
kind OblectamentaMsgDefDirective;
OblectamentaMsgDefDirective msgtag;

...
 asm{
   msg{
        msgtag;
        ...
   };
 };
... 
     
```
The name of the *OblectamentaMsgDefDirective* symbol has no further meaning and is ignored by the assembler, hence the snippet given above is equivalent to the following one. 


```javascript
kind OblectamentaMsgDefDirective;
OblectamentaMsgDefDirective MyMsgTag;

...
 asm{
   msg{
        MyMsgTag;
        ...
   };
 };
... 
     
```

#### Location of the output
If the serialized message (the output) has to be written into the static data segment, a *data label* needs to be defined within the *msg* section, the location of the data label within the section can be arbitrary.
The following snippets are equivalent, both define the location of the serialized message to be the one labeled *msg_buffer*.
```javascript
kind OblectamentaMsgDefDirective;
OblectamentaMsgDefDirective MyMsgTag;
OblectamentaDataLabel msg_buffer;
...
 asm{
   msg{
        MyMsgTag;
        msg_buffer; //destination address of serialized message        
        f("gaga"); // if f has no meaning the result is an uninterpreted AST segment and not further evalutated by the msg-macro
        1; // Is ignored by the msg-macro
        

   };
 };
... 
     
```
```javascript
kind OblectamentaMsgDefDirective;
OblectamentaMsgDefDirective MyMsgTag;
OblectamentaDataLabel msg_buffer;
...
 asm{
   msg{
        MyMsgTag;
        f("gaga"); // if f has no meaning the result is an uninterpreted AST segment and not further evalutated by the msg-macro
        1; // Is ignored by the msg-macro
        msg_buffer;//destination address of serialized message
        ...
   };
 };
... 
     
```
### Defining the content of a message
A msg contains assembler mnemonics which have the same meaning as in a top-level asm section. Certain syntyctic elements within a msg macro define the structure and content of the serialized message, these elements are: *fields*, *tags* and *arrays*. 
These are the only syntactical entities of this kind, they carry no special meaning outside a msg-section.
#### Fields
The name N of a field or group is defined with a section with name N. Fields can be empty. Fields can be nested. 
```javascript
msg{
 user{ // field user
   name{};
   salary{};
   address{
   }:
 };
};
```
#### Tags
*Tags* define the basic types of data which a field can contain. There are the following tags: *sz*, *i32*, *i64*, *f64*. Tags are symbols of kind *OblectamentaMessageTag*.
A tag T signals to the assembler that data of the type T is the top element of the coumputation stack if the assembler code preceding T is executed.
The following snippet defines a field with the name *A* that contains a 32 bit signed integer with the content 42.

```javascript
msg{
 A{
   ldi32(11);
   ldi32(31);
   addi32;
   i32;
 };
};
```

### Example

```javascript
kind OblectamentaMsgDefDirective;        
OblectamentaDataLabel zip,street,firstname1,initial1,lastname1,msg_buffer2, msg_buffer,msg_text, one, some_number;


//Global Data
oblectamenta{
 global{
   data{
        msg_text;"Hello there!";0;
        
        firstname1;"Max";0;
        initial1;"M";0;
        lastname1;"Mustermann";0;
        street;"Pennsylvania Avenue";
        zip;"78996-5643";
        msg_buffer; for (i : 1 .. 256){0;} // here goes the serialized message as generated in S::Actions::doSendEvent()
    };
 };
};

sm/*state machine*/{
    /*name*/S;
    states{Initial; A;};
    Actions{
        doSendEvent{
            oblectamenta{
                    OblectamentaMessageTag i32;
                    OblectamentaMessageTag i64;
                    OblectamentaMessageTag f64;
                    OblectamentaMessageTag sz;                                   
                    OblectamentaMsgDefDirective this_is_a_msg_to_be_serialized;
            text{
                asm{
                  msg{
                    this_is_a_msg_to_be_serialized; // we want this to be treated as a message. 
                    msg_buffer; // That's the location the resulting byte stream should be written to
                   age{
                    ldi32(33);
                    i32;
                   };
                   id{
                    ldi64(11);
                    i64;
                   };
                   salary{
                    lddbl(33.3);
                    dbg_print_cs_and_regs(0);
                    f64;
                   };
                   firstName{
                    lea(firstname1);
                    sz;
                   };
                   initialName{
                    lea(initial1);
                    sz;
                   };
                   lastName{
                    lea(lastname1);
                    sz;
                   };
                   address{
                    street{lea(street);sz;};
                    no{ldi32(1000);i32;};
                    zip{lea(zip);sz;};
                   };
                };//msg
                dbg_print_data(0);
                dbg_deserialize_protobufish_to_json(msg_buffer);
              };//asm
             };//text
            };//oblectamenta
        };//doSendEvent
    };//Actions
    t/*transition*/{/*from*/Initial; /*to*/A;/*action*/doSendEvent;};
};

```
