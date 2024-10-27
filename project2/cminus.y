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
static int savedLineNo;  /* ditto */
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
%right ASSIGN
%right THEN ELSE
%token ERROR

/*

typedef enum {StmtK,ExpK, DeclK} NodeKind;
typedef enum {VarK, ArrK, FuncK, ParamK, VoidParamK, ArrParamK} DeclKind;
typedef enum {IfK,ElseK, WhileK, CompoundK, ReturnK } StmtKind;
typedef enum {OpK,ConstK,IdK,ArrIdK,CallK, AssignK} ExpKind;

*/

%%

/**************************/
/* Grammar rules          */
/**************************/

program : declaration_list { savedTree = $1; print_debug_info();}
        ;

declaration_list : declaration_list declaration
                   { YYSTYPE t = $1;
                     if (t != NULL) {
                       while (t->sibling != NULL) { t = t->sibling; }
                       t->sibling = $2;
                       $$ = $1;
                     } else {
                       $$ = $2;
                     }
                   }
                 | declaration { $$ = $1; }
                 ;

declaration : var_declaration { $$ = $1; }
            | fun_declaration { $$ = $1; }
            ;

id : ID
     { 
       savedName = copyString(tokenString);
       savedLineNo = lineno;
     }
   ;

num : NUM
      { $$ = newExpNode(ConstK);
        savedNumber = atoi(tokenString);
        savedLineNo = lineno;
        $$->attr.val=savedNumber;
        $$->lineno=lineno;
      }
    ;

var_declaration : type_specifier id SEMI
                  { $$ = newDeclNode(VarK);
                    $$->type=savedType;
                    $$->lineno = lineno;
                    $$->attr.name = savedName;
                  }
                | type_specifier id LBRACE num RBRACE SEMI
                  { $$ = newDeclNode(ArrK);
                    $$->type=IntArray;
                    $$->attr.name=savedName;
                    
                    $$->lineno=lineno;
                    $$->child[0] = $4;
                  }
                ;

type_specifier : INT { savedType=Integer; }
               | VOID { savedType=Void;}
               ;

fun_declaration : type_specifier id LPAREN params RPAREN compound_stmt
                 { $$ = newDeclNode(FuncK);
                   $$->type=savedType;
                   $$->attr.name=savedName;
                   $$->lineno=lineno;
                   $$->child[0] = $4;
                   $$->child[1] = $6;
                 }
                ;

params : param_list { $$ = $1; }
       | VOID
         { $$ = newDeclNode(VoidParamK);
         }
       ;

param_list : param_list COMMA param
             { YYSTYPE t = $1;
               if (t != NULL) {
                 while (t->sibling != NULL) { t = t->sibling; }
                 t->sibling = $3;
                 $$ = $1;
               } else {
                 $$ = $3;
               }
             }
           | param { $$ = $1; }
           ;
/* 여기는 parameter를 갖는 것... type이 void여도 void parameter는 아님 그냥 type이 void인 인자 */
param : type_specifier id
        { $$ = newDeclNode(ParamK);
          $$->type=savedType;
          $$->attr.name = savedName;
        }
        /* c-는 배열 크기 지정해서 함수 전달 불가 */ 
      | type_specifier id LBRACE RBRACE
        { $$ = newDeclNode(ArrParamK);
          $$->attr.name = copyString(savedName);
          $$->type=IntArray;
        }
      ;

compound_stmt : LCURLY local_declarations statement_list RCURLY
                { $$ = newStmtNode(CompoundK);
                  $$->child[0] = $2;
                  $$->child[1] = $3;
                }
              ;

local_declarations : local_declarations var_declaration
                     { YYSTYPE t = $1;
                       if (t != NULL) {
                         while (t->sibling != NULL) { t = t->sibling; }
                         t->sibling = $2;
                         $$ = $1;
                       } else {
                         $$ = $2;
                       }
                     }
                   | { $$ = NULL; }
                   ;

