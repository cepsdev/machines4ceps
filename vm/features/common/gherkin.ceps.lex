%ext_gherkin
%ignore_ws
%%

lexer read_descr{
 \.  =>  $0 exit .
 any  =>  $0 blank .
}

lexer read_given{
 empty stack the_stack => let blank $2 blank = obj(stack{}) ; $2;.
 \.  =>   exit .
 any  =>  .
}

lexer read_given_and{
 empty data segment ident => let blank $3 blank = obj(data{}) ; $3;.
 empty code segment ident => let blank $3 blank = obj(text{}) ; $3;.
 empty computation stack ident => let blank $3 blank = obj(compute_stack{}) ; $3;.
 \.  =>   exit .
 any  =>  .
}

lexer read_when{
 create a new VM ident => let blank $4 blank = obj(vm{}) ; $4;.
 \.  =>   exit .
 any  =>  .
}

lexer read_then{
 ident is equal to a VM formed of ident,ident,ident,ident =>  verdict{equality_test{ 
            $0;
            vm{
                $8;
                $10;
                $12;
                $14;
            };
        };};.
 \.  =>   exit .
 any  =>  .
}

lexer read_scenario{

 Given => Given{ call read_given};.
 And => And {call read_given_and}; .
 When =>  When {call read_when}; .
 Then =>  Then {call read_then}; .
 any => .;

 Scenario \: => rewind exit .
}


BEGIN{
 Feature \: => Feature{ double_quote call read_descr double_quote ; }; .
 Scenario \: => Scenario{ 
                 title{ label blank __ blank title = double_quote call read_descr double_quote ;};
                 call read_scenario 
                }; .
 any => .
}

%%