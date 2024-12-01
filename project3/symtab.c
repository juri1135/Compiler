/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"


Scope currentScope = NULL;    // 현재 활성화된 스코프
int topScope = -1;            // 스코프 스택의 최상위 인덱스
int sizeOfList = 0;           // scopeList의 현재 크기
Scope scopeList[SIZE] = {0}; 
Scope scopeStack[SIZE];



/* The record in the bucket lists for
 * each variable, including name, 
 * assigned memory location, and
 * the list of line numbers in which
 * it appears in the source code
 */


/* the hash function */



Scope newScope(char *name) {
    Scope scope = (Scope)malloc(sizeof(struct ScopeListRec));
    scope->location=0;
    scope->name = strdup(name);
    scope->nestedLevel = (currentScope == NULL) ? 0 : currentScope->nestedLevel + 1;
    scope->parent = currentScope;
    scope->childCounter = 1;
    memset(scope->hashTable, 0, sizeof(scope->hashTable));
    memset(scope->paramlist, 0, sizeof(scope->paramlist));
    
    // scopeList에 추가
    if (sizeOfList < SIZE) {
        scopeList[sizeOfList++] = scope;
    } else {
        fprintf(stderr, "Error: scopeList overflow.\n");
        exit(EXIT_FAILURE);
    }
    
    return scope;
}

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert( char* kind,char * name, int lineno, char *type, Scope scope )
{   int h = hash(name);
    BucketList l = scope->hashTable[h];
  //symbol table linked list 순회하면서 해당 변수 이미 존재하는 지 확인... 
  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
  //만약 끝까지 갔는데 NULL이면 아직 정의되지 않은 거니까 symbol table에 추가
  if (l == NULL) {
      l = (BucketList)malloc(sizeof(struct BucketListRec));
      l->name = strdup(name);             // 이름 복사
      l->lines = (LineList)malloc(sizeof(struct LineListRec));
      l->lines->lineno = lineno;          // 첫 번째 라인 번호
      l->memloc = scope->location++;                    // 메모리 위치
      l->lines->next = NULL;
      l->type = strdup(type);             
      l->kind=strdup(kind);
      l->next = scope->hashTable[h];      // 연결 리스트로 추가
      scope->hashTable[h] = l;}
  else /* found in table, so just add line number */
  { LineList t = l->lines;
    while (t->next != NULL) t = t->next;
    t->next = (LineList) malloc(sizeof(struct LineListRec));
    t->next->lineno = lineno;
    t->next->next = NULL;
  }
} /* st_insert */
void st_insert_function(char* kind,char * name, int lineno, char *type, Scope scope, ParameterList params) {
    int h = hash(name);
    BucketList l = scope->hashTable[h];

    l = (BucketList)malloc(sizeof(struct BucketListRec));
    l->name = strdup(name);             // 이름 복사
    l->lines = (LineList)malloc(sizeof(struct LineListRec));
    l->lines->lineno = lineno;          // 첫 번째 라인 번호
    l->memloc = scope->location++;                // 메모리 위치
    l->lines->next = NULL;
    l->type = strdup(type);             
    l->kind=strdup(kind);
    l->next = scope->hashTable[h];      // 연결 리스트로 추가
    scope->hashTable[h] = l;
    l->params=params;
    // paramlist에 매개변수 리스트 저장
    scope->paramlist[h] = params;
    //fprintf(stderr, "Function '%s' inserted with return type '%s'.\n", name, type);
    
}
void addParameter(ParameterList *plist, char *name, char *type) {
    ParameterList newParam = (ParameterList)malloc(sizeof(struct ParamListRec));
    newParam->name = strdup(name);
    newParam->type = strdup(type);
    newParam->next = NULL;

    if (*plist == NULL) {
        *plist = newParam;  // 첫 번째 매개변수
    } else {
        ParameterList temp = *plist;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newParam;  // 마지막에 추가
    }

    // Debugging 출력
    //fprintf(stderr, "Added parameter: %s (%s)\n", name, type);

    // 현재 매개변수 리스트 출력
    //fprintf(stderr, "Current Parameter List:\n");
    ParameterList temp = *plist;
    while (temp != NULL) {
        //fprintf(stderr, "- %s (%s)\n", temp->name, temp->type);
        temp = temp->next;
    }
}
BucketList st_lookup_bucket(char *name) {
    Scope scope = currentScope;
    while (scope != NULL) {
        int h = hash(name);
        BucketList l = scope->hashTable[h];
        while (l != NULL) {
            if (strcmp(name, l->name) == 0) {
                return l; // 심볼 발견
            }
            l = l->next;
        }
        scope = scope->parent; // 부모 스코프로 이동
    }
    return NULL; // 심볼 미발견
}
/* Function to insert a line number into the symbol's line list */
void insertLines(char *name, int lineno) {
    BucketList symbol = st_lookup_bucket(name);
        LineList currentLine = symbol->lines;
        if (currentLine == NULL) {
            // 라인 리스트가 비어있는 경우 새 노드 생성
            symbol->lines = (LineList) malloc(sizeof(struct LineListRec));
            if (symbol->lines == NULL) {
                fprintf(stderr, "Error: Memory allocation failed for LineList.\n");
                exit(EXIT_FAILURE);
            }
            symbol->lines->lineno = lineno;
            symbol->lines->next = NULL;
        } else {
            // 기존 라인 리스트의 끝으로 이동
            while (currentLine->next != NULL) {
                currentLine = currentLine->next;
            }
            // 새로운 라인 번호 추가
            currentLine->next = (LineList) malloc(sizeof(struct LineListRec));
            if (currentLine->next == NULL) {
                fprintf(stderr, "Error: Memory allocation failed for LineList.\n");
                exit(EXIT_FAILURE);
            }
            currentLine->next->lineno = lineno;
            currentLine->next->next = NULL;
        }
    
}