statement_list : statement_list statement
                 { YYSTYPE t = $1;
                   if (t != NULL) {
                     while (t->sibling != NULL) { t = t->sibling; }
                    t->sibling = $2;
                    $$ = $1;
                   } else {
                     $$ = $2;
                   }
                 }
               | { $$ = NULL; }
               ;

statement : expression_stmt { $$ = $1; }
          | compound_stmt { $$ = $1; }
          | selection_stmt { $$ = $1; }
          | iteration_stmt { $$ = $1; }
          | return_stmt { $$ = $1; }
          | ERROR SEMI { yyerror("Syntax error in statement."); yyerrok; }
          ;

expression_stmt : expression SEMI { $$ = $1; }
                | SEMI { $$ = NULL; }
                ;

selection_stmt: IF LPAREN expression RPAREN statement {
  $$ = newStmtNode(IfK);
  $$->child[0] = $3;
  $$->child[1] = $5;
} %prec THEN | IF LPAREN expression RPAREN statement ELSE statement {
  $$ = newStmtNode(IfK);
  $$->child[0] = $3;
  $$->child[1] = $5;
  $$->child[2] = $7;
}

iteration_stmt : WHILE LPAREN expression RPAREN statement
                 { $$ = newStmtNode(WhileK);
                   $$->child[0] = $3;
                   $$->child[1] = $5;
                 }
               ;

return_stmt : RETURN SEMI
              { $$ = newStmtNode(ReturnK);
              }
            | RETURN expression SEMI
              { $$ = newStmtNode(ReturnK);
                $$->child[0] = $2;
              }
            ;

expression : var ASSIGN expression
             { $$ = newStmtNode(AssignK);
               $$->child[0] = $1;
               $$->child[1] = $3;
             }
           | simple_expression { $$ = $1; }
           ;

var : id
      { $$ = newExpNode(IdK);
      }
    | id LBRACE expression RBRACE
      {
        $$ = newExpNode(ArrIdK);
        $$->child[0] = $3;
      }
    ;

simple_expression : additive_expression relop additive_expression
                    { $$ = newExpNode(OpK);
                      $$->type=$2->type;
                      $$->child[0] = $1;
                      $$->child[1] = $3;
                    }
                  | additive_expression { $$ = $1; }
                  ;

relop : LT { $$->attr.op = LT; }
      | LE { $$->attr.op = LE; }
      | GT { $$->attr.op = GT; }
      | GE { $$->attr.op = GE; }
      | EQ { $$->attr.op = EQ; }
      | NE { $$->attr.op = NE; }
      ;

additive_expression : additive_expression addop term
                      { $$ = newExpNode(OpK);
                        $$->child[0] = $1;
                        $$->child[1] = $3;
                      }
                    | term { $$ = $1; }
                    ;

addop : PLUS { $$->attr.op = PLUS; }
      | MINUS { $$->attr.op = MINUS; }
      ;

term : term mulop factor
       { $$ = newExpNode(OpK);
         $$->child[0] = $1;
         $$->child[1] = $3;
       }
     | factor { $$ = $1; }
     ;

mulop : TIMES { $$->attr.op = TIMES; }
      | OVER { $$->attr.op = OVER; }
      ;

factor : LPAREN expression RPAREN { $$ = $2; }
       | var { $$ = $1; }
       | call { $$ = $1; }
       | num 
          { $$ = newExpNode(ConstK);
            $$->attr.val=savedNumber;
          }
       ;

call : id LPAREN args RPAREN 
       { $$ = newExpNode(CallK);
         $$->attr.name = savedName;
         $$->child[0] = $3;
       }
     ;

args : arg_list { $$ = $1; }
     | { $$ = NULL; }
     ;

arg_list : arg_list COMMA expression
           { YYSTYPE t = $1;
             if (t != NULL) {
               while (t->sibling != NULL) { t = t->sibling; }
               t->sibling = $3;
               $$ = $1;
             } else {
               $$ = $3;
             }
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
void print_debug_info() {
    printf("Debug Info:\n");
    printf("savedTree: %p\n", (void *)savedTree);
    printf("savedName: %s\n", savedName ? savedName : "NULL");
    printf("savedType: %d\n", savedType);
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
