(* MinCamlの構文を表現  *)
type t =
  | Unit
  | Bool of bool
  | Int of int
  | Float of float
  | Not of t
  | Neg of t
  | Add of t * t
  | Sub of t * t
  | Mul of t * t
  | Div of t * t
  | Xor of t * t
  | Or of t * t
  | And of t * t
  | Sll of t * t
  | Srl of t * t
  | FNeg of t
  | FAdd of t * t
  | FSub of t * t
  | FMul of t * t
  | FDiv of t * t
  | Eq of t * t
  | LE of t * t
  | If of t * t * t
  | Let of (Id.t * Type.t) * t * t
  | LetDef of (Id.t * Type.t) * t
  | Var of Id.t
  | LetRec of fun_def * t
  | LetRecDef of fun_def
  | App of t * t list
  | Tuple of t list
  | LetTuple of (Id.t * Type.t) list * t * t
  | Array of t * t
  | I2F of t
  | F2I of t
  | SQRT of t
  | FABS of t
  | In of t
  | Out of t
  | Get of t * t
  | Put of t * t * t
and fun_def = { name : Id.t * Type.t; args : (Id.t * Type.t) list; body : t }
