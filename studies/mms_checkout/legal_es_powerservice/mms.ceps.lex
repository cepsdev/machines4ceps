%ext_mms
%ignore_ws
%%

BEGIN{

 spec ident => use_case{$1;.
 steps => steps {.
 expectations => };expect{. 
 ceps => };};.
 create new basket => Step blank new_basket; new_basket;.
 submit basket => Step blank submit_basket; submit_basket;.
 insert articles such that => Step blank enter_items;enter_items ( power_service == 1 , total_price > 400, size <= 10 );.
 popup => expect{popup_show;};.

 any => ; . 


}

%%

