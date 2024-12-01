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
    char *name;                 
    char *type;               
    struct ParamListRec *next;  
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
  int childCounter;
    int location;
    char *name;                    
    int nestedLevel;               
    struct ScopeListRec *parent;    
    BucketList hashTable[SIZE];     
    ParameterList paramlist[SIZE];  
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

static int hash ( char * key )
{ int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  { temp = ((temp << SHIFT) + key[i]) % SIZE;
    ++i;
  }
  return temp;
}
/* Scope management */
Scope newScope(char *name);
void pushScope(Scope scope);
void popScope();

/* Symbol table operations */
void st_insert( char* kind,char * name, int lineno, char *type, Scope scope );
void st_insert_function(char* kind,char * name, int lineno, char *type, Scope scope, ParameterList params);
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
