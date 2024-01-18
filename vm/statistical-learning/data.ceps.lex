%ext_data
%ignore_ws
%encode_line_feed_as_endl
%%
BEGIN{
    ident ident ident ident ident ident ident ident ident => / data_written=0;/.
    endl | data_written => }; / row_start=1; / .
    endl | !data_written =>  / row_start=1; / .
    
    double|row_start => row{ $0; /row_start=0;data_written=1;/.
    int|row_start =>  row{ $0; /row_start=0;data_written=1;/.
    \$|row_start  => row{ undef; /row_start=0;data_written=1;/.

    double|!row_start =>  $0; .
    int|!row_start =>   $0; .
    \$|!row_start  =>  undef; .
    \?|!row_start  =>  undef; .



    any => .
}
%%