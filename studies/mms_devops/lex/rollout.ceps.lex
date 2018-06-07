%ext_rll
%ignore_ws
%%

BEGIN{

 rollout => rollout{ .

 name ident  => name{$1;};.

 ---> => steps{ .

 step ident ident ident \. =>  $1$2$3;.
 step ident ident \. =>  $1$2; .
 step ident \. =>  $1; .
 
 <--- => };markets{. 
 
 market ident \. => market{$1;};.

 tuollor\. => };};.
 any =>.
}

%%

