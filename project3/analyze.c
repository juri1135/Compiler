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
Scope scopeArray[SIZE];
static int scopeIndex;
char *funcName;  
static int checkIndex;
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
  if(strcmp(type,"return")==0){
    fprintf(listing, "Error: Invalid return at line %d\n", lineno);
  }else if(strcmp(type,"assign")==0){
    fprintf(listing, "Error: invalid assignment at line %d\n", lineno);
  }else if(strcmp(type,"operation")==0){
    fprintf(listing, "Error: invalid operation at line %d\n", lineno);
  }else{
    fprintf(listing, "Error: invalid condition at line %d\n", lineno);
  }
}
void error_redefine(FILE *listing, TreeNode *t,char * kind) {
    // BucketList에서 심볼 정보 가져오기
    BucketList bucket = st_lookup_bucket(t->attr.name,kind);
    if (bucket == NULL) {
        fprintf(listing, "Error: Internal issue - no bucket found for symbol \"%s\".\n", t->attr.name);
        return; // 안전하게 함수 종료
    }
    LineList linelist = bucket->lines;

    fprintf(listing, "Error: Symbol \"%s\" is redefined at line %d (already defined at lines: ", t->attr.name, t->lineno);

    LineList current = linelist;
    while (current != NULL) {
        fprintf(listing, "%d", current->lineno); 
        if (current->next != NULL) {
            fprintf(listing, " "); 
        }
        current = current->next; 
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
 char * nodeTypeToString(TreeNode *t){
        switch (t->type) {
            case Integer:
              return "int";
            case IntArray:
                return"int[]";
            case Void:
                return"void";
            case VoidArray:
                return"void[]";
            default:
                return"undetermined";
        }
 }
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
					return "";
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
          return "";
            
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
				return "";
			}
			break;

		default:
			return "";
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
        popScope();      
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
          if (st_lookup_top(t->attr.name,"Variable")) {
            error_redefine(listing, t,"Variable");
            insertLines(t->attr.name, t->lineno,"Variable");
          } else if (t->type == Void || t->type == VoidArray) {
              error_void_call_index(listing,"void",t->attr.name,t->lineno);
              char *type = (t->kind.decl == ArrK) ? "void[]" : "void";
              st_insert("Variable",t->attr.name, t->lineno, type, currentScope);
          } else {
              char *type = (t->kind.decl == ArrK) ? "int[]" : "int";
              st_insert("Variable",t->attr.name, t->lineno, type, currentScope);
          }
          break;
          
				case FuncK: 
          if (st_lookup_top(t->attr.name,"Function")) {
             error_redefine(listing, t,"Function");
             insertLines(t->attr.name, t->lineno,"Function");
          } else {
              ParameterList params = NULL;
              char * type=t->type==Integer?"int":"void";
              st_insert_function("Function",t->attr.name,t->lineno,type,currentScope,params);
              //printf("%s: %s\n",t->attr.name,type);
              nestedLevel++;
              Scope funcScope = newScope(t->attr.name);
              funcScope->nestedLevel=nestedLevel;
              pushScope(funcScope);
              scopeArray[scopeIndex++]=funcScope;
              for(int i=0; i<scopeIndex; i++){
                //printf("%s ",scopeArray[i]->name);
              }//printf("\n");
              scopeDepth=1;
              currentParameterList = &(currentScope->paramlist[hash(t->attr.name)]);
          }
          break;
      
				case ParamK:
          //printf("cur scope=%s, var=%s\n",currentScope->name,t->attr.name);
          if (st_lookup_top(t->attr.name,"Variable")) { //현재 function 인자에 중복으로 선언된 거 있는 지 확인하는 
            error_redefine(listing, t,"Variable");
            insertLines(t->attr.name, t->lineno,"Variable");
          } else if(t->type==Void){
            error_void_call_index(listing,"void",t->attr.name,t->lineno);
            st_insert("Variable",t->attr.name, t->lineno, "void",currentScope);
            addParameter(currentParameterList, t->attr.name, "void");
          } else {
            st_insert("Variable", t->attr.name,t->lineno,"int",currentScope);
            addParameter(currentParameterList, t->attr.name, "int");
          }
          break;
        
				case ArrParamK:
					if (st_lookup_top(t->attr.name,"Variable")) {
						error_redefine(listing, t,"Variable");
            insertLines(t->attr.name, t->lineno,"Variable");
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
          }else {
            //printf("scopeDepth is not 1\n");
            //중첩된 Compound는 고유 이름 생성
            snprintf(scopeName, sizeof(scopeName), "%s_%d", currentScope->name, currentScope->childCounter++);
            Scope newscope = newScope(scopeName);
            scopeArray[scopeIndex++]=newscope;
            for(int i=0; i<scopeIndex; i++){
              //printf("%s ",scopeArray[i]->name);
            }
            //printf("\n");
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
					BucketList list = st_lookup_bucket(t->attr.name,"Function");
          if (list == NULL) {
              error_undeclared(listing,"undeclared_func",t->attr.name,t->lineno);
              st_insert_function("Function",t->attr.name,t->lineno,"Undetermined",currentScope,NULL);
              t->type=Undetermined;
          } else {
              //printf("ExpK(call): %s\n",t->attr.name);
              t->type = (strcmp(list->type, "int") == 0) ? Integer : Void;
              insertLines(t->attr.name, t->lineno,"Function"); 
          }
					break;
				case VarExpK:
          BucketList list2 = st_lookup_bucket(t->attr.name,"Variable");
          if (list2 == NULL) {
              error_undeclared(listing,"undeclared_id",t->attr.name,t->lineno);
              st_insert("Variable",t->attr.name,t->lineno,"Undetermined",currentScope);
              t->type=Undetermined;
          } else {
              //printf("ExpK: %s\n",t->attr.name);
              t->type = (strcmp(list2->type, "int") == 0) ? Integer : Void;
              insertLines(t->attr.name, t->lineno,"Variable"); 
          }
					break;
        case AssignK:
          t=t->child[0];
          BucketList list3 = st_lookup_bucket(t->attr.name,"Variable");
          if (list3 == NULL) {
              error_undeclared(listing,"undeclared_id",t->attr.name,t->lineno);
              st_insert("Variable",t->attr.name,t->lineno,"Undetermined",currentScope);
              t->type=Undetermined;
          } else {
              //printf("ExpK: %s\n",t->attr.name);
              t->type = (strcmp(list3->type, "int") == 0) ? Integer : Void;
              insertLines(t->attr.name, t->lineno,"Variable"); 
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
      //fprintf(stderr, "Current scope before pop: %s\n", currentScope->name);
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
    scopeIndex=0;
    scopeArray[scopeIndex++] = globalScope;
  //input, output 함수 삽입 이건 AST에 안 남기도 하고... 코드에 없으니까 scopelist엔 추가 안 하는 걸로...
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
int checkFunctionCall(TreeNode *callNode, BucketList funcBucket, FILE *listing) {
  ParameterList param = funcBucket->params;
  TreeNode *arg = callNode->child[0];
  //printf("check function call\n");
  //if(param==NULL) printf("%s void param\n",funcBucket->name);
  ParameterList tempParam = param;
  while(tempParam != NULL){
    //printf("Param - name: %s, type: %s\n", tempParam->name, tempParam->type);
    tempParam = tempParam->next;
  }
  TreeNode *tempArg = arg;
  while(tempArg != NULL){
    //printf("Arg s, type: %d\n", tempArg->type);
    tempArg = tempArg->sibling;
  }
  param = funcBucket->params;
  arg = callNode->child[0];

  while (param != NULL && arg != NULL) {
    char *argType = nodeTypeToString(arg);
    if (strcmp(argType, param->type) != 0) {
      error_void_call_index(listing, "invalidfunc", callNode->attr.name, callNode->lineno);
      return 0;
    }
    param = param->next;
    arg = arg->sibling;
  }
  if (param != NULL || arg != NULL) {
    error_void_call_index(listing, "invalidfunc", callNode->attr.name, callNode->lineno);
    return 0;
  }
  return 1;
}
/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode *t) {
  if (t == NULL) return;

  switch (t->nodekind) {
    case ExpK:
    //기존에 존재하지 않는 변수나 함수 호출하는 건 symbol table build하면서 체크함
    //함수 호출하면 cur scope->global까지 가면서 해당 함수 찾고 symbol table에 저장된 param과 일치하는 지 확인
    //return type은 returnK에서 확인해주면 됨
      switch (t->kind.exp) {
        case OpK: // 연산자 노드
          //printf("op %s, %s\n",nodeTypeToString(t->child[0]),nodeTypeToString(t->child[1]));
          //int+int, void+void, int[]+int[]... 이 중 int op int만 허용
           if (t->child[0]->type == Integer && t->child[1]->type == Integer) {
              t->type=Integer;
          }else {
            error_invalid(listing, "operation", t->lineno);
              // t->type=Undetermined;
          }
          break;
          //int=int 혹은 int[]=int[]만 허용
        case AssignK:
          BucketList bucket = st_lookup_bucket(t->child[0]->attr.name,"Variable");
          if(bucket == NULL){
            //undeclared는 이미 출력한 상태
            //error_undeclared(listing,"var",t->attr.name,t->lineno);
            // t->type = Undetermined;
          }
          //printf("assgin %s, %s\n",nodeTypeToString(t->child[0]),nodeTypeToString(t->child[1]));
          if(t->child[0]->type==IntArray&&t->child[1]->type==IntArray){
            t->type=IntArray;
          }
          else if(t->child[0]->type==Integer&&t->child[1]->type==Integer){
            t->type=Integer;
          }
          else{
            error_invalid(listing,"assign",t->lineno);
            // t->type=Undetermined;
          }
          break;
        case ConstK:
            t->type = Integer;
            break;
            //id 혹은 id[]... 여기선 index 체크만 하면 됨. 
        case VarExpK:
          //일단 [] 안에 들어간 게 있다면 Integer인지 보고 int 아니면 에러
          //그리고 id[]가 정말 intArr인 지 확인.(symbol table 기반) intArr이라면 t는 intger
          if(t->child[0] != NULL) {
            if(t->child[0]->type != Integer){
              error_void_call_index(listing, "indexNotInt", t->attr.name,t->lineno);
              t->type=Integer;
              // t->type = Undetermined;
            } else {
              //변수의 타입을 심볼 테이블에서 가져와야 함
              //!이것도 preorder에서 scope 결정해서 current scope 알게 된 상태로 들어가야
              BucketList bucket = st_lookup_bucket(t->attr.name,"Variable");
              if(bucket == NULL){
                //undeclared는 이미 출력한 상태
                printf("undeclared %s %d\n",t->attr.name, t->lineno);
                // t->type = Undetermined;
              }
              else {
                if(strcmp(bucket->type,"int[]")==0){
                  //printf("int[int]=int %s %d\n",t->attr.name,t->lineno);
                  t->type = Integer; // 배열의 요소 타입이 Integer라면
                }
                else {
                  //void 배열인 경우는 이미 build symbol table에서 출력
                  //printf("voidarr%s %d\n",t->attr.name,t->lineno);
                  // t->type = Undetermined;
                }
              }
            }
          }
          break;

        case CallK: { // 함수 호출
          //호출 자체는 부모에 있는 함수 호출 가능하니까... st_lookup
          //여기 오기 전에 current Scope이 다 지정이 되어 있어야 함
          //printf("callk curSope: %s, lineno: %d name: %s\n",currentScope->name,t->lineno,t->attr.name);
          BucketList funcBucket = st_lookup_bucket(t->attr.name,"Function");
          if (funcBucket == NULL) {
            //이미 undeclare는 symbol table 만들 때 출력했음
            break;
          } 
          if(strcmp(funcBucket->type,"Undetermined")==0){
            error_void_call_index(listing,"func",t->attr.name,t->lineno);
            break;
          }
          //printf("%s\n",funcBucket->name);
          checkFunctionCall(t, funcBucket, listing);
          t->type = (strcmp(funcBucket->type,"int")==0) ? Integer : Void;
          // if (checkFunctionCall(t, funcBucket, listing)) {
          //     t->type = (strcmp(funcBucket->type,"int")==0) ? Integer : Void;
          // } else {
          //     //t->type = Undetermined;
          // }
          break;
        }
        default:
          break;
      }
      break;

      case StmtK:
        switch (t->kind.stmt) {
          case IfK: 
          case ElseK:
          case WhileK:
            if (t->child[0]->type != Integer) {
              error_invalid(listing,"condition",t->lineno);
            }
            break;
          case CompoundK:
            //printf("compound cur scope: %s, checkindex: %d\n",currentScope->name,checkIndex);
            checkIndex-=2;
            currentScope=scopeArray[checkIndex];
            scopeDepth--;
            //printf("compound after curSope: %s, lineno: %d name: %s\n",currentScope->name,t->lineno,t->attr.name);
            break;
          case ReturnK: { // return문
            // 현재 함수의 반환 타입을 심볼 테이블에서 가져옴
            // printf("returnk curSope: %s, lineno: %d fucname: %s\n",currentScope->name,t->lineno,funcName);
            //returnk에는 name type이 없음... checkScope에서 funck 보이면 전역변수로 설정해줘야 될듯
            BucketList funcBucket = st_lookup_bucket(funcName,"Function");
            //returnK의 child0이 return하는 expression. 
            //if(funcBucket==NULL) printf("null\n");
            //else  printf("%s\n",funcBucket->name);
            ExpType funcType;
            if (strcmp(funcBucket->type,"int")==0) {
              //printf("return type: int\n");
              funcType = Integer;
            } else if (funcBucket->type==Void) {
              //printf("return type: void\n");
              funcType = Void;
            } else {
              //printf("return type: undetermined\n");
              funcType = Undetermined;
            }
            if (t->child[0] != NULL) {
              if (funcType != t->child[0]->type) {
                error_invalid(listing,"return",t->lineno);
              }
            } else {
              if (funcType != Void) {
                error_invalid(listing,"return",t->lineno);
              }
            }
            break;
          }
          default:
            break;
        }
        break;

      default:
        break;
    }
}
static void checkScope(TreeNode *t) {
  switch (t->nodekind) {
    case DeclK:
      switch (t->kind.decl) {
        case FuncK:
        //printf("check funck %s, %d\n",t->attr.name,t->lineno);
          funcName=t->attr.name;
          if(checkIndex==scopeIndex){
            printf("index out of range!\n");
          }else{
            //printf("check scope funck curscope: %s\n",currentScope->name);
           currentScope=scopeArray[checkIndex++];
           scopeDepth=1;
           //printf("after check scope curscope: %s\n",currentScope->name);
          }break;
        default:
            break;
      }
      break;

    case StmtK:
      switch (t->kind.stmt){ 
        case CompoundK:
        //printf("check compound %s, %d\n",t->attr.name,t->lineno);
          if(scopeDepth==1){
            //printf("첫 번째 compound %d\n",t->lineno);
            scopeDepth++;
          }
          else{
            //printf("check scope compound curscope: %s\n",currentScope->name);
            currentScope=scopeArray[checkIndex++];
            scopeDepth--;
            //printf("after check scope curscope: %s\n",currentScope->name);
          }
          break;
        default:
          break;
      }
      break;

    case ExpK:
      break;

    default:
      break;
  }
}
/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode *syntaxTree) {

    //printf("%d\n",scopeIndex);
    for(int i=0; i<scopeIndex; i++){
      //printf("%s ",scopeArray[i]->name);
    }
    checkIndex=0;
    currentScope=scopeArray[checkIndex++];
    traverse(syntaxTree, checkScope, checkNode);
}