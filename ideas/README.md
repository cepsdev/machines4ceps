# Ideas
## Serialization within Oblectamenta (gRPC wire protocol, json, bson and the like)

### General Context
Oblectamenta is the "lingua franca" of ceps, it is a portable machine language and in combination with ceps' "Abstract Syntax Tree as first class cititzen
principle" and the embracement of gotos via state machines we have a pretty expressive and at the same time exremely simple programming environment. If you need C, use ceps' grammar transformation, to get it, (ToDo: Write a C compiler using ceps which generates Oblectamenta code) Let's think about communicating with other parties via json via http, gRPC etc.
### Encoding messages

Consider the following Oblectamenta fragment:

```
...
data{
 text; "Hello there!";
};
...
text{
    
    AnEvent(
     message{
        user{
            id{
                ldi32(1);
                i32;
            };
            name{
                lea(text);
                sz;
            };
        };
      }
    );
}
```

The result shoud be:
- Event AnEvent has a payload, i.e. contains a pointer P to a message 
- P points into a memory address which contains something like |"user"|"id"|1|"name"|"Hello there!"|

The format alluded to can be easily converted into protobuf/json/bson etc. which would allow us to write services by employing a bunch of state machines
and pretty high level code in assembler.

More formal semantics:
```
...
text{
    
    N0(
     N1{
        ....
        Nn{
            Nn,1{
                F1
                tag1
            };
            Nn,2{
                F2
                tag2
            };
            ...
            Nn,m{
                Fm
                tagm
            };
        };
        ...
      };
      ...
    );
    ...
}
Where Fi is wellformed assembler code, and tagj is one of i32,i64,ui32,ui63,float32,float16,float64,sz,utf8.

The result is a 'linearization of the tree fragment' :

N0|....|Nn,1|tag1 representation of the top size(tag1) bytes of the compute stack immediately after processing F1|
        Nn,2|tag2 representation of the top size(tag2) bytes of the compute stack immediatley after processing F2|
        ...
        Nn,m|tagm representation of the top size(tagm) bytes of the compute stack immediatley after processing Fm|
        ....
   ...
...

```

### Reading messages

```
kind OblectamentaMsgDefDirective;        
kind OblectamentaMsgReadDirective;
OblectamentaMsgReadDirective read_message;
...
data{
 buffer; ... // here is the serialized message
};
...
text{
 msg{
   read_message;
   buffer;
   on_error{pr("Ups");halt;};
   A{
    i32;
    //value lies on top of the compute stack
   };
 };   
};
```
