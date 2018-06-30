%ext_rsc
%ignore_ws
%%

BEGIN{
 scenario => Simulation{.

 Given rollout ident => Start{$2;};.

 after any seconds any => start_timer($1*s, $3); .

 \. => }; .
 any => . 
}

%%

