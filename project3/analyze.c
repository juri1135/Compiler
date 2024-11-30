/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"
#include "util.h"

static int locationCounter = 0;
int scopeDepth = 0;   
ParameterList* currentParameterList=NULL;
int nestedLevel=0;

void error_undeclared(FILE * listing, char * type, char* name, int lineno){
  if(strcmp(type,"undeclared_func")==0){
    fprintf(listing, "Error: undeclared function \"%s\" is called at line %d\n", name, lineno);}
  else{
    fprintf(listing, "Error: undeclared variable \"%s\" is used at line %d\n", name, lineno);
  }
}
void error_void_call_index(FILE *listing, char *type, char * name, int lineno){
  if(strcmp(type,"void")==0){
    fprintf(listing, "Error: The void-type variable is declared at line %d (name : \"%s\")\n", lineno, name);
  }else if(strcmp(type,"indexNotInt")==0){
    fprintf(listing, "Error: Invalid array indexing at line %d (name : \"%s\"). indicies should be integer\n", lineno, name);
  }else if(strcmp(type,"voidarrIndex")==0){
    fprintf(listing, "Error: Invalid array indexing at line %d (name : \"%s\"). indexing can only allowed for int[] variables\n", lineno, name);
  }else{
    fprintf(listing, "Error: Invalid function call at line %d (name : \"%s\")\n", lineno, name);
  }
}
void error_invalid(FILE *listing, char *type, int lineno){
  if(strcmp(type,"return")){
    fprintf(listing, "Error: Invalid return at line %d\n", lineno);
  }else if(strcmp(type,"assign")==0){
    fprintf(listing, "Error: invalid assignment at line %d\n", lineno);
  }else if(strcmp(type,"operation")==0){
    fprintf(listing, "Error: invalid operation at line %d\n", lineno);
  }else{
    fprintf(listing, "Error: invalid condition at line %d\n", lineno);
  }
}
void error_redefine(FILE *listing, TreeNode *t) {
    // BucketList에서 심볼 정보 가져오기
    BucketList bucket = st_lookup_bucket(t->attr.name);
    if (bucket == NULL) {
        fprintf(listing, "Error: Internal issue - no bucket found for symbol \"%s\".\n", t->attr.name);
        return; // 안전하게 함수 종료
    }
    LineList linelist = bucket->lines;

    fprintf(listing, "Error: Symbol \"%s\" is redefined at line %d (already defined at lines: ", t->attr.name, t->lineno);

    LineList current = linelist;
    while (current != NULL) {
        fprintf(listing, "%d", current->lineno); // 현재 라인 번호 출력
        if (current->next != NULL) {
            fprintf(listing, ", "); // 쉼표 출력
        }
        current = current->next; // 다음 노드로 이동
    }

    fprintf(listing, ").\n");
}

void inputFunc(void){
  //project 2에서 사용한 문법 기반으로 작성
  TreeNode * funcNode=newDeclNode(FuncK);
  funcNode->type=Integer;
  funcNode->attr.name="input";
  funcNode->lineno=0;
  
  //child 0은 param, 1은 compound
  //void param은 decl VoidParamK
  TreeNode * param=newDeclNode(VoidParamK);
  funcNode->child[0]=param;

  TreeNode * compound=newStmtNode(CompoundK);
  compound->child[0]=NULL;
  compound->child[1]=NULL;
  funcNode->child[1]=compound;

  //return Node는 형태를 모르니까 만들진 말고 그냥 st_insert_function에 int로 넘겨
  st_insert_function(currentScope,"input","int",NULL);
}

