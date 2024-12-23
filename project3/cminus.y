/****************************************************/
/* File: cminus.y                                   */
/* The C-Minus Yacc/Bison specification file        */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

/**************************/
/* C declarations         */
/**************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"



#define YYSTYPE TreeNode *
static int savedNumber;
static char * savedName; /* for use in assignments */
/*globals.h에 있는 type과 맞춰주기*/
static ExpType savedType;
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void); // added 11/2/11 to ensure no conflict with lex
void print_debug_info(void);
/* 암시적 선언 문제 해결... */
int yyerror(char * message);

%}

/**************************/
/* Bison declarations     */
/**************************/


%token IF ELSE INT RETURN VOID WHILE
%token ID NUM
%token LT LE GT GE EQ NE SEMI
%token LPAREN RPAREN LCURLY RCURLY LBRACE RBRACE
%left PLUS MINUS
%left TIMES OVER COMMA
/*최종 대입 대상이 LHS니까 오른쪽부터 결합해서 최종으로 LHS에 넣어줘야*/
%right ASSIGN
%right THEN ELSE
%token ERROR


%%

/**************************/
/* Grammar rules          */
/**************************/

program     : decl_list { savedTree = $1;}
            ;

decl_list   : decl_list declaration
                 { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $2;
                     $$ = $1; }
                     else $$ = $2;
                 }
                 | declaration { $$ = $1; }
                 ;

declaration : var_decl { $$ = $1; }
            | func_decl { $$ = $1; }
            ;
/* 얘를 node로 만들어야 덮어 씌워지는 대참사 막을 수 있음... */
id          :  ID
                 { $$ = newExpNode(IdK,lineno);
                 savedName = copyString(tokenString); 
                 $$->attr.name=savedName;
                 $$->lineno=lineno;
                 }
            ;
num         : NUM
                 { $$ = newExpNode(ConstK,lineno);
                   savedNumber = atoi(tokenString);
                   $$->attr.val = savedNumber;
                   $$->lineno=lineno;
                 }
                 ;
/*1번은 child 없이 id랑 type에서 속성 갖고 오기*/
var_decl    : type_spec id SEMI
                 { $$ = newDeclNode(VarK, $2->lineno);
                 $$->lineno=$2->lineno;
                   $$->type = $1->type;  
                   $$->attr.name = $2->attr.name;  /* id node의 이름... savedName 쓰니까 update되어버림 */
                 }
/*�迭�� type_spec�� ���� type ����*/
            | type_spec id LBRACE num RBRACE SEMI
                 { $$ = newDeclNode(ArrK,$2->lineno);
                   $$->type = ($1->type== Integer) ? IntArray : VoidArray;
                   $$->attr.name =$2->attr.name;
                   $$->child[0] = $4;
                 }
            ;
/*��׵� node�� �� ������ָ� update�Ǿ node�� ������ֱ� */
type_spec   : INT 
                 { $$ = newExpNode(TypeK,lineno); 
                   $$->type = Integer; 
                 }
            | VOID 
                 { $$ = newExpNode(TypeK,lineno); 
                   $$->type = Void;     
                 }
                 ;
/*child�� param�̶� comp type, name�� �� saved���� */
func_decl   : type_spec id LPAREN params RPAREN comp_stmt
                 { $$ = newDeclNode(FuncK,$2->lineno);
                   $$->type = $1->type;  
                   $$->attr.name = $2->attr.name;  
                   $$->child[0] = $4; 
                   $$->child[1] = $6; 
                 }
                 ;

params      : param_list { $$ = $1; }
            | VOID { $$ = newDeclNode(VoidParamK,lineno); }
            ;
param_list : param_list COMMA param
                 { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $3;
                     $$ = $1; }
                     else $$ = $3;
                 }
           | param { $$ = $1; }
           ;
/* ����� parameter�� ���� ��... type�� void���� void parameter�� �ƴ� �׳� type�� void�� ���� */
param      : type_spec id
                 { $$ = newDeclNode(ParamK,$2->lineno);
                   $$->type = $1->type;  
                   $$->attr.name = copyString(savedName);  
                 }
           | type_spec id LBRACE RBRACE
                 { $$ = newDeclNode(ArrParamK,$2->lineno);
                   $$->type = ($1->type==Integer)?IntArray:VoidArray; 
                   $$->attr.name = copyString(savedName); 
                 }
           ;

comp_stmt  : LCURLY local_decl stmt_list RCURLY
                { $$ = newStmtNode(CompoundK,lineno);
                 $$->child[0] = $2 ? $2 : NULL;  // local_declarations�� ������ NULL
                  $$->child[1] = $3 ? $3 : NULL;
                  $$->lineno=lineno;
                }
           ;

local_decl : local_decl var_decl
                 { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $2;
                     $$ = $1; }
                     else $$ = $2;
                 }
           | { $$ = NULL; }
           ;

stmt_list : stmt_list statement
                 { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $2;
                     $$ = $1; }
                     else $$ = $2;
                 }
          | { $$ = NULL; }
          ;

statement : exp_stmt { $$ = $1; }
          | comp_stmt { $$ = $1; }
          | select_stmt { $$ = $1; }
          | iter_stmt { $$ = $1; }
          | return_stmt { $$ = $1; }
          | ERROR SEMI { yyerror("Syntax error in statement."); yyerrok; }
          ;

exp_stmt  : expression SEMI { $$ = $1; }
          | SEMI { $$ = NULL; }
          ;

select_stmt: IF LPAREN expression RPAREN statement   
                 { $$ = newStmtNode(IfK,$5->lineno);
                   $$->child[0] = $3;
                   $$->child[1] = $5;
                   $$->lineno=$5->lineno;
                 } %prec THEN
           | IF LPAREN expression RPAREN statement ELSE statement 
                { $$ = newStmtNode(ElseK,$7->lineno);
                  $$->child[0] = $3;
                  $$->child[1] = $5;
                  $$->child[2] = $7;
                  $$->lineno=$7->lineno;
                }
           ;
iter_stmt : WHILE LPAREN expression RPAREN statement 
               { $$ = newStmtNode(WhileK,$5->lineno);
                 $$->child[0] = $3;
                 $$->child[1] = $5;
                 $$->lineno=$5->lineno;
               }
          ;

return_stmt : RETURN SEMI
                 { $$ = newStmtNode(ReturnK,lineno);
                   $$->lineno=lineno;
                 }
            | RETURN expression SEMI
                 { $$ = newStmtNode(ReturnK,lineno);
                   $$->child[0] = $2;
                   $$->lineno=lineno;
                 }
            ;

expression : var ASSIGN expression
                { $$ = newExpNode(AssignK,$1->lineno);
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->lineno=$1->lineno;
                }
           | simple_exp { $$ = $1; }
           ;

var        : id
               { $$ = newExpNode(VarExpK,$1->lineno);
                 $$->attr.name = copyString(savedName);
                 $$->lineno=$1->lineno;
               }
           | id LBRACE expression RBRACE
             { $$ = newExpNode(VarExpK,$1->lineno);
               $$->attr.name = $1->attr.name;
               $$->child[0] = $3;
               $$->lineno=$1->lineno;
             }
           ;

simple_exp : add_exp LT add_exp
               { $$ = newExpNode(OpK, $1->lineno);  
                 $$->child[0] = $1;
                 $$->attr.op =LT;  
                 $$->child[1] = $3;
                 $$->lineno=$1->lineno;
               }
           | add_exp LE add_exp
               { $$ = newExpNode(OpK, $1->lineno); 
                 $$->child[0] = $1;
                 $$->attr.op =LE; 
                 $$->child[1] = $3;
                 $$->lineno=$1->lineno;
               }
           |add_exp GT add_exp
               { $$ = newExpNode(OpK, $1->lineno);  
                 $$->child[0] = $1;
                 $$->attr.op = GT;  
                 $$->child[1] = $3;
                 $$->lineno=$1->lineno;
               }
           |add_exp GE add_exp
               { $$ = newExpNode(OpK, $1->lineno);  
                 $$->child[0] = $1;
                 $$->attr.op = GE;  
                 $$->child[1] = $3;
                 $$->lineno=$1->lineno;
               }
           | add_exp EQ add_exp
               { $$ = newExpNode(OpK, $1->lineno);  
                 $$->child[0] = $1;
                 $$->attr.op = EQ;  
                 $$->child[1] = $3;
                 $$->lineno=$1->lineno;
               }
           | add_exp NE add_exp
               { $$ = newExpNode(OpK, $1->lineno); 
                 $$->child[0] = $1;
                 $$->attr.op = NE;  
                 $$->child[1] = $3;
                 $$->lineno=$1->lineno;
               }
           | add_exp { $$ = $1; }
           ;



add_exp    : add_exp PLUS term
                { $$ = newExpNode(OpK, $1->lineno); 
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = PLUS;
                  $$->lineno=$1->lineno;
                }
           | add_exp MINUS term
                { $$ = newExpNode(OpK, $1->lineno); 
                  $$->child[0] = $1;
                  $$->child[1] = $3;
                  $$->attr.op = MINUS; 
                  $$->lineno=$1->lineno;
                }
           | term { $$ = $1; }
           ;

term       : term TIMES factor
                 { $$ = newExpNode(OpK, $1->lineno);
                   $$->child[0] = $1;
                   $$->attr.op = TIMES;
                   $$->child[1] = $3;
                   $$->lineno=$1->lineno;
                 }
           | term OVER factor
                 { $$ = newExpNode(OpK, $1->lineno);
                   $$->child[0] = $1;
                   $$->attr.op = OVER;
                   $$->child[1] = $3;
                   $$->lineno=$1->lineno;
                 }
           | factor { $$ = $1; }
           ;

factor     : LPAREN expression RPAREN { $$ = $2; }
           | var { $$ = $1; }
           | call { $$ = $1; }
           | num 
                { $$ = newExpNode(ConstK, $1->lineno);
                  $$->attr.val=savedNumber;
                  $$->lineno=$1->lineno;
                }
           ;
    
call       : id LPAREN args RPAREN 
                { $$ = newExpNode(CallK, $1->lineno);
                  $$->attr.name = $1->attr.name;
                  $$->child[0] = $3;
                  $$->lineno=$1->lineno;
                }
           ;

args       : arg_list { $$ = $1; }
           | { $$ = NULL; }
           ;
      
arg_list   : arg_list COMMA expression
              { YYSTYPE t = $1;
                   if (t != NULL)
                   { while (t->sibling != NULL)
                        t = t->sibling;
                     t->sibling = $3;
                     $$ = $1; }
                     else $$ = $3;
                 }
           | expression { $$ = $1; }
           ;

%%

/**************************/
/* Additional C code      */
/**************************/

int yyerror(char * message) {
  fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
  fprintf(listing,"Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}
/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void) {
  return getToken();
}

TreeNode * parse(void) {
  yyparse();
  return savedTree;
}