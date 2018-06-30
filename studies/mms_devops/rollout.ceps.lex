%ext_rll
%ignore_ws
%%

BEGIN{

 name ident => rollout{name{$1;};steps{.

 step ident ident ident \. =>  $1$2$3;.
 step ident ident \. =>  $1$2; .
 step ident \. =>  $1; .
 
 \. => };};. 
 
 any => . 


}

%%

