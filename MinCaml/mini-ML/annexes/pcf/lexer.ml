# 10 "lexer.mll"
 
  open Parseutils
  open Parser        (* The type token is defined in parser.mli *)
  exception Eof

# 8 "lexer.ml"
let __ocaml_lex_tables = {
  Lexing.lex_base =
   "\000\000\241\255\242\255\244\255\245\255\075\000\001\000\248\255\
    \249\255\250\255\251\255\252\255\253\255\000\000\014\000\243\255\
    \247\255\002\000\254\255\006\000\255\255";
  Lexing.lex_backtrk =
   "\255\255\255\255\255\255\255\255\255\255\009\000\014\000\255\255\
    \255\255\255\255\255\255\255\255\255\255\001\000\000\000\255\255\
    \255\255\255\255\255\255\001\000\255\255";
  Lexing.lex_default =
   "\001\000\000\000\000\000\000\000\000\000\255\255\255\255\000\000\
    \000\000\000\000\000\000\000\000\000\000\255\255\255\255\000\000\
    \000\000\018\000\000\000\255\255\000\000";
  Lexing.lex_trans =
   "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\003\000\004\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \003\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \013\000\012\000\015\000\009\000\019\000\008\000\010\000\020\000\
    \014\000\014\000\014\000\014\000\014\000\014\000\014\000\014\000\
    \014\000\014\000\000\000\006\000\016\000\007\000\014\000\014\000\
    \014\000\014\000\014\000\014\000\014\000\014\000\014\000\014\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\011\000\000\000\000\000\005\000\
    \000\000\005\000\005\000\005\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\000\000\000\000\
    \000\000\000\000\005\000\000\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \002\000\000\000\255\255\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000";
  Lexing.lex_check =
   "\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\000\000\000\000\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \000\000\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \000\000\000\000\013\000\000\000\017\000\000\000\000\000\019\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\255\255\000\000\006\000\000\000\014\000\014\000\
    \014\000\014\000\014\000\014\000\014\000\014\000\014\000\014\000\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\000\000\255\255\255\255\000\000\
    \255\255\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\
    \000\000\000\000\000\000\005\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\255\255\255\255\
    \255\255\255\255\005\000\255\255\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\005\000\005\000\
    \005\000\005\000\005\000\005\000\005\000\005\000\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \000\000\255\255\017\000\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\255\
    \255\255\255\255\255\255\255\255";
  Lexing.lex_base_code =
   "";
  Lexing.lex_backtrk_code =
   "";
  Lexing.lex_default_code =
   "";
  Lexing.lex_trans_code =
   "";
  Lexing.lex_check_code =
   "";
  Lexing.lex_code =
   "";
}

let rec token lexbuf =
   __ocaml_lex_token_rec lexbuf 0
and __ocaml_lex_token_rec lexbuf __ocaml_lex_state =
  match Lexing.engine __ocaml_lex_tables __ocaml_lex_state lexbuf with
      | 0 ->
let
# 19 "lexer.mll"
                  n
# 130 "lexer.ml"
= Lexing.sub_lexeme lexbuf lexbuf.Lexing.lex_start_pos lexbuf.Lexing.lex_curr_pos in
# 19 "lexer.mll"
                    ( INT(int_of_string n) )
# 134 "lexer.ml"

  | 1 ->
# 20 "lexer.mll"
                    ( LPAREN )
# 139 "lexer.ml"

  | 2 ->
# 21 "lexer.mll"
                    ( RPAREN )
# 144 "lexer.ml"

  | 3 ->
# 22 "lexer.mll"
                    ( FUN )
# 149 "lexer.ml"

  | 4 ->
# 23 "lexer.mll"
                    ( DOT )
# 154 "lexer.ml"

  | 5 ->
# 24 "lexer.mll"
                    ( ADD )
# 159 "lexer.ml"

  | 6 ->
# 25 "lexer.mll"
                    ( SUB )
# 164 "lexer.ml"

  | 7 ->
# 26 "lexer.mll"
                    ( EQ )
# 169 "lexer.ml"

  | 8 ->
# 27 "lexer.mll"
                    ( END )
# 174 "lexer.ml"

  | 9 ->
let
# 28 "lexer.mll"
           lxm
# 180 "lexer.ml"
= Lexing.sub_lexeme lexbuf lexbuf.Lexing.lex_start_pos lexbuf.Lexing.lex_curr_pos in
# 28 "lexer.mll"
                    ( match lxm with 
	                  | "ifz" -> IFZ
	                  | "then" -> THEN
	                  | "else" -> ELSE
	                  | "let" -> LET
	                  | "letrec" -> LETREC
	                  | "in" -> IN
	                  | _ -> IDENT(lxm) )
# 191 "lexer.ml"

  | 10 ->
# 36 "lexer.mll"
                     ( (Lexing.new_line lexbuf) ; (token lexbuf) )
# 196 "lexer.ml"

  | 11 ->
# 37 "lexer.mll"
                     ( token lexbuf )
# 201 "lexer.ml"

  | 12 ->
# 38 "lexer.mll"
                    ( comment lexbuf )
# 206 "lexer.ml"

  | 13 ->
# 39 "lexer.mll"
                    ( EOF )
# 211 "lexer.ml"

  | 14 ->
let
# 40 "lexer.mll"
        lxm
# 217 "lexer.ml"
= Lexing.sub_lexeme_char lexbuf lexbuf.Lexing.lex_start_pos in
# 40 "lexer.mll"
                    ( raise (Parse_Exception (Printf.sprintf "Unexpected character: %c"  lxm,  Parseutils.pos())) )
# 221 "lexer.ml"

  | __ocaml_lex_state -> lexbuf.Lexing.refill_buff lexbuf;
      __ocaml_lex_token_rec lexbuf __ocaml_lex_state

and comment lexbuf =
   __ocaml_lex_comment_rec lexbuf 17
and __ocaml_lex_comment_rec lexbuf __ocaml_lex_state =
  match Lexing.engine __ocaml_lex_tables __ocaml_lex_state lexbuf with
      | 0 ->
# 44 "lexer.mll"
       ( token lexbuf )
# 233 "lexer.ml"

  | 1 ->
# 45 "lexer.mll"
       ( comment lexbuf )
# 238 "lexer.ml"

  | __ocaml_lex_state -> lexbuf.Lexing.refill_buff lexbuf;
      __ocaml_lex_comment_rec lexbuf __ocaml_lex_state

;;

