%ext_log
%ignore_ws
%%

BEGIN{
 -- Logs begin at ident integer-integer-integer integer:integer:integer UTC, end at ident integer-integer-integer integer:integer:integer UTC. -- => header{$5;$6;$8;$10;$11;$13;$15;$20;$21;$23;$25;as_int(double_quote $26 double_quote); $28;$30;};.
 ident integer integer:integer:integer ident-ident ident[integer]: [integer/integer/integer _ integer:integer:integer:integer] [Message] CP measured voltage changed from double V to double V => 
  voltage{ timestamp{
                day{as_int(double_quote $20 double_quote);};
                month{as_int(double_quote $18 double_quote);};
                year{2000+as_int(double_quote $16 double_quote);};
                hour{as_int(double_quote $22 double_quote);};
                minute{as_int(double_quote $24 double_quote);};
                sec{as_int(double_quote $26 double_quote);};
                millisecond{as_int(double_quote $28 double_quote);};
              };  from{$-5;}; to{$-2;};};.
 any => .
}

%%
