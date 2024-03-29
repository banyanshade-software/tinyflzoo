/* MIRANDA TRANS */
/* performs translation to combinatory logic */

/**************************************************************************
 * Copyright (C) Research Software Limited 1985-90.  All rights reserved. *
 * The Miranda system is distributed as free software under the terms in  *
 * the file "COPYING" which is included in the distribution.              *
 *                                                                        *
 * Revised to C11 standard and made 64bit compatible, January 2020        *
 *------------------------------------------------------------------------*/

#include "data.h"
#include "big.h"
#include "lex.h"

 /* miscellaneous declarations  */
extern word nill,Void;
extern word listdiff_fn,count_fn,from_fn;
extern word diagonalise,concat;
extern word lastname,initialising;
extern word current_id,echoing;
extern word errs;
word newtyps=NIL;  /* list of typenames declared in current script */
word SGC=NIL; /* list of user defined sui-generis constructors */
#define sui_generis(k) (/* k==Void|| */ member(SGC,k))
                       /* 3/10/88 decision to treat `()' as lifted */
static word abshfnck(word,word);
static word abstr(word,word);
static word combine(word,word);
static void decl1(word,word);
static word fixrepeats(word);
static word getrel(word,word);
static word here_inf(word);
static word imageless(word,word,word);
static word invgetrel(word,word);
static word leftfactor(word);
static word less(word,word);
static word less1(word,word);
static word liscomb(word,word);
static word makeshow(word,word);
static word mklazy(word);
static word mkshowt(word,word);
static word mktuple(word);
static void nameclash(word);
static int nclchk(word,word,word);
static word new_mklazy(word);
static word primconstr(word);
static void respec_error(word);
static word scanpattern(word,word,word,word);
static word sort(word);
static word translet(word,word);
static word transletrec(word,word);
static word transtries(word,word);
static word transzf(word,word,word);
static word _codegen(word x);

static word abstract(word x, word e) /* abstraction of template x from compiled expression e */
{
    switch(tag[x]) {
        case ID:
            if(isconstructor(x))
                return(sui_generis(x)?ap(K,e):
                       ap2(Ug,primconstr(x),e));
            else return(abstr(x,e));
        case CONS:
            if(hd[x]==CONST)
                if(tag[tl[x]]==INT)return(ap2(MATCHINT,tl[x],e));
                else return(ap2(MATCH,tl[x]==NILS?NIL:tl[x],e));
                else return(ap(U_,abstract(hd[x],abstract(tl[x],e))));
        case TCONS:
        case PAIR: /* tuples */
            return(ap(U,abstract(hd[x],abstract(tl[x],e))));
        case AP:
            if(sui_generis(head(x)))
                return(ap(Uf,abstract(hd[x],abstract(tl[x],e))));
            if(tag[hd[x]]==AP&&hd[hd[x]]==PLUS) /* n+k pattern */
                return(ap2(ATLEAST,tl[hd[x]],abstract(tl[x],e)));
            while(tag[x]==AP)
            { e= abstract(tl[x],e);
                x= hd[x]; }
            /* now x must be a constructor */
        default: ;
    }
    if(isconstructor(x))
        return(ap2(Ug,primconstr(x),e));
    printf("error in declaration of \"%s\", undeclared constructor in pattern: ",
           get_id(current_id));  /* something funny here - fix later */
    out(stdout,x);
    printf("\n");
    return(NIL);
}

static word primconstr(word x)
{
    x=id_val(x);
    while(tag[x]!=CONSTRUCTOR) x=tl[x];
    return(x);
  /* => constructor values are of the form TRY f k where k is the
     original constructor value, and ! constructors are of the form
     MKSTRICT i k */
}

word memb(word l, word x)  /* tests if x is a member of list "l" - used in testing for
              repeated names - see rule for "v2" in rules.y */
{
    if(tag[x]==TVAR)  /* type variable! */
    while(l!=NIL&&!eqtvar(hd[l],x))l= tl[l];
  else while(l!=NIL&&hd[l]!=x)l= tl[l];
  return(l!=NIL);
}

static word abstr(word x, word e)   /*  "bracket abstraction" of variable x from code e */
{
    switch(tag[e]) {
        case TCONS:
        case PAIR:
        case CONS: return(liscomb(abstr(x,hd[e]),abstr(x,tl[e])));
        case AP: if(hd[e]==BADCASE||hd[e]==CONFERROR)
            return(ap(K,e)); /* don't go inside error info */
            return(combine(abstr(x,hd[e]),abstr(x,tl[e])));
        case LAMBDA:
        case LET:
        case LETREC:
        case TRIES:
        case LABEL:
        case SHOW:
        case LEXER:
        case SHARE: fprintf(stderr,"impossible event in abstr (tag=%d)\n",tag[e]),
            exit(1);
        default: if(x==e||isvar_t(x)&&isvar_t(e)&&eqtvar(x,e))
            return(I); /* see note */
            return(ap(K,e));
    }
} /* note - we allow abstraction wrt tvars - see genshfns() */

#define mkindex(i)  ((i)<256?(i):make(INT,i,0))
   /* will fall over if i >= IBASE */

static word abstrlist(word x, word e)  /* abstraction of list of variables x from code e */
{
    switch(tag[e]) {
        case TCONS:
        case PAIR:
        case CONS:
            return(liscomb(abstrlist(x,hd[e]),abstrlist(x,tl[e])));
        case AP:
            if(hd[e]==BADCASE||hd[e]==CONFERROR)
                return(ap(K,e)); /* don't go inside error info */
            else return(combine(abstrlist(x,hd[e]),abstrlist(x,tl[e])));
        case LAMBDA: case LET: case LETREC: case TRIES: case LABEL: case SHOW:
        case LEXER:
        case SHARE:
            fprintf(stderr,"impossible event in abstrlist (tag=%d)\n",tag[e]),
            exit(1);
        default: {
            word i=0;
            while(x!=NIL&&hd[x]!=e)i++,x=tl[x];
            if(x==NIL)return(ap(K,e));
            return(ap(SUBSCRIPT,mkindex(i)));
        }
    }
}

word rv_script=0; /* flags readvals in use (for garbage collector) */
                  /* used in steer.c */

word codegen(word x)
{
    if (_dump_compile) {
        printf("++++ codegen %lx :\n", x);
        out(stdout,x);
        printf("\n");
    }
    word r = _codegen(x);
    if (_dump_compile) {
        printf("---- compiled code %lx:\n", r);
        out(stdout,r);
        printf("\n");
    }
    return r;
}

static word _codegen(word x) /* returns expression x with abstractions performed */
{
    extern word commandmode,cook_stdin,common_stdin,common_stdinb,rv_expr;
    switch(tag[x]) {
        case AP:
            if(commandmode /* beware of corrupting lastexp */
               && x!=cook_stdin&&x!=common_stdin&&x!=common_stdinb) /* but share $+ $- */
                return(make(AP,_codegen(hd[x]),_codegen(tl[x])));
            if(tag[hd[x]]==AP&&hd[hd[x]]==APPEND&&tl[hd[x]]==NIL)
                return(_codegen(tl[x])); /* post typecheck reversal of HR bug fix */
            hd[x]=_codegen(hd[x]); tl[x]=_codegen(tl[x]);
            /* otherwise do in situ */
            return(tag[hd[x]]==AP&&hd[hd[x]]==G_ALT?leftfactor(x):x);
        case TCONS:
        case PAIR:
            return(make(CONS,_codegen(hd[x]),_codegen(tl[x])));
        case CONS:
            if(commandmode)
                return(make(CONS,_codegen(hd[x]),_codegen(tl[x])));
            /* otherwise do in situ (see declare) */
            hd[x]=_codegen(hd[x]);
            tl[x]=_codegen(tl[x]);
            return(x);
        case LAMBDA:
            return(abstract(hd[x],_codegen(tl[x])));
        case LET:
            return(translet(hd[x],tl[x]));
        case LETREC:
            return(transletrec(hd[x],tl[x]));
        case TRIES:
            return(transtries(hd[x],tl[x]));
        case LABEL:
            return(_codegen(tl[x]));
        case SHOW:
            return(makeshow(hd[x],tl[x]));
        case LEXER: {
            word r=NIL,uses_state=0;;
            while(x!=NIL) {
                word rule=abstr(mklexvar(0),_codegen(tl[tl[hd[x]]]));
                rule=abstr(mklexvar(1),rule);
                if (!(tag[rule]==AP&&hd[rule]==K)) uses_state=1;
                r=cons(cons(hd[hd[x]], /* start condition stuff */
                            cons(ap(hd[tl[hd[x]]],NIL),   /* matcher [] */
                                 rule)),
                       r);
                x=tl[x];
            }
            if(!uses_state) { /* strip off (K -) from each rule */
                for (x=r;x!=NIL;x=tl[x]) tl[tl[hd[x]]]=tl[tl[tl[hd[x]]]];
                r = ap(LEX_RPT,ap(LEX_TRY,r));
            }
            else r = ap(LEX_RPT1,ap(LEX_TRY1,r));
            return(ap(r,0));
        } /* 0 startcond */
        case STARTREADVALS:
            if(ispoly(tl[x])) {
                extern word cook_stdin,ND;
                printf("type error - %s used at polymorphic type :: [",
                       cook_stdin&&x==hd[cook_stdin]?"$+":"readvals or $+");
                out_type(redtvars(tl[x])),printf("]\n");
                polyshowerror=1;
                if(current_id)
                    ND=add1(current_id,ND),
                    id_type(current_id)=wrong_t,
                    id_val(current_id)=UNDEF;
                if(hd[x])sayhere(hd[x],1);
            }
            if (commandmode) rv_expr=1;
            else rv_script=1;
            return(x);
        case SHARE:
            if(tl[x]!= -1) /* arbitrary flag for already visited */
                hd[x]=_codegen(hd[x]),tl[x]= -1;
            return(hd[x]);
        default:
            if(x==NILS) return(NIL);
            return(x); /* identifier, private name, or constant */
    }
}

int lfrule=0; /* used in steer.c */

static word leftfactor(word x)
/* grammar optimisations - x is of the form ap2(G_ALT,...)
   G_ALT(G_SEQ a b) a  => G_SEQ a (G_ALT b G_UNIT)
   G_ALT(G_SEQ a b)(G_SEQ a c) => G_SEQ a (G_ALT b c)
   G_ALT(G_SEQ a b)(G_ALT a d) => G_ALT(G_SEQ a (G_ALT b G_UNIT)) d
   G_ALT(G_SEQ a b)(G_ALT(G_SEQ a c) d) => G_ALT(G_SEQ a (G_ALT b c)) d
*/
{
    word a,b,c,d;
    if(tag[c=tl[hd[x]]]==AP&&tag[hd[c]]==AP&&hd[hd[c]]==G_SEQ)
        a=tl[hd[c]],b=tl[c];
    else return(x);
    if(same(a,d=tl[x]))
    { hd[x]=ap(G_SEQ,a), tl[x]=ap2(G_ALT,b,G_UNIT); lfrule++;
        /* printob("rule1: ",x); */
        return(x);
    }
    if(tag[d]==AP&&tag[hd[d]]==AP)
        c=hd[hd[d]];
    else return(x);
    if(c==G_SEQ&&same(a,tl[hd[d]])) { c=tl[d],
        hd[x]=ap(G_SEQ,a), tl[x]=leftfactor(ap2(G_ALT,b,c)); lfrule++;
        /* printob("rule2: ",x); */
        return(x);
    }
    if(c!=G_ALT) return(x);
    if(same(a,c=tl[hd[d]])) {
        d=tl[d];
        hd[x]=ap(G_ALT,ap2(G_SEQ,a,ap2(G_ALT,b,G_UNIT)));
        tl[x]=d; lfrule++;
        /* printob("rule3: ",x); */
        return(leftfactor(x));
    }
    if (tag[c]==AP && tag[hd[c]]==AP && hd[hd[c]]==G_SEQ
        && same(a,tl[hd[c]])) {
        c=tl[c],d=tl[d],
        hd[x]=ap(G_ALT,ap2(G_SEQ,a,leftfactor(ap2(G_ALT,b,c))));
        tl[x]=d; lfrule++;
        /* printob("rule4: ",x); */
        return(leftfactor(x));
    }
    return(x);
}

word same(word x, word y) /* structural equality */
{
    if (x==y) return(1);
    if (tag[x]==ATOM||tag[y]==ATOM||tag[x]!=tag[y]) return(0);
    if (tag[x]<INT) return(hd[x]==hd[y]&&tl[x]==tl[y]);
    if (tag[x]>STRCONS) return(same(hd[x],hd[y])&&same(tl[x],tl[y]));
    return (hd[x]==hd[y] && same(tl[x],tl[y])); /* INT..STRCONS */
}

static word was_poly;
int polyshowerror=0;

static word makeshow(word here, word type)
{
    word f;
    extern word ND;
    was_poly=0;
    f=mkshow(0,0,type);
    /* printob("showfn=",f); */ /* DEBUG */
    if (here && was_poly) {
        extern char *current_script;
        printf("type error in definition of %s\n",get_id(current_id));
        sayhere(here,0);
        printf(" use of \"show\" at polymorphic type ");
        out_type(redtvars(type));
        putchar('\n');
        id_type(current_id)=wrong_t;
        id_val(current_id)=UNDEF;
        polyshowerror=1;
        ND=add1(current_id,ND);
        was_poly=0;
    }
    return(f);
}

word mkshow(word s, word p, word t) /* build a show function appropriate to type t */
                                           /* p is precedence - 0 for top level, 1 for internal */
                                           /* s flags special case invoked from genshfns */
{
    extern word shownum1,showbool,showchar,showlist,showstring,showparen,
    showvoid,showpair,showfunction,showabstract,showwhat;
    word a=NIL;
    while(tag[t]==AP)a=cons(tl[t],a),t=hd[t];
    switch(t) {
        case num_t: return(p?shownum1:SHOWNUM);
        case bool_t: return(showbool);
        case char_t: return(showchar);
        case list_t: if(hd[a]==char_t)return(showstring);
            return(ap(showlist,mkshow(s,0,hd[a])));
        case comma_t: return(ap(showparen,ap2(showpair,mkshow(s,0,hd[a]),
                                              mkshowt(s,hd[tl[a]]))));
        case void_t: return(showvoid);
        case arrow_t:return(showfunction);
        default:
            if(tag[t]==ID) {
                word r=t_showfn(t);
                if(r==0) /* abstype without show function */
                    return(showabstract);
                if(r==showwhat) /* dont apply to parameter showfns */
                    return(r);
                while(a!=NIL)r=ap(r,mkshow(s,1,hd[a])),a=tl[a];
                if(t_class(t)==algebraic_t)r=ap(r,p);
                return(r);
                /* note that abstype-showfns have only one precedence
                 and show their components (if any) at precedence 1
                 - if the latter is a problem could do parenthesis
                 stripping */
            }
            if (isvar_t(t)) {
                if(s)return(t); /* see genshfns */
                was_poly=1;
                return(showwhat);
            }
            /* arbitrary - could be any strict function */
            if(tag[t]==STRCONS) { /* pname */ /* DEBUG */
                printf("warning - mkshow applied to suppressed type\n");
                return(showwhat);
            } else {
                printf("impossible event in mkshow ("),
                out_type(t), printf(")\n");
                return(showwhat);
            }
    }
}

static word mkshowt(word s, word t) /* t is a (possibly singleton) tuple type */
                                    /* flags special call from genshfns */
{
    extern word showpair;
    if(tl[t]==void_t)return(mkshow(s,0,tl[hd[t]]));
    return(ap2(showpair,mkshow(s,0,tl[hd[t]]),mkshowt(s,tl[t])));
}

word algshfns=NIL; /* list of showfunctions for all algebraic  types in scope
		     (list of pnames) - needed to make dumps */
             /* used in steer.c */

void genshfns(void) /* called after meta type check - create show functions for
                     algebraic types */
{
    word s;
    for (s=newtyps;s!=NIL;s=tl[s])
    if(t_class(hd[s])==algebraic_t) {
        word f=0,r=t_info(hd[s]); /* r is list of constructors */
        word ush= tl[r]==NIL&&member(SGC,hd[r])?Ush1:Ush;
        for(;r!=NIL;r=tl[r]) {
            word t=id_type(hd[r]),k=id_val(hd[r]);
            while(tag[k]!=CONSTRUCTOR) k=tl[k];/* lawful and !'d constructors*/
            /* k now holds constructor(i,hd[r]) */
            /* k=constructor(hd[k],datapair(get_id(tl[k]),0));*/
             /* this `freezes' the name of the constructor */
            /* incorrect, makes showfns immune to aliasing, should be
             done at mkshow time, not genshfn time - FIX LATER */
            while(isarrow_t(t))
                k=ap(k,mkshow(1,1,tl[hd[t]])),t=tl[t]; /* NB 2nd arg */
            k=ap(ush,k);
            while(iscompound_t(t))k=abstr(tl[t],k),t=hd[t];
            /* see kahrs.bug.m (this is the fix) */
            if(f)f=ap2(TRY,k,f);
            else f=k;
        }
        /* f~=0, placeholder types dealt with in specify() */
        pn_val(t_showfn(hd[s]))=f;
        algshfns=cons(t_showfn(hd[s]),algshfns);
    }
    else if (t_class(hd[s])==abstract_t) /* if showfn present check type is ok */
        if(t_showfn(hd[s]))
            if(!abshfnck(hd[s],id_type(t_showfn(hd[s]))))
                printf("warning - \"%s\" has type inappropriate for a show-function\n",
                       get_id(t_showfn(hd[s]))),t_showfn(hd[s])=0;
}

static word abshfnck(word t, word f) /* t is an abstype, is f right type for its showfn? */
{
    word n=t_arity(t),i=1;
    while(i<=n)
        if(isarrow_t(f)) {
            word h=tl[hd[f]];
            if(!(isarrow_t(h)&&isvar_t(tl[hd[h]])&&gettvar(tl[hd[h]])==i
                 &&islist_t(tl[h])&&tl[tl[h]]==char_t)) return(0);
            i++,f=tl[f];
        } else return(0);
    if(!(isarrow_t(f)&&islist_t(tl[f])&&tl[tl[f]]==char_t)) return(0);
    f=tl[hd[f]];
    while(iscompound_t(f)&&isvar_t(tl[f])&&gettvar(tl[f])==n--) f=hd[f];
    return(f==t);
}

static word transtries(word id, word x) /* x is a list of alternative values, in reverse order */
{
    word r,h=0,earliest;
    if(fallible(hd[x])) { /* add default last case */
        word oldn=tag[id]==ID?datapair(get_id(id),0):0;
        r=ap(BADCASE,h=cons(oldn,0));
        /* 0 is placeholder for here-info */
        /* oldn omitted if id is pattern - FIX LATER */ }
    else r=_codegen(earliest=hd[x]), x = tl[x];
    while(x!=NIL)r=ap2(TRY,_codegen(earliest=hd[x]),r), x=tl[x];
    if (h) tl[h]=hd[earliest]; /* first line-no is the best marker */
    return(r);
}

static word translet(word d, word e) /* compile block with body e and def d */
{
    word x=mklazy(d);
    return (ap(abstract(dlhs(x),_codegen(e)),_codegen(dval(x))));
}
/* nasty bug, codegen(dval(x)) was interfering with abstract(dlhs(x)...
   to fix made codegen on tuples be NOT in situ 20/11/88  */

static word transletrec(word dd, word e) /* better method,  using list indexing - Jan 88 */
{
    word lhs=NIL,rhs=NIL,pn=1;
    /* list of defs (x=e) is combined to listwise def `xs=es' */
    for(;dd!=NIL;dd=tl[dd]) {
        word x=hd[dd];
        if(tag[dlhs(x)]==ID)  /* couldn't be constructor, by grammar */
            lhs=cons(dlhs(x),lhs),
            rhs=cons(_codegen(dval(x)),rhs);
        else { word i=0,ids,p=mkgvar(pn++); /* see note 1 */
            x=new_mklazy(x); ids=dlhs(x);
            lhs=cons(p,lhs),rhs=cons(_codegen(dval(x)),rhs);
            for(;ids!=NIL;ids=tl[ids],i++)
            lhs=cons(hd[ids],lhs),
            rhs=cons(ap2(SUBSCRIPT,mkindex(i),p),rhs);
        }
    }
    if(tl[lhs]==NIL) /* singleton */
        return(ap(abstr(hd[lhs],_codegen(e)),ap(Y,abstr(hd[lhs],hd[rhs]))));
    return(ap(abstrlist(lhs,_codegen(e)),ap(Y,abstrlist(lhs,rhs))));
}



/* note 1: we here use the alternative `mklazy' transformation
   pat = e =>  x1=p!0;...;xn=p!(n-1);p=(lambda(pat)[xs])e|conferror;
   where p is a private name (need be unique only within a given letrec)
*/

static word mklazy(word d) /* transforms local p=e to ids=($p.ids)e|conferror */
{
    if(irrefutable(dlhs(d)))return(d);
    {
        word ids=mktuple(dlhs(d));
        if(ids==NIL){ printf("impossible event in mklazy\n"); return(d); }
        dval(d)=ap2(TRY,ap(lambda(dlhs(d),ids),dval(d)),
                    ap(CONFERROR,cons(dlhs(d),here_inf(dval(d)))));
        dlhs(d)=ids;
        return(d);
    }
}

static word new_mklazy(word d) /* transforms local p=e to ids=($p.ids)e|conferror
                                with ids a LIST (not tuple as formerly) */
{
    word ids=get_ids(dlhs(d));
    if(ids==NIL){ printf("impossible event in new_mklazy\n"); return(d); }
    dval(d)=ap2(TRY,ap(lambda(dlhs(d),ids),dval(d)),
                ap(CONFERROR,cons(dlhs(d),here_inf(dval(d)))));
    dlhs(d)=ids;
    return(d);
}

static word here_inf(word rhs) /* rhs is of form tries(id,val_list) */
{
    word x=tl[rhs];
    while(tl[x]!=NIL)x=tl[x]; /* find earliest alternative */
    return(hd[hd[x]]); /* hd[x] is of form label(here_info,value) */
}

word irrefutable(word x) /* x built from suigeneris constr's and (unrepeated) names */
{
    if(tag[x]==CONS)return(0); /* includes constants */
    if(isconstructor(x))return(sui_generis(x));
    if(tag[x]==ID)return(1);
    if(tag[x]==AP&&tag[hd[x]]==AP&&hd[hd[x]]==PLUS) /* n+k pattern */
        return(0);
    return(irrefutable(hd[x])&&irrefutable(tl[x]));
}

static word combine(word x, word y)
{
    word a,b,a1,b1;
    a= tag[x]==AP&&hd[x]==K;
    b= tag[y]==AP&&hd[y]==K;
    if(a&&b)return(ap(K,ap(tl[x],tl[y])));
    /* rule of K propagation */
    if(a&&y==I)return(tl[x]);
    /* rule 'eta */
    b1= tag[y]==AP&&tag[hd[y]]==AP&&hd[hd[y]]==B;
    if(a)if(b1)return(ap3(B1,tl[x],tl[hd[y]],tl[y])); else
    /* Mark Scheevel's new B1 introduction rule -- adopted Aug 83 */
        if(tag[tl[x]]==AP&&tag[hd[tl[x]]]==AP&&hd[hd[tl[x]]]==COND)
            return(ap3(COND,tl[hd[tl[x]]],ap(K,tl[tl[x]]),y));
        else return(ap2(B,tl[x],y));
    a1= tag[x]==AP&&tag[hd[x]]==AP&&hd[hd[x]]==B;
    if(b)if(a1)if(tag[tl[hd[x]]]==AP&&hd[tl[hd[x]]]==COND)
        return(ap3(COND,tl[tl[hd[x]]],tl[x],y));
    else return(ap3(C1,tl[hd[x]],tl[x],tl[y]));
    else return(ap2(C,x,tl[y]));
    if(a1)if(tag[tl[hd[x]]]==AP&&hd[tl[hd[x]]]==COND)
        return(ap3(COND,tl[tl[hd[x]]],tl[x],y));
    else return(ap3(S1,tl[hd[x]],tl[x],y));
    else return(ap2(S,x,y));
}

static word liscomb(word x, word y)  /* the CONSy analogue of "combine" */
{
    word a,b;
    a= tag[x]==AP&&hd[x]==K;
    b=  tag[y]==AP&&hd[y]==K;
    if(a&&b)return(ap(K,cons(tl[x],tl[y])));
    /* K propagation again */
    if(a)if(y==I)return(ap(P,tl[x])); /* eta P - new rule added 20/11/88 */
    else return(ap2(B_p,tl[x],y));
    if(b)return(ap2(C_p,x,tl[y]));
    return(ap2(S_p,x,y));
}

/* B_p,C_p,S_p are the CONSy analogues of B,C,S
   see MIRANDA REDUCE for their definitions */

word compzf(word e, word qq, word diag) /* compile a zf expression with body e and qualifiers qq
                                         (listed in reverse order); diag is 0 for sequential
                                         and 1 for diagonalising zf expressions */
{
    word hold=NIL,r=0,g1= -1; /* r is number of generators */
    while(qq!=NIL) {/* unreverse qualifier list */
        if(hd[hd[qq]]==REPEAT)qq=fixrepeats(qq);
        hold=cons(hd[qq],hold);
        if(hd[hd[qq]]==GUARD)r++; /* count filters */
        qq = tl[qq];
    }
    for (qq=hold;qq!=NIL && hd[hd[qq]]==GUARD; qq=tl[qq])  r--; /* less leading filters */
    if (hd[hd[hold]]==GENERATOR) g1=tl[tl[hd[hold]]]; /* rhs of 1st generator */
    e = transzf(e, hold, diag ? diagonalise : concat);
    /* diagonalise [ // ] comprehensions, but not [ | ] ones */
    if(diag)
        while(r--)e=ap(concat,e); /* see funny version of rule 3 below */
    return(e==g1?ap2(APPEND,NIL,e):e); /* test in g1 is to fix HR bug */
}
/* HR bug - if Rule 1 applied at outermost level, type info is lost
   eg [p|p<-3] ==> 3  (reported by Ham Richards, Nov 89)
*/

static word transzf(word e, word qq, word conc)  /* Bird and Wadler page 63 */
{
    word q,q2;
    if (qq==NIL) return(cons(e,NIL));
    q=hd[qq];
    if(hd[q]==GUARD)
        return(ap3(COND,tl[q],transzf(e,tl[qq],conc),NIL));
    if(tl[qq]==NIL)
        if(hd[tl[q]]==e&&isvariable(e))return(tl[tl[q]]); /* Rule 1 */
        else if(irrefutable(hd[tl[q]]))
            return(ap2(MAP,lambda(hd[tl[q]],e),tl[tl[q]])); /* Rule 2 */
        else /* Rule 2 warped for refutable patterns */
            return(ap2(FLATMAP,lambda(hd[tl[q]],cons(e,NIL)),tl[tl[q]]));
    q2=hd[tl[qq]];
    if(hd[q2]==GUARD)
        if(conc==concat) { /* Rule 3 */
            tl[tl[q]]=ap2(FILTER,lambda(hd[tl[q]],tl[q2]),tl[tl[q]]);
            tl[qq]=tl[tl[qq]];
            return(transzf(e,qq,conc));
        } else { /* funny [//] version of Rule 3 to avoid creating weak lists */
            e=ap3(COND,tl[q2],cons(e,NIL),NIL);
            tl[qq]=tl[tl[qq]];
            return(transzf(e,qq,conc));
        } /* plus wrap result with concat */
    return(ap(conc,transzf(transzf(e,tl[qq],conc),cons(q,NIL),conc)));
    /* Rule 4 */
}

static word fixrepeats(word qq)  /* expands multi-lhs generators in zf expressions */
{
    word q = hd[qq];
    word rhs = q;
    qq = tl[qq];
    while (hd[rhs]==REPEAT) rhs = tl[tl[rhs]];
    rhs = tl[tl[rhs]];  /* rhs now contains the common right hand side */
    while(hd[q]==REPEAT)  { qq = cons(cons(GENERATOR,cons(hd[tl[q]],rhs)),qq);
        q = tl[tl[q]];
    }
    return(cons(q,qq));
} /* EFFICIENCY PROBLEM - rhs gets re-evaluated for each lhs, fix later */
/* likewise re-typechecked, although this probably doesn't matter */

word lastlink(word x) /* finds last link of a list -- needed with zf body elision */
{
    while (tl[x]!=NIL) x=tl[x];
    return(x);
}

#define ischar(x) ((x)>=0&&(x)<=255)

word genlhs(word x) /* x is an expression found on the lhs of <- and genlhs returns
             the corresponding pattern */
{
    word hold;
    switch(tag[x]) {
        case AP:
            if(tag[hd[x]]==AP&&hd[hd[x]]==PLUS&&isnat(tl[x]))
                return(ap2(PLUS,tl[x],genlhs(tl[hd[x]])));  /* n+k pattern */
        case CONS:
        case TCONS:
        case PAIR:
            hold=genlhs(hd[x]); return(make(tag[x],hold,genlhs(tl[x])));
        case ID:
            if(member(idsused,x))return(cons(CONST,x));
            if(!isconstructor(x))idsused=cons(x,idsused); return(x);
        case INT: return(cons(CONST,x));
        case DOUBLE: syntax("floating point literal in pattern\n");
            return(nill);
        case ATOM: if(x==True||x==False||x==NILS||x==NIL||ischar(x))
            return(cons(CONST,x));
        default: syntax("illegal form on left of <-\n");
            return(nill);
    }
}

word speclocs=NIL; /* list of cons(id,hereinfo) giving location of spec for
		     ids both defined and specified - needed to locate errs
		     in meta_tcheck, abstr_mcheck */
            /* used in dta.c and steer.c */
word getspecloc(word x)
{
    word s=speclocs;
    while (s!=NIL&&hd[hd[s]]!=x) s=tl[s];
    return(s==NIL?id_who(x):tl[hd[s]]);
}

void declare(word x, word e)    /* translates  <pattern> = <exp>  at top level  */
{
    if (tag[x]==ID&&!isconstructor(x)) decl1(x,e);
    else {
        word bindings=scanpattern(x,x,share(tries(x,cons(e,NIL)),undef_t),
                                  ap(CONFERROR,cons(x,hd[e])));
        /* hd[e] is here-info */
        /* note creation of share node to force sharing on code generation
         and typechecking */
        if(bindings==NIL){
            errs=hd[e];
            syntax("illegal lhs for definition\n");
            return;
        }
        lastname=0;
        while(bindings!=NIL)  {
            word h;
            if(id_val(h=hd[hd[bindings]])!=UNDEF) {
                errs=hd[e]; nameclash(h); return;
            }
            id_val(h)=tl[hd[bindings]];
            if (id_who(h)!=NIL) speclocs=cons(cons(h,id_who(h)),speclocs);
            id_who(h)=hd[e]; /* here-info */
            if (id_type(h)==undef_t) addtoenv(h);
            bindings = tl[bindings];
        }
    }
}

static word scanpattern(word p, word x, word e, word fail) /* declare ids in x as components of `p=e', each as
                                                     n = ($p.n)e,  result is list of bindings */
{
    if(hd[x]==CONST||isconstructor(x)) return(NIL);
    if(tag[x]==ID) {
        word binding=
        cons(x,ap2(TRY,ap(lambda(p,x),e),fail));
        return(cons(binding,NIL));
    }
    if(tag[x]==AP&&tag[hd[x]]==AP&&hd[hd[x]]==PLUS) /* n+k pattern */
        return(scanpattern(p,tl[x],e,fail));
    return(shunt(scanpattern(p,hd[x],e,fail),scanpattern(p,tl[x],e,fail)));
}

word get_ids(word x) /* return list of names in pattern x (without repetitions) */
{
    if(hd[x]==CONST||isconstructor(x))return(NIL);
    if(tag[x]==ID)return(cons(x,NIL));
    if(tag[x]==AP&&tag[hd[x]]==AP&&hd[hd[x]]==PLUS) /* n+k pattern */
        return(get_ids(tl[x]));
    return(UNION(get_ids(hd[x]),get_ids(tl[x])));
}

static word mktuple(word x) /* extract tuple-structure of names from pattern x */
{
    if (hd[x]==CONST||isconstructor(x)) return(NIL);
    if(tag[x]==ID)return(x);
    if(tag[x]==AP&&tag[hd[x]]==AP&&hd[hd[x]]==PLUS) /* n+k pattern */
        return(mktuple(tl[x]));
    {
        word y=mktuple(tl[x]); x=mktuple(hd[x]);
        return(x==NIL?y:y==NIL?x:pair(x,y));
    }
}

static void decl1(word x, word e)  /* declare name x to have the value denoted by e */
{
    if(id_val(x)!=UNDEF&&lastname!=x) {
        errs=hd[e]; nameclash(x); return;
    }
    if(id_val(x)==UNDEF)  {
        id_val(x)= tries(x,cons(e,NIL));
        if(id_who(x)!=NIL)speclocs=cons(cons(x,id_who(x)),speclocs);
        id_who(x)= hd[e]; /* here-info */
        if(id_type(x)==undef_t)addtoenv(x);
    } else if (!fallible(hd[tl[id_val(x)]]))
        errs=hd[e],
        printf("%ssyntax error: unreachable case in defn of \"%s\"\n",
               echoing?"\n":"",get_id(x)),
        acterror();
    else tl[id_val(x)]= cons(e,tl[id_val(x)]);
    /* multi-clause definitions are composed as tries(id,rhs_list)
     where id is included purely for diagnostic purposes
     note that rhs_list is reversed - put right by code generation */
}

word fallible(word e) /* e is "fallible" rhs - if not sure, says yes */
{
    for(;;) {
        if(tag[e]==LABEL)e=tl[e];
        if(tag[e]==LETREC||tag[e]==LET)e=tl[e]; else
            if(tag[e]==LAMBDA)
                if(irrefutable(hd[e]))e=tl[e];
                else return(1); else
                    if(tag[e]==AP&&tag[hd[e]]==AP&&tag[hd[hd[e]]]==AP&&hd[hd[hd[e]]]==COND)
                        e=tl[e]; else
                            return(e==FAIL);   /* test for nested (COND a b FAIL) */
    }
} /* NOTE
     When an rhs contains FAIL as a result of compiling an elseless guard set
     it is of the form
	XX ::= ap3(COND,a,b,FAIL) | let[rec](def[s],XX) | lambda(pat,XX)
     an rhs is fallible if
	1) it is an XX, as above, or
	2) it is of the form lambda(pat1,...,lambda(patn,e)...)
	   where at least one of the patterns pati is refutable.
  */

/* combinator to select i'th out of n args *//*
word k(i,n)
int i,n;
{ if(i==1)return(n==1?I:n==2?K:ap2(B,K,k(1,n-1)));
  if(i==2&&n==2)return(KI); *//* redundant but saves space *//*
  return(ap(K,k(i-1,n-1)));
} */ /* not currently used */

#define arity_check if(t_arity(tf)!=arity)\
  printf("%ssyntax error: \
wrong number of parameters for typename \"%s\" (%ld expected)\n",\
          echoing?"\n":"",get_id(tf),t_arity(tf)),errs=here,acterror()

void decltype(word tf, word class, word info, word here)  /* declare a user defined type */
{
    word arity=0;
    extern word errs;
    while(tag[tf]==AP)arity++,tf=hd[tf];
    if (class==synonym_t && id_type(tf)==type_t && t_class(tf)==abstract_t && t_info(tf)==undef_t) {
        /* this is binding for declared but not yet bound abstract typename */
        arity_check;
        id_who(tf)=here;
        t_info(tf)=info;
        return;
    }
    if(class==abstract_t&&id_type(tf)==type_t&&t_class(tf)==synonym_t) {
        /* this is abstype declaration of already bound typename */
        arity_check;
        t_class(tf)=abstract_t;
        return;
    }
    if (id_val(tf)!=UNDEF) {
        errs=here; nameclash(tf); return;
    }
    if (class!=synonym_t) newtyps=add1(tf,newtyps);
    id_val(tf) = make_typ(arity,class==algebraic_t?make_pn(UNDEF):0,class,info);
    if(id_type(tf)!=undef_t) {
        errs=here; respec_error(tf); return;
    } else addtoenv(tf);
    id_who(tf)=here;
    id_type(tf)=type_t;
}

void declconstr(word x, word n, word t)  /* declare x to be constructor number n of type t */
  /* x must be an identifier */
{
    id_val(x)=constructor(n,x);
    if (n>>16)  {
        syntax("algebraic type has too many constructors\n"); return;
        
    }
    if (id_type(x)!=undef_t) {
        errs=id_who(x); respec_error(x); return;
    }
    else addtoenv(x);
    id_type(x) = t;
} /* the value of a constructor x is constructor(constr_tag,x)
     where constr_tag is a small natural number */

word block(word defs, word e, word keep) /* semantics of "where" - performs dependency analysis */
/* defs has form list(defn(pat,typ,val)), e is body of block */
/* if `keep' hold together as single letrec */
{
    word ids=NIL,deftoids=NIL,g=NIL,d;
    extern word SYNERR,detrop;
    /* return(letrec(defs,e));*/ /* release one semantics was just this */
    if(SYNERR)return(NIL);  /* analysis falls over on empty patterns */
    for(d=defs;d!=NIL;d=tl[d])  /* first collect all ids defined in block */
    { word x = get_ids(dlhs(hd[d]));
        ids=UNION(ids,x);
        deftoids=cons(cons(hd[d],x),deftoids);
    }
    defs=sort(defs);
    for(d=defs;d!=NIL;d=tl[d]) { /* now build dependency relation g */
        word x=intersection(deps(dval(hd[d])),ids),y=NIL;
        for(;x!=NIL;x=tl[x])  /* replace each id by corresponding def */
        y=add1(invgetrel(deftoids,hd[x]),y);
        g=cons(cons(hd[d],add1(hd[d],y)),g);
        /* treat all defs as recursive for now */
    }
    g=reverse(g); /* keep in address order of first components */
    /* g is list(cons(def,defs))
     where defs are all on which def immediately depends, plus self */
    g = tclos(g);  /* now g is list(cons(def,ultdefs)) */
    { /* check for unused definitions */
        word x=intersection(deps(e),ids),y=NIL; //,*g1= &g;
        for(;x!=NIL;x=tl[x])
        { word d=invgetrel(deftoids,hd[x]);
            if(!member(y,d))y=UNION(y,getrel(g,d)); }
        defs=setdiff(defs,y);  /* these are de trop */
        if(defs!=NIL)detrop=append1(detrop,defs);
        if(keep)             /* if local polymorphism not required */
            return(letrec(y,e)); /* analysis was solely to find unwanted defs */
        /* remove redundant entries from g */
        /* no, leave in for typecheck - could remove afterwards
         while(*g1!=NIL&&defs!=NIL)
         if(hd[hd[*g1]]==hd[defs])*g1=tl[*g1]; else
         if(hd[hd[*g1]]<hd[defs])g1= &tl[*g1];
         else defs=tl[defs]; */
    }
    g = msc(g);    /* g is list(defgroup,ultdefs) */
    g = tsort(g);  /* g is list(defgroup) in dependency order */
    g = reverse(g); /* reconstruct block inside-first */
    while(g!=NIL) {
        if(tl[hd[g]]==NIL &&
           intersection(get_ids(dlhs(hd[hd[g]])),deps(dval(hd[hd[g]])))==NIL) e=let(hd[hd[g]],e);  /* single non-recursive def */
        else e=letrec(hd[g],e);
        g=tl[g];
    }
    return(e);
}
/* Implementation note:
 tsort will fall over if there is a non-list strong component because it
 was originally written on assumption that relation is over identifiers.
 Whence need to pretend all defs recursive until after tsort.
 Could do better - some defs may be subsidiary to others */

word tclos(word r) /* fast transitive closure - destructive in r */
/* r is of form list(cons(x,xs)) */
{
    word r1;
    for(r1=r;r1!=NIL;r1=tl[r1]) {
        word x= less1(tl[hd[r1]],hd[hd[r1]]);
        /* invariant x intersect tl[hd[r1]] = NIL */
        while(x!=NIL)  {
            x=imageless(r,x,tl[hd[r1]]);
            tl[hd[r1]]=UNION(tl[hd[r1]],x);
        }
    }
    return(r);
}

static word getrel(word r, word x) /* r is list(cons(x,xs)) - return appropriate xs, else NIL */
{
    while(r!=NIL&&hd[hd[r]]!=x)r=tl[r];
    return(r==NIL?NIL:tl[hd[r]]);
}

static word invgetrel(word r, word x) /* return first x1 such that `x1 r x' error if none found */
{
    while (r!=NIL&&!member(tl[hd[r]],x)) r=tl[r];
    if (r==NIL) fprintf(stderr,"impossible event in invgetrel\n"),exit(1);
    return(hd[hd[r]]);
}


static word imageless(word r, word y, word z) /* image of set y in reln r, less set z */
{
    word i=NIL;
    while(r!=NIL&&y!=NIL)
        if(hd[hd[r]]==hd[y])
            i=UNION(i,less(tl[hd[r]],z)),r=tl[r],y=tl[y]; else
                if(hd[hd[r]]<hd[y])r=tl[r];
                else y=tl[y];
    return(i);
}

static word less(word x, word y)  /* non-destructive set difference x-y */
{
    word r=NIL;
    while(x!=NIL&&y!=NIL)
        if(hd[x]==hd[y])x=tl[x],y=tl[y];
        else
            if(hd[x]<hd[y])r=cons(hd[x],r),x=tl[x];
            else y=tl[y];
    return(shunt(r,x));
}

static word less1(word x, word a)  /* non-destructive set difference x- {a} */
{
    word r=NIL;
    while (x!=NIL&&hd[x]!=a) r=cons(hd[x],r),x=tl[x];
    return(shunt(r,x==NIL?NIL:tl[x]));
}

static word sort(word x) /* into address order */
{
    word a=NIL,b=NIL,hold=NIL;
    if(x==NIL||tl[x]==NIL)return(x);
    while(x!=NIL) /* split x */
    { hold=a,a=cons(hd[x],b),b=hold;
        x=tl[x]; }
    a=sort(a),b=sort(b);
    /* now merge two halves back together */
    while(a!=NIL&&b!=NIL)
        if(hd[a]<hd[b])x=cons(hd[a],x),a=tl[a];
        else x=cons(hd[b],x),b=tl[b];
    if(a==NIL)a=b;
    while(a!=NIL)x=cons(hd[a],x),a=tl[a];
    return(reverse(x));
}

word sortrel(word x) /* sort relation into address order of first components */
  /* x is a list of cons(y,ys) */
{
    word a=NIL,b=NIL,hold=NIL;
    if(x==NIL||tl[x]==NIL)return(x);
    while(x!=NIL) { /* split x */
        hold=a,a=cons(hd[x],b),b=hold;
        x=tl[x];
    }
    a=sortrel(a),b=sortrel(b);
    /* now merge two halves back together */
    while(a!=NIL&&b!=NIL)
        if(hd[hd[a]]<hd[hd[b]])x=cons(hd[a],x),a=tl[a];
        else x=cons(hd[b],x),b=tl[b];
    if(a==NIL)a=b;
    while(a!=NIL)x=cons(hd[a],x),a=tl[a];
    return(reverse(x));
}

void specify(word x, word t, word h) /* semantics of a "::" statement */
 /* N.B. t not yet in reduced form */
{
    extern word showwhat;
    if(tag[x]!=ID&&t!=type_t){
        errs=h;
        syntax("incorrect use of ::\n");
        return;
        
    }
    if(t==type_t)  {
        word a=0;
        while(tag[x]==AP)a++,x=hd[x];
        if(!(id_val(x)==UNDEF&&id_type(x)==undef_t)) {
            errs=h; nameclash(x); return;
            
        }
        id_type(x)=type_t;
        if (id_who(x)==NIL) id_who(x)=h; /* premise always true, see above */
        /* if specified and defined, locate by definition */
        id_val(x)=make_typ(a,showwhat,placeholder_t,NIL);/* placeholder type */
        addtoenv(x);
        newtyps=add1(x,newtyps);
        return;
    }
    if (id_type(x)!=undef_t) {
        errs=h; respec_error(x); return;
    }
    id_type(x)=t;
    if(id_who(x)==NIL)id_who(x)=h; /* as above */
    else speclocs=cons(cons(x,h),speclocs);
    if(id_val(x)==UNDEF)addtoenv(x);
}

static void respec_error(word x) /* only one type spec per name allowed - IS THIS RIGHT? */
{
    extern word primenv;
    if(echoing)putchar('\n');
    printf("syntax error: type of \"%s\" already declared%s\n",get_id(x),
           member(primenv,x)?" (in standard environment)":"");
    acterror();
}

static void nameclash(word x) /* only one top level binding per name allowed */
{
    extern word primenv;
    if(echoing)putchar('\n');
    printf("syntax error: nameclash, \"%s\" already defined%s\n",get_id(x),
           member(primenv,x)?" (in standard environment)":"");
    acterror();
}

void nclashcheck(word n, word dd, word hr) /* is n already bound in list of definitions dd */
{
    while (dd!=NIL&&!nclchk(n,dlhs(hd[dd]),hr)) dd=tl[dd];
}

static int nclchk(word n, word p, word hr)  /* is n already bound in pattern p */
{
    if(hd[p]==CONST)return(0);
    if(tag[p]==ID) {
        if(n!=p)return(0);
        if(echoing)putchar('\n');
        errs=hr,
        printf("syntax error: conflicting definitions of \"%s\" in where clause\n",
               get_id(n)),
        acterror();
        return(1); }
    if(tag[p]==AP&&hd[p]==PLUS) /* hd of n+k pattern */
        return(0);
    return(nclchk(n,hd[p],hr)||nclchk(n,tl[p],hr));
}

word transtypeid(word x)  /* recognises literal type constants - see rules.y */
{
    char *n=get_id(x);
    return(strcmp(n,"bool")==0 ? bool_t :
           strcmp(n,"num")==0 ? num_t   :
           strcmp(n,"char")==0 ? char_t :
           x);
}

/* end of MIRANDA TRANS */