void outputFunc(void){
  TreeNode *funcNode = newDeclNode(FuncK);
  funcNode->type = Void;              
  funcNode->attr.name = "output";      
  funcNode->lineno = 0;                
   
  ParameterList params = NULL;
  addParameter(&params, "value", "int");
  
  TreeNode *param = newDeclNode(ParamK);
  param->type = Integer;
  param->attr.name = "value";
  funcNode->child[0] = param;

  TreeNode *compound = newStmtNode(CompoundK);
  compound->child[0] = NULL;                 // 지역 변수 없음
  compound->child[1] = NULL;                 // 명령문 없음
  funcNode->child[1] = compound;
  // 심볼 테이블에 추가
  st_insert_function(currentScope, "output", "void", params);
}
/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t, void (* preProc) (TreeNode *), void (* postProc) (TreeNode *) ){ 
  if (t != NULL)
  { preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++)
        traverse(t->child[i],preProc,postProc);
    }
    postProc(t);
    traverse(t->sibling,preProc,postProc);
  }
}

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode * t){ 
  if (t==NULL) return;
  else return;
}



/* Procedure insertNode inserts 
 * identifiers stored in t into 
 * the symbol table 
 */
static void insertNode(TreeNode *t) {
	switch (t->nodekind) {
		case DeclK:
			switch (t->kind.decl) {
				case VarK:
        case ArrK: 
          if (st_lookup_top(t->attr.name)) {
            error_redefine(listing, t);
          } else if (t->type == Void || t->type == VoidArray) {
              error_void_call_index(listing,"void",t->attr.name,t->lineno);
          } else {
              char *type = (t->kind.decl == ArrK) ? "intArray" : "int";
              st_insert("Variable",t->attr.name, t->lineno, locationCounter++, type, currentScope);
          }
          break;
          

				case FuncK: // 함수 처리
          if (st_lookup_top(t->attr.name)) { // 현재 스코프에서 중복 확인
             error_redefine(listing, t);
          } else {
              ParameterList params = NULL;
              char * type=t->type==Integer?"int":"void";
              st_insert("Function",t->attr.name,t->lineno,locationCounter++,type,currentScope);
              st_insert_function(currentScope, t->attr.name, type, params);

              Scope funcScope = newScope(t->attr.name);
              pushScope(funcScope);
              currentParameterList = &(currentScope->paramlist[hash(t->attr.name)]);
          }
          scopeDepth=1;
          break;
      
				case ParamK:
        if (st_lookup_top(t->attr.name)) { //현재 function 인자에 중복으로 선언된 거 있는 지 확인하는 
            error_redefine(listing, t);
          } else {
            st_insert("Variable", t->attr.name,t->lineno,locationCounter++,"int",currentScope);
            addParameter(currentParameterList, t->attr.name, "int");
          }
          break;
        
				case ArrParamK:
					if (st_lookup(t->attr.name)) {
						error_redefine(listing, t);
					}  else if (t->type == VoidArray) { // void[] 배열 선언
              error_void_call_index(listing,"void",t->attr.name,t->lineno);
          } else {
						st_insert("Variable",t->attr.name, t->lineno, locationCounter++,"int[]",currentScope);
            addParameter(currentParameterList, t->attr.name, "int");
					}
					break;
  
				case VoidParamK:
					st_insert("Variable",t->attr.name, t->lineno, locationCounter++, "void",currentScope);
					break;

				default:
					break;
			}
			break;

		case StmtK:
			switch (t->kind.stmt) {
				case CompoundK: {
          char scopeName[64];

           if (scopeDepth== 1) {
            // 함수의 첫 번째 Compound는 함수 스코프를 그대로 사용
            snprintf(scopeName, sizeof(scopeName), "%s", currentScope->name);
          } else {
            // 중첩된 Compound는 고유 이름 생성
            snprintf(scopeName, sizeof(scopeName), "%s_%d", currentScope->name,scopeDepth- 1);
          }

          Scope newscope = newScope(scopeName);
          pushScope(newscope);
          scopeDepth++;
          nestedLevel++;
          break;
        }
				default:
					break;
			}
			break;

		case ExpK:
      BucketList list = st_lookup_bucket(t->attr.name);
			switch (t->kind.exp) {
        
				case CallK:
					
          if (list == NULL) {
              error_undeclared(listing,"undeclared_func",t->attr.name,t->lineno);
          } else {
              t->type = (strcmp(list->type, "int") == 0) ? Integer : Void;
              insertLines(t->attr.name, t->lineno); // 라인 번호 추가
          }
					break;
				case IdK:
          if (list == NULL) {
              error_undeclared(listing,"undeclared_id",t->attr.name,t->lineno);
          } else {
              t->type = (strcmp(list->type, "int") == 0) ? Integer : Void;
              insertLines(t->attr.name, t->lineno); // 라인 번호 추가
          }
					break;

				default:
					break;
			}
			break;

		default:
			break;
	}
}
void pushScope(Scope scope) {
    if (topScope >= SIZE - 1) {
        fprintf(stderr, "Error: Scope stack overflow. Cannot push more scopes.\n");
        exit(EXIT_FAILURE);
    }
    if (currentScope != NULL) {
        scope->parent = currentScope;
    }

    static int compoundCounter = 0;
    if (strcmp(scope->name, "Compound") == 0) {
        char uniqueName[64];
        snprintf(uniqueName, sizeof(uniqueName), "Compound_%d", compoundCounter++);
        free(scope->name);
        scope->name = strdup(uniqueName); // 고유 이름 설정
    }
    scope->currentParamList = NULL;
    scope->nestedLevel = nestedLevel; // nestedLevel 저장
    scopeStack[++topScope] = scope;
    currentScope = scope;
    nestedLevel++;
    fprintf(stderr, "Pushed scope: %s (Nested Level: %d)\n", scope->name, nestedLevel);
}

void popScope() {
    if (topScope < 0) {
        fprintf(stderr, "Error: Scope stack underflow. No scopes to pop.\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Popped scope: %s (Nested Level: %d)\n", currentScope->name, nestedLevel);

    scopeStack[topScope] = NULL;
    topScope--;

    currentScope = (topScope >= 0) ? scopeStack[topScope] : NULL;

    if (nestedLevel > 0) {
        nestedLevel--;
    }
}

void postProc(TreeNode *t) {
    if (t->nodekind == StmtK && t->kind.stmt == CompoundK) {
        popScope();      // 스코프 닫기
        scopeDepth--;   
        locationCounter = 0; // 메모리 위치 초기화
    }
}
/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{ //우리는 postOrder에서 scope 닫아야 하니까... nullProc 대신 scope 관리 함수 전달 
  //맨 처음에 새로 global scope 만들고, 현재 scope를 global로 설정... 
    Scope globalScope = newScope("global");
    pushScope(globalScope);
  //input, output 함수 삽입
    inputFunc();
    outputFunc();

    // AST 순회하며 심볼 테이블 작성
    traverse(syntaxTree, insertNode, postProc);
    popScope();
  if (TraceAnalyze)
  { fprintf(listing,"\nSymbol table:\n\n");
    printSymTab(listing);
  }
}

static void typeError(TreeNode * t, char * message)
{ fprintf(listing,"Type error at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t)
{ 
//switch (t->nodekind)
//   { case ExpK:
//       switch (t->kind.exp)
//       { case OpK:
//           if ((t->child[0]->type != Integer) ||
//               (t->child[1]->type != Integer))
//             typeError(t,"Op applied to non-integer");
//           if ((t->attr.op == EQ) || (t->attr.op == LT))
//             t->type = Boolean;
//           else
//             t->type = Integer;
//           break;
//         case ConstK:
//         case IdK:
//           t->type = Integer;
//           break;
//         default:
//           break;
//       }
//       break;
//     case StmtK:
//       switch (t->kind.stmt)
//       { case IfK:
//           if (t->child[0]->type == Integer)
//             typeError(t->child[0],"if test is not Boolean");
//           break;
//         case AssignK:
//           if (t->child[0]->type != Integer)
//             typeError(t->child[0],"assignment of non-integer value");
//           break;
//         case WriteK:
//           if (t->child[0]->type != Integer)
//             typeError(t->child[0],"write of non-integer value");
//           break;
//         case RepeatK:
//           if (t->child[1]->type == Integer)
//             typeError(t->child[1],"repeat test is not Boolean");
//           break;
//         default:
//           break;
//       }
//       break;
//     default:
//       break;

//   }
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{ traverse(syntaxTree,nullProc,checkNode);
}
