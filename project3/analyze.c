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

int scopeDepth = 0;   
ParameterList* currentParameterList=NULL;
int nestedLevel=0;
Scope globalScope;
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
  st_insert_function("Function","input",0,"int",currentScope,NULL);
}

void outputFunc(void){ 
   nestedLevel++;
   ParameterList params = NULL;
   addParameter(&params, "value", "int");
   st_insert_function("Function","output",0,"void",currentScope,params);
   BucketList temp=currentScope->hashTable[hash("output")];
  temp->params=params; 
   Scope funcScope=newScope("output");
   funcScope->nestedLevel=nestedLevel;
   pushScope(funcScope);
                
  
  st_insert("Variable", "value",0,"int",currentScope);
  
  popScope();    
  
}

/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
char * nodeKindToString(TreeNode * t){
  switch (t->nodekind) {
		case DeclK:
			switch (t->kind.decl) {
				case VarK:
          return "decl var";
        case ArrK: 
          return "decl arr";
				case FuncK:
          return "decl func";
				case ParamK:
          return "decl param";
        case ArrParamK:
          return "decl arrparam";
				case VoidParamK:
          return "decl voidparam";
				
				default:
					break;
			}
			break;

		case StmtK:
			switch (t->kind.stmt) {
				case CompoundK: 
          return "stmt compound";
        case IfK:
           
          return "stmt if";
        case ElseK:
          return "stmt else";
        case WhileK:
          return "stmt while";
        case ReturnK:
          return "stmt return";
        default:
            break;
            
    }
			break;

		case ExpK:
  
			switch (t->kind.exp) {
        
				case CallK:
					return "exp call";
				case IdK:
           return "exp id";
				case OpK:
          return "exp op";
        case AssignK:
          return "exp assgin";
        case ConstK:
          return "exp const";
        case TypeK:
          return "exp type";
        case VarExpK:
          return "exp varexp";
        

				default:
					break;
			}
			break;

		default:
			break;
	}
}
static void traverse( TreeNode * t, void (* preProc) (TreeNode *), void (* postProc) (TreeNode *) ){ 
  if (t != NULL)
  {  //printf("Traversing node: %s lineno:%d\n", nodeKindToString(t),t->lineno);
    preProc(t);
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


void postProc(TreeNode *t) {
    if (t->nodekind == StmtK && t->kind.stmt == CompoundK) {
        popScope();      // 스코프 닫기
        scopeDepth--;   
    }
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
            insertLines(t->attr.name, t->lineno);
          } else if (t->type == Void || t->type == VoidArray) {
              error_void_call_index(listing,"void",t->attr.name,t->lineno);
              char *type = (t->kind.decl == ArrK) ? "void[]" : "void";
              st_insert("Variable",t->attr.name, t->lineno, type, currentScope);
          } else {
              char *type = (t->kind.decl == ArrK) ? "int[]" : "int";
              st_insert("Variable",t->attr.name, t->lineno, type, currentScope);
          }
          break;
          

				case FuncK: // 함수 처리
          if (st_lookup_top(t->attr.name)) { // 현재 스코프에서 중복 확인
             error_redefine(listing, t);
             insertLines(t->attr.name, t->lineno);
              
          } else {
              ParameterList params = NULL;
              char * type=t->type==Integer?"int":"void";
              st_insert_function("Function",t->attr.name,t->lineno,type,currentScope,params);
              //printf("%s: %s\n",t->attr.name,type);
              nestedLevel++;
              Scope funcScope = newScope(t->attr.name);
              funcScope->nestedLevel=nestedLevel;
              pushScope(funcScope);
              scopeDepth=1;
              
              currentParameterList = &(currentScope->paramlist[hash(t->attr.name)]);
              
          }
          
          break;
      
				case ParamK:
        //printf("cur scope=%s, var=%s\n",currentScope->name,t->attr.name);
        if (st_lookup_top(t->attr.name)) { //현재 function 인자에 중복으로 선언된 거 있는 지 확인하는 
            error_redefine(listing, t);
            insertLines(t->attr.name, t->lineno);
          } else {
            st_insert("Variable", t->attr.name,t->lineno,"int",currentScope);
            addParameter(currentParameterList, t->attr.name, "int");
          }
          break;
        
				case ArrParamK:
					if (st_lookup(t->attr.name)) {
						error_redefine(listing, t);
            insertLines(t->attr.name, t->lineno);
					}  else if (t->type == VoidArray) { // void[] 배열 선언
              error_void_call_index(listing,"void",t->attr.name,t->lineno);
              st_insert("Variable",t->attr.name, t->lineno, "void[]",currentScope);
            addParameter(currentParameterList, t->attr.name, "void[]");
          } else {
						st_insert("Variable",t->attr.name, t->lineno, "int[]",currentScope);
            addParameter(currentParameterList, t->attr.name, "int[]");
					}
					break;
  
				case VoidParamK:
					break;

				default:
					break;
			}
			break;

		case StmtK:
			switch (t->kind.stmt) {
				case CompoundK: 
          //printf("compound. curscope=%s\n",currentScope->name);
          char scopeName[64];

           if (scopeDepth== 1) {
            //printf("scopeDepth==1\n");
            scopeDepth++;
          } else {
            //printf("scopeDepth is not 1\n");
            // 중첩된 Compound는 고유 이름 생성
            snprintf(scopeName, sizeof(scopeName), "%s_%d", currentScope->name,scopeDepth- 1);
            Scope newscope = newScope(scopeName);
            nestedLevel++;
            newscope->nestedLevel=nestedLevel;
            pushScope(newscope);
            scopeDepth++;
            
          }
          break;
        
				default:
			    break;
      }
      break;
		case ExpK:
			switch (t->kind.exp) {
				case CallK:
					BucketList list = st_lookup_bucket(t->attr.name);
          if (list == NULL) {
              error_undeclared(listing,"undeclared_func",t->attr.name,t->lineno);
              st_insert_function("Function",t->attr.name,t->lineno,"undetermined",currentScope,NULL);
          } else {
              //printf("ExpK(call): %s\n",t->attr.name);
              t->type = (strcmp(list->type, "int") == 0) ? Integer : Void;
              insertLines(t->attr.name, t->lineno); // 라인 번호 추가
          }
					break;
				case VarExpK:
          BucketList list2 = st_lookup_bucket(t->attr.name);
          if (list2 == NULL) {
              error_undeclared(listing,"undeclared_id",t->attr.name,t->lineno);
              st_insert("Variable",t->attr.name,t->lineno,"undetermined",currentScope);
          } else {
              //printf("ExpK: %s\n",t->attr.name);
              t->type = (strcmp(list2->type, "int") == 0) ? Integer : Void;
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

    scope->currentParamList = NULL;
    scope->nestedLevel = nestedLevel; // nestedLevel 저장
    scopeStack[++topScope] = scope;
    currentScope = scope;
   // fprintf(stderr, "Pushed scope: %s (Nested Level: %d)\n", scope->name, nestedLevel);
}


void popScope() {
    if (topScope < 0) {
        fprintf(stderr, "Error: Scope stack underflow. No scopes to pop.\n");
        exit(EXIT_FAILURE);
    }

      //fprintf(stderr, "Current scope before pop: %s\n", currentScope->name);
    // 함수 매개변수 리스트 연결 처리
    if(strcmp("global",currentScope->name)!=0){
      for (int i = 0; i < SIZE; i++) {
          BucketList funcBucket = currentScope->parent->hashTable[i];
          if (funcBucket != NULL && strcmp(currentScope->name,funcBucket->name)==0) {
            //fprintf(stderr, "HashTable[%d]: %s\n", i, funcBucket->name);
        
            while (funcBucket != NULL) {
                if (strcmp(funcBucket->kind, "Function") == 0) {
                    // 현재 스코프의 paramlist[i]를 함수 BucketList의 params로 연결
                    if(strcmp("output",funcBucket->name)!=0) funcBucket->params = currentScope->paramlist[i];
                    //fprintf(stderr, "Linked parameters to function '%s'.\n", funcBucket->name);
                     // 연결된 파라미터 확인 출력
                    ParameterList temp = funcBucket->params;
                    //fprintf(stderr, "Parameters for function '%s':\n", funcBucket->name);
                    while (temp != NULL) {
                        //fprintf(stderr, "- %s (%s)\n", temp->name, temp->type);
                        temp = temp->next;
                    }
                }
                funcBucket = funcBucket->next;
            }
          }
      }
    }
    // 기존 스코프 닫기 작업
    //fprintf(stderr, "Popped scope: %s (Nested Level: %d)\n", currentScope->name, nestedLevel);
    scopeStack[topScope] = NULL;
    topScope--;

    currentScope = (topScope >= 0) ? scopeStack[topScope] : NULL;

    if (nestedLevel > 0) {
        nestedLevel--;
    }
}


/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{ //우리는 postOrder에서 scope 닫아야 하니까... nullProc 대신 scope 관리 함수 전달 
  //맨 처음에 새로 global scope 만들고, 현재 scope를 global로 설정... 
    globalScope = newScope("global");
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



/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t) {
  switch (t->nodekind) {
    case StmtK:
      switch (t->kind.stmt) {
        case AssignK: {
          // Verify the type match of two operands when assigning.
          if (t->child[0]->attr.arr.type == IntegerArray) {
            typeError(t->child[0], "Assignment to Integer Array Variable");
          }

          if (t->child[0]->attr.type == Void) {
            typeError(t->child[0], "Assignment to Void Variable");
          }
          break;
        }
        case ReturnK: {
          const TreeNode * funcDecl;
          const ExpType funcType = funcDecl->type;
          const TreeNode * expr = t->child[0];

          if (funcType == Void && (expr != NULL && expr->type != Void)) {
             typeError(t,"expected no return value");
           } else if (funcType == Integer && (expr == NULL || expr->type == Void)) {
             typeError(t,"expected return value");
           }
        }
        default:
          break;
       }
       break;
     default:
       break;
   }
}
/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree) {
  traverse(syntaxTree, nullProc, checkNode);
}