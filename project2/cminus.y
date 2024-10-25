/****************************************************/
/* File: tiny.y                                     */
/* The TINY Yacc/Bison specification file           */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedLineNo;  /* ditto */
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void); // added 11/2/11 to ensure no conflict with lex

%}

%token IF ELSE WHILE RETURN INT VOID 
%token ID NUM 
%token ASSIGN EQ LT GT LE GE NEQ PLUS MINUS TIMES OVER LPAREN RPAREN SEMI COMMA LBRACE RBRACE LBRACK RBRACK 
%token ERROR 

%% /* Grammar for TINY */

program     : decl-list
                 { savedTree = $1;} 
            ;
decl-list   : decl-list decl 
                  { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $2;
                     $$ = $1; }
                     else $$ = $2;
                 }
            | decl { $$ = $1; }
            ;
decl        : var-decl { $$ = $1; }
            | func-decl { $$ = $1; }
            ;
var-decl    : type_spec ID SEMI 
                 {
                  $$ = newDeclNode(VarDeclK); 
                  $$->attr.name = copyString($2); 
                  $$->type = $1;  
                 }          
            | type_spec ID LBRACK NUM RBRACK SEMI
                 {  $$ = newDeclNode(VarDeclKK);  
                    $$->attr.name = copyString($2); 
                    $$->type =  $1;
                    $$->child[0] = newConstNode(atoi($4)); 
                 }
            ;
type_spec   : INT { $$ = Integer; }
            | VOID { $$ = Void; }
            ;
func_decl   : type_spec ID LPAREN params RPAREN comp_stmt
                 {
                  $$ = newDeclNode(FuncDeclK); 
                  $$->attr.name = copyString($2); 
                  $$->type = $1;  
                  $$->child[0] = $4;
                  $$->child[1] = $6;
                 }   
            ;
params      : param-list { $$ = $1; }
            | VOID { $$ = NULL; }  
            ;
param-list  : param-list COMMA param
                 { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $3;
                     $$ = $1; }
                     else $$ = $3;
                 }
            | param
                 { $$ = $1; }
            ;
param       : type_spec ID
                  { $$ = newDeclNode(ParamK);
                    $$->type=$1;
                    $$->attr.name = copyString($2);
                  }
            | type_spec ID LPAREN RPAREN
                  { $$ = newDeclNode(ParamK);
                    $$->attr.name = copyString($2);
                    $$->type = $1;
                    $$->child[0] = NULL; 
                  }
            ;
comp_stmt   : LBRACE local_decl stmt-list RBRACE 
                 { $$ = newStmtNode(CompoundK);
                   $$->child[0]=$2;
                   $$->child[1]=$3;
                 }
            ;
local_decl  : local_decl var-decl
                 { YYSTYPE t=$1;
                   if (t !=NULL)
                   { while (t->sibling !=NULL)
                        t=t->sibling;
                     t->sibling = $3;
                     $$ = $1; }
                     else $$ = $3;
                 }
              | empty { $$ = NULL;}
            ;
stmt_list   : stmt_list stmt
                 { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $3;
                     $$ = $1; }
                     else $$ = $3;
                 }
            | empty { $$ = NULL;}
            ;
stmt        : if_stmt { $$ = $1; }
            | comp_stmt { $$ = $1; }
            | select_stmt { $$ = $1; }
            | iter_stmt { $$ = $1; }
            | return_stmt { $$ = $1; }
            | exp_stmt  { $$ = $1; }
            ;
exp_stmt    : expression SEMI { $$ = $1 ;}
            | SEMI { $$ = NULL; }
            ; 
select_stmt : mif
            | uif
            ;
mif         : IF LPAREN expression RPAREN mif ELSE stmt
                 { $$ = newStmtNode(IfK);
                   $$->child[0]=$3;
                   $$->child[1]=$5;
                   $$->child[2]=$7;
                 }
            ;
uif         : IF LPAREN expression RPAREN stmt
                 { $$ = newStmtNode(IfK);
                   $$->child[0]=$3;
                   $$->child[1]=$5;
                 }
            | IF LPAREN expression RPAREN mif ELSE uif
                 { $$ = newStmtNode(IfK);
                   $$->child[0]=$3;
                   $$->child[1]=$5;
                   $$->child[2]=$7;
                 }
iter_stmt   : WHILE LPAREN expression RPAREN stmt
                 { $$ = newStmtNode(WhileK);
                   $$->child[0]= $3;
                   $$->child[1]= $5;
                 }
            ;
return_stmt : RETURN SEMI 
                 { $$ = newStmtNode(ReturnK);
                   $$->child[0]=NULL;
                 }
            | RETUTN expression SEMI
                 { $$ = newStmtNode(ReturnK);
                   $$->child[0]=$2;
                 }
            ;
expression  : var ASSIGN expression 
                 { $$ = newExpNode(AssignK);
                   $$->child[0]=$1;
                   $$->child[1]=$3;
                 }
            | simple_exp { $$ = $1; }
            ;
var         : ID 
                 { $$ = newExpNode(IdK);
                   $$->attr.name=copyString($1);
                 }
            | ID LBRACK expression RBRACK 
                 { $$ = newExpNode(IdK);
                   $$->attr.name = copyString($1);
                   $$->child[0] = $3; 
                 }
simple_exp  : add_exp relop add_exp
                 { $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = $2;
                 }
            | add_exp { $$ = $1;}
            ;
relop       : LE { $$ = $1;}
            | LT { $$ = $1;}
            | GT { $$ = $1;}
            | GE { $$ = $1;}
            | EQ { $$ = $1;}
            | NE { $$ = $1;}
            ;
add_exp     : add_exp add_op term 
                 { $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = $2;
                 }
            | term { $$ = $1; }
            ;
add_op      : PLUS { $$ = $1; }
              MINUS { $$ = $1; }
            ; 
term        : term TIMES factor 
                 { $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = TIMES;
                 }
            | term OVER factor
                 { $$ = newExpNode(OpK);
                   $$->child[0] = $1;
                   $$->child[1] = $3;
                   $$->attr.op = OVER;
                 }
            | factor { $$ = $1; }
            ;
args        : arg-list 
                 { $$ = $1; }
            | empty  { $$ = NULL; }
            ;
arg-list    : arg-list COMMA exp 
                  { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $3;
                     $$ = $1; }
                     else $$ = $3;
                 }
            | exp { $$ = $1 ;}
            ;
   ;
factor      : LPAREN exp RPAREN
                 { $$ = $2; }
            | NUM
                 { $$ = newExpNode(ConstK);
                   $$->attr.val = atoi($1);
                 }
            | var 
                { $$ = $1 ;}
            | call
                { $$ = $1 ;}
            ;
call        : ID LPAREN args RPAREN
                 { $$ = newExpNode(CallK);
                   $$->attr.name=copyString($1);
                   $$->child[0] = $3;
                 }
            ;
%%

int yyerror(char * message)
{ fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
  fprintf(listing,"Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{ return getToken(); }

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}

