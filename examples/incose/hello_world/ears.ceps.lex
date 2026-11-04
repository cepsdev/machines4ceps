// ears.ceps.lex - EARS requirement sentences to ceps AST.
//
// The grammar of SPEC.md section 3, one rule per accepted pattern. It is generic over
// EARS documents: it contains no vocabulary from any particular problem domain.
//
// Reading the rules: the left-hand side is a token pattern, `ident` matches any
// identifier, `\.` is a literal period, and $N refers to the Nth matched token counting
// from zero - punctuation included. The right-hand side is emitted as ceps source.
//
// Note `transition to` rather than `enter` as the mode verb: the pattern language
// reserves `enter` and `exit` for its own on-enter/on-exit matching, so they cannot
// appear as literals in a requirement sentence. See SPEC.md section 3.3.

%ext_req
%ignore_ws
%%

BEGIN{

 // P1 - Ubiquitous: initialisation.
 //   The <system> shall initialise in the <state> state.
 ident - integer The ident shall initialise in the ident state \. =>
   incose_init{
     req{double_quote $0 - $2 double_quote;};
     tag{$0 _ $2;};
     system{$4;};
     state{$9;};
   }; .

 // P2 - State-driven and event-driven (EARS "complex"), transition response.
 //   While the <system> is <state>, when <trigger> is received,
 //   the <system> shall transition to the <state> state.
 ident - integer While the ident is ident , when ident is received ,
   the ident shall transition to the ident state \. =>
   incose_transition{
     req{double_quote $0 - $2 double_quote;};
     tag{$0 _ $2;};
     system{$5;};
     from{$7;};
     on{$10;};
     to{$20;};
     trace{print(double_quote [ $0 - $2 ] blank $7 blank on blank $10 blank to blank $20 \n double_quote);};
   }; .

 // P4 - Unwanted behaviour. Same model element as P2; the distinction is recorded so
 //      the trace can name it, because "this transition exists to handle something that
 //      should not happen" is information a reviewer wants.
 //   While the <system> is <state>, if <trigger> is received,
 //   then the <system> shall transition to the <state> state.
 ident - integer While the ident is ident , if ident is received ,
   then the ident shall transition to the ident state \. =>
   incose_transition{
     req{double_quote $0 - $2 double_quote;};
     tag{$0 _ $2;};
     system{$5;};
     from{$7;};
     on{$10;};
     to{$21;};
     unwanted{true;};
     trace{print(double_quote [ $0 - $2 ] blank $7 blank on blank $10 blank to blank $21 blank ( unwanted blank behaviour ) \n double_quote);};
   }; .

 // P3 - State-driven and event-driven, action response. The action is named
 //      <verb>_<object>, so "shall illuminate the hazard_lamp" yields
 //      illuminate_hazard_lamp. Per D3 this is the default reading of any response that
 //      is not `transition to`.
 //   While the <system> is <state>, when <trigger> is received,
 //   the <system> shall <verb> the <object>.
 ident - integer While the ident is ident , when ident is received ,
   the ident shall ident the ident \. =>
   incose_action{
     req{double_quote $0 - $2 double_quote;};
     tag{$0 _ $2;};
     system{$5;};
     from{$7;};
     on{$10;};
     act{$17 _ $19;};
     trace{print(double_quote [ $0 - $2 ] blank $7 blank on blank $10 blank does blank $17 _ $19 \n double_quote);};
   }; .

 // P5 - Event-driven with no precondition. Applies in every declared state.
 //   When <trigger> is received, the <system> shall transition to the <state> state.
 ident - integer When ident is received ,
   the ident shall transition to the ident state \. =>
   incose_global{
     req{double_quote $0 - $2 double_quote;};
     tag{$0 _ $2;};
     system{$9;};
     on{$4;};
     to{$14;};
     trace{print(double_quote [ $0 - $2 ] blank any blank state blank on blank $4 blank to blank $14 \n double_quote);};
   }; .

 // D5 - Loudness. An identifier whose sentence matched none of the patterns above. The
 //      sentence itself is then consumed by `any` below and discarded, but the
 //      requirement is named in the conformance report rather than vanishing.
 //
 //      This rule must stay last: it is a prefix of every rule above it.
 ident - integer =>
   incose_nonconforming{
     req{double_quote $0 - $2 double_quote;};
     report{print(double_quote [ $0 - $2 ] blank NON - CONFORMING : blank matches blank no blank accepted blank pattern , blank no blank model blank element blank generated \n double_quote);};
   }; .

 any => .
}

%%
