#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdio.h>

/* SIZE is the size of the hash table */
#define SIZE 211

/* SHIFT is the power of two used as multiplier in hash function */
#define SHIFT 4

/* Enumeration for symbol kinds */
typedef enum { FUNCTION, VARIABLE } SymbolKind;

/* Structure for line numbers where a symbol appears */
typedef struct LineListRec {
    int lineno;
    struct LineListRec *next;
} *LineList;

/* Structure for parameter list */
typedef struct ParamListRec {
    char *name;                 // 매개변수 이름
    char *type;                 // 매개변수 타입 (예: int, void 등)
    struct ParamListRec *next;  // 다음 매개변수 (연결 리스트)
} *ParameterList;

/* Structure for symbols in the hash table */
typedef struct BucketListRec
   { char * name;
     LineList lines;
     int memloc ; /* memory location for variable */
     struct BucketListRec * next;
     char *type;
     char *kind;
     ParameterList params;
   } * BucketList;

/* Structure for scopes */
typedef struct ScopeListRec {
    char *name;                     // 스코프 이름
    int nestedLevel;                // 중첩 수준
    struct ScopeListRec *parent;    // 부모 스코프
    BucketList hashTable[SIZE];     // 해시 테이블 (심볼 리스트)
    ParameterList paramlist[SIZE];  // 매개변수 리스트 해시 테이블
    ParameterList *currentParamList;
} *Scope;
/* SIZE is the size of the hash table */
#define SIZE 211

/* SHIFT is the power of two used as multiplier in hash function */
#define SHIFT 4
/* Function prototypes */
extern Scope scopeStack[SIZE];
extern int topScope;
extern Scope currentScope;
extern Scope scopeList[SIZE];
extern int sizeOfList;

static int hash(char * key);
/* Scope management */
Scope newScope(char *name);
void pushScope(Scope scope);
void popScope();

/* Symbol table operations */
void st_insert( char* kind,char * name, int lineno, int loc, char *type, Scope scope );
void st_insert_function(Scope scope, char *name, char *type, ParameterList params);
BucketList st_lookup_bucket(char *name);
int st_lookup(char *name);
BucketList st_lookup_top(char *name);

/* Line number insertion */
void insertLines(char *name, int lineno);

/* Parameter list management */
void addParameter(ParameterList *plist, char *name, char *type);

/* Symbol table printing */
void printSymTab(FILE *listing);
void printFunc(FILE *listing);
void printGlob(FILE *listing);
void printScope(FILE *listing);

#endif /* SYMTAB_H */
