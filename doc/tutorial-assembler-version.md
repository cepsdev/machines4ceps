# Tutorial using portable assembler (Oblectamenta)


## Guards

```ceps
kind Guard;

oblectamenta{
 OblectamentaDataLabel valve_status;
 global{
   data{
        valve_status;
        1;
    };
 };
};

Guard valve_open;
Guard valve_not_open;

valve_open = 
 oblectamenta{text{asm{
    OblectamentaDataLabel valve_status;
    ldi32(valve_status);
    sti32(RES);
 };};};

valve_not_open =  oblectamenta{text{asm{
    OblectamentaDataLabel valve_status;
    OblectamentaCodeLabel is_unequal_zero;
    ldi32(valve_status);
    bnzeroi32(is_unequal_zero);
    ldi32(1);
    sti32(RES);
    halt;
    is_unequal_zero;
    ldi32(0);
    sti32(RES);
 };};};
 
sm{
    S;
    states{Initial;W;L;R;};
    t{Initial;W;};
    t{W;L;valve_open;};
    t{W;R;valve_not_open;};
};

Simulation{
    Start{S;};
};

```

```ceps
kind Guard;
```
This is a decleration. We use transition guards which are symbols of the type (called kind) *Guard*.

```ceps
oblectamenta{
 OblectamentaDataLabel valve_status;
 global{
   data{
        valve_status;
        1;
    };
 };
};
```
Declares and defines a 32 bit signed integer value named *vault_status' and sets its value to 1, this value is stored in the static data segment and remains available until process termination.
```ceps
Guard valve_open;
Guard valve_not_open;
```
Declares two symbols of type *Guard*: vault_open and vault_not_open, without defining their value. The expression *Guard A  = B* would be a syntax error.


