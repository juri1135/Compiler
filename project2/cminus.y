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
/*최종 대입 대상이 LHS니까 오른쪽부터 결합해서 최종으로 LHS에 넣어줘야*/
%right ASSIGN
%right THEN ELSE
%token ERROR


%%

/**************************/
/* Grammar rules          */
/**************************/

program : declaration_list { savedTree = $1;}
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
/*이건 node 아니고 특정 node의 name으로 들어 간다...*/
id :  ID
      { $$ = newExpNode(IdK);
       savedName = copyString(tokenString);  /* 이름을 savedName에 저장 */
       savedLineNo = lineno;
       $$->attr.name=savedName;
       $$->lineno=lineno;}
   ;
/* 얘는 const node로 만들어짐... arr의 child이자 그 외에도 사용...*/
num : NUM
      { $$ = newExpNode(ConstK);
        savedNumber = atoi(tokenString);
        $$->attr.val = savedNumber;
        $$->lineno = lineno;
       }
    ;
/*1번은 child 없이 id랑 type에서 속성 갖고 오기*/
var_declaration : type_specifier id SEMI
                  { $$ = newDeclNode(VarK);
                    $$->type = $1->type;  
                    $$->attr.name = $2->attr.name;  /* id에서 반환한 이름 */
                    $$->lineno = savedLineNo;
                   
                  }
/*배열은 child로 const Node NUM 가져야 함 type은 savedType, 이름도 savedName*/
                | type_specifier id LBRACE num RBRACE SEMI
                  { $$ = newDeclNode(ArrK);
                    $$->type = ($1->type== Integer) ? IntArray : VoidArray;
                    $$->attr.name =$2->attr.name;
                    $$->child[0] = $4;
                    $$->lineno = savedLineNo;
                  }
                ;
/*얘네는 node의 속성 중 type으로 들어가서 node 생성 필요 없음 */
type_specifier : INT 
                { 
                  $$ = newDeclNode(TypeK); 
                  $$->type = Integer; 
                }
               | VOID 
                { 
                  $$ = newDeclNode(TypeK); 
                  $$->type = Void;     
                }
               ;
/*child는 param이랑 comp type, name은 다 saved에서 */
fun_declaration : type_specifier id LPAREN params RPAREN compound_stmt
                 { 
                   $$ = newDeclNode(FuncK);
                   $$->type = $1->type;  
                   $$->attr.name = $2->attr.name;  
                   $$->lineno = lineno;
                   $$->child[0] = $4; 
                   $$->child[1] = $6; 
                 }
                ;

params : param_list { $$ = $1; }
       | VOID { $$ = newDeclNode(VoidParamK); }
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
        { 
        $$ = newDeclNode(ParamK);
        $$->type = $1->type;  
        $$->attr.name = copyString(savedName);  
        $$->lineno = savedLineNo;
      }
      | type_specifier id LBRACE RBRACE
      { 
        $$ = newDeclNode(ArrParamK);
        $$->type = IntArray; 
        $$->attr.name = copyString(savedName); 
        $$->lineno = savedLineNo;
      }
      ;

compound_stmt : LCURLY local_declarations statement_list RCURLY
                { $$ = newStmtNode(CompoundK);
                 $$->child[0] = $2 ? $2 : NULL;  // local_declarations가 없으면 NULL
                  $$->child[1] = $3 ? $3 : NULL;
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
                 { 
                   YYSTYPE t = $1;
                   if (t != NULL) {
                     while (t->sibling != NULL && t->sibling != t) { 
                        t = t->sibling; 
                     }
                     if (t->sibling == t) {
                         t->sibling = NULL;  
                     }
                     t->sibling = $2;
                     $$ = $1;
                   } else {
                     $$ = $2;}
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

selection_stmt: IF LPAREN expression RPAREN statement   
                { $$ = newStmtNode(IfK);
                  $$->child[0] = $3;
                  $$->child[1] = $5;
                } %prec THEN
              | IF LPAREN expression RPAREN statement ELSE statement 
                { $$ = newStmtNode(ElseK);
                  $$->child[0] = $3;
                  $$->child[1] = $5;
                  $$->child[2] = $7;
                }

iteration_stmt : WHILE LPAREN expression RPAREN statement 
                 { 
                $$ = newStmtNode(WhileK);
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
             { 
               $$ = newExpNode(AssignK);
               $$->child[0] = $1;
               $$->child[1] = $3;
             }
           | simple_expression { $$ = $1; }
           ;

var : id
      { $$ = newExpNode(VarExpK);
        $$->attr.name = copyString(savedName);
      }
    | id LBRACE expression RBRACE
      {
        $$ = newExpNode(VarExpK);
        $$->attr.name = $1->attr.name;
        $$->child[0] = $3;
      }
    ;

simple_expression : additive_expression LT additive_expression
                    { 
                      $$ = newExpNode(OpK);  
                      $$->child[0] = $1;
                      $$->attr.op =LT;  
                      $$->child[1] = $3;
                    }
                  | additive_expression LE additive_expression
                    { 
                      $$ = newExpNode(OpK); 
                      $$->child[0] = $1;
                      $$->attr.op =LE; 
                      $$->child[1] = $3;
                    }
                  |additive_expression GT additive_expression
                    { 
                      $$ = newExpNode(OpK);  
                      $$->child[0] = $1;
                      $$->attr.op = GT;  
                      $$->child[1] = $3;
                    }
                  |additive_expression GE additive_expression
                    { 
                      $$ = newExpNode(OpK);  
                      $$->child[0] = $1;
                      $$->attr.op = GE;  
                      $$->child[1] = $3;
                    }
                  | additive_expression EQ additive_expression
                    { 
                      $$ = newExpNode(OpK);  
                      $$->child[0] = $1;
                      $$->attr.op = EQ;  
                      $$->child[1] = $3;
                    }
                  | additive_expression NE additive_expression
                    { 
                      $$ = newExpNode(OpK); 
                      $$->child[0] = $1;
                      $$->attr.op = NE;  
                      $$->child[1] = $3;
                     }
                  | additive_expression { $$ = $1; }
                  ;



additive_expression : additive_expression PLUS term
                      { $$ = newExpNode(OpK); 
                        $$->child[0] = $1;
                        $$->child[1] = $3;
                        $$->attr.op = PLUS;
                      }
                    | additive_expression MINUS term
                      { $$ = newExpNode(OpK); 
                        $$->child[0] = $1;
                        $$->child[1] = $3;
                        $$->attr.op = MINUS; 
                      }
                    | term { $$ = $1; }
                    ;

term : term TIMES factor
       { $$ = newExpNode(OpK);
         $$->child[0] = $1;
         $$->attr.op = TIMES;
         $$->child[1] = $3;
       }
     | term OVER factor
       { $$ = newExpNode(OpK);
         $$->child[0] = $1;
         $$->attr.op = OVER;
         $$->child[1] = $3;
       }
     | factor { $$ = $1; }
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
         $$->attr.name = $1->attr.name;
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
