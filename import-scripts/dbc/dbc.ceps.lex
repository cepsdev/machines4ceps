%ext_dbc
%ignore_ws
%%

lexer read_signal_signedness{
 integer + => signed{1;}; .
 integer - => unsigned{0;}; .
 else => rewind exit .
}

lexer read_signal_factor_offset{
 ( any, any ) => scale{$1;}; offset{$3;};.
 ( - any, any ) => scale{-$2;}; offset{$4;};.
 ( any, - any ) => scale{$1;}; offset{-$4;};.
 ( - any, - any ) => scale{-$2;}; offset{-$5;};.
 else => rewind exit .
}


lexer read_signal_min_max{
 [ any \| any ] => min{$1;}; max{$3;};.
 [ - any \| any ] => min{-$2;}; max{$4;};.
 [ any \| - any ] => min{$1;}; max{-$4;};.
 [ - any \| - any ] => min{-$2;}; max{-$5;};.
 else => rewind exit .
}

lexer read_signal_unit{
 string => unit{$0;}; .
 else => rewind exit .
}

lexer read_id_list{
 ident , => $0; .
 ident | !read_id_list_end => $0; /read_id_list_end=1;/ .
 any | read_id_list_end => rewind exit .
}

lexer read_signal {
 integer \| integer @  => blank start{$0;}; 
                         width{$2;}; 
                         call read_signal_signedness endl
                         blank call read_signal_factor_offset endl
                         blank call read_signal_min_max endl
                         blank call read_signal_unit endl
                         blank receiver{ /read_id_list_end=0;/ call read_id_list};.
 else => rewind exit .
}

lexer read_signals{
 SG_ ident M :  => sig{endl 
                 blank multiplexor{1;}; endl
                 blank name{$1;}; endl 
                 call read_signal endl}; 
                 endl.
 SG_ ident ident :  => sig{endl 
                 blank multiplexed{$2;}; endl
                 blank name{$1;}; endl 
                 call read_signal endl}; 
                 endl.                 
 SG_ ident  : => sig{endl 
                 blank name{$1;}; endl 
                 call read_signal endl}; 
                 endl.
               
 else => rewind exit .
}

lexer read_value_description_mapping{
 ; => exit .
 integer string => blank entry{index{$0;};value{$1;};}; endl.
 else => rewind exit .
}

lexer skip_ns_enumeration{
 : => exit .
 any => .
}

lexer read_sender_list{
 any ; => $0 ; exit .
 any , => $0 ; .
}

BEGIN{
 VERSION string => .
 NS_ :  => call skip_ns_enumeration.
 BO_ integer VECTOR__INDEPENDENT_SIG_MSG : integer ident => dbc_msg_technical {endl
  canid{ double_quote $1 double_quote ; };
  id{$2;};
  len{$4;};
  sender{$5;};endl
  call read_signals
 }; endl .

 BO_ integer ident : integer ident => dbc_msg {endl
  canid{ double_quote $1 double_quote ; };
  id{$2;};
  len{$4;};
  sender{$5;};endl
  call read_signals
 }; endl .
 
 CM_ SG_ any ident string ; => dbc_signal_comment$2{sig{name{$3;};comment{$4;};};};endl .
 VAL_ CAT_DEF_ => .
 VAL_ any ident => endl dbc_value_description_mapping$1{ sig{name{$2;}; call read_value_description_mapping }; }; endl.
 BO_TX_BU_ any : => dbc_sender_ofmsg{can_id{ double_quote $1 double_quote ; }; sender{call read_sender_list}; }; .
 BA_ "GenMsgCycleTime" BO_ integer integer ; => dbc_cycle_def{canid{double_quote $3 double_quote ;};interval{$4;};}; .
 any => .
}

%%