BucketList st_lookup_top(char *name){
   int h = hash(name);
    BucketList l = currentScope->hashTable[h]; // 현재 스코프에서만 검색
    while (l != NULL) {
        if (strcmp(name, l->name) == 0) {
            return l; // 현재 스코프에 존재하면 반환
        }
        l = l->next;
    }
    return NULL;
}
/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
int st_lookup(char *name) {
    Scope scope = currentScope;
    while (scope != NULL) {
        int h = hash(name);
        BucketList l = scope->hashTable[h];
        while (l != NULL) {
            if (strcmp(name, l->name) == 0) {
                return l->memloc;
            }
            l = l->next;
        }
        scope = scope->parent; // 상위 스코프로 이동
    }
    return -1; // 찾지 못함
}
//만약 해당 scope 찾는 거에서 -1을 return받았다면 
/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing)
{ int i;
  fprintf(listing," Symbol Name   Symbol Kind  Symbol Type  Scope Name  Location  Line Numbers\n");
  fprintf(listing,"-------------  -----------  ------------ ----------  --------  ------------\n");
  for (i=0;i<SIZE;++i){ 
    Scope scope = scopeList[i];
    if (scope == NULL) continue; // scope가 NULL이면 스킵
    for (int j = 0; j < SIZE; ++j) {
    BucketList l = scope->hashTable[j];
      while (l != NULL) {
        LineList t = l->lines;
        fprintf(listing, "%-14s %-12s %-12s %-11s %-8d ", l->name, l->kind, l->type, scope->name, l->memloc);
        while (t != NULL)
        { fprintf(listing,"%4d ",t->lineno);
          t = t->next;
        }
        fprintf(listing,"\n");
        l = l->next;
      }
    }
  }
  fprintf(listing,"\n");
  printFunc(listing);
  printGlob(listing);
  printScope(listing);
} /* printSymTab */

void printFunc(FILE *listing) {
    fprintf(listing, "< Functions >\n");
    fprintf(listing, "Function Name   Return Type   Parameter Name  Parameter Type\n");
    fprintf(listing, "-------------  -------------  --------------  --------------\n");

    for (int i = 0; i < SIZE; i++) {
        Scope scope = scopeList[i];
        if (scope == NULL) continue; // 스코프가 NULL이면 건너뜀

        for (int j = 0; j < SIZE; j++) {
            BucketList l = scope->hashTable[j];
            while (l != NULL) {
                if (strcmp(l->kind, "Function") == 0) {
                    if (strcmp(l->type, "undetermined") == 0) {
                        fprintf(listing, "%-15s %-13s %-16s %-14s\n", l->name, "undetermined", "", "undetermined");
                    } else {
                        fprintf(listing, "%-15s %-13s ", l->name, l->type);
                        
                        // 매개변수 리스트 처리
                        ParameterList p =  l->params;
                        if (p == NULL) {
                            fprintf(listing, "%-16s %-14s\n", "", "void");
                        } else {
                            fprintf(listing, "\n");
                            while (p != NULL) {
                                fprintf(listing, "%-15s %-13s %-16s %-14s\n", "-", "-", p->name, p->type);
                                p = p->next;
                            }
                        }
                    }
                }
                l = l->next;
            }
        }
    }
    fprintf(listing, "\n");
}/* printSymTab */


void printGlob(FILE *listing) {
    fprintf(listing, "< Global Symbols >\n");
    fprintf(listing, "Symbol Name   Symbol Kind   Symbol Type\n");
    fprintf(listing, "---------------------------------------\n");

    for (int i = 0; i < SIZE; i++) {
      Scope scope = scopeList[i];
      if (scope == NULL) continue; // scope가 NULL이면 스킵
        for (int j = 0; j < SIZE; ++j) {
          BucketList l = scope->hashTable[j];
            while (l != NULL) {
                if (strcmp(scope->name, "global") == 0) {
                    fprintf(listing, "%-12s %-13s %-12s\n", l->name, l->kind, l->type);
                }
                l = l->next;
            }
        }
    }
    fprintf(listing, "\n");
}


void printScope(FILE *listing) {
    fprintf(listing, "< Scopes >\n");
    fprintf(listing, "Scope Name   Nested Level   Symbol Name   Symbol Type\n");
    fprintf(listing, "-----------------------------------------------------\n");

    for (int i = 0; i < SIZE; i++) {
      Scope scope = scopeList[i];
      
      if (scope == NULL||strcmp(scope->name,"global")==0) continue; // scope가 NULL이면 스킵
        
        for (int j = 0; j < SIZE; ++j) {
            BucketList l = scope->hashTable[j];
            while (l != NULL) {
                fprintf(listing, " %-16s %-14d %-12s %-12s\n", scope->name, scope->nestedLevel, l->name, l->type);
                l = l->next;
            }
        }
    }
}