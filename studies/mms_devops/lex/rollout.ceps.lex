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
 step ident (any,any) \. =>  $1; job_definition{$1;check{$3;};run{$5;};}; .
 
 <--- => };markets{. 
 
 market ident \. => market{$1;};.

 tuollor\. => };};.
 any =>.
}

%%

