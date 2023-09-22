%ext_xsd
%ignore_ws
%%


lexer read_transitions{
 [ * ] --> ident => t{Initial;$6; };. 
 ident --> ident => t{ $0  ;  $4  ;};.
 ident --> [ * ] => t{$0;Final; };. 
 any => .
}


BEGIN{
 <xs:element name=string type=string/> 
  => element{ name{ $6; }; type{}; };.
 any => .
}

%%