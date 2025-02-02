/****************************************************/
/* File: scan.c                                     */
/* The scanner implementation for the TINY compiler */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "util.h"
#include "scan.h"

/* states in scanner DFA */
typedef enum
   { START,INEQ,INCOMMENT,INNE,INLT,INGT,INNUM,INID,DONE,INCOMMENT_ }
   StateType;

/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];

/* BUFLEN = length of the input buffer for
   source code lines */
#define BUFLEN 256

static char lineBuf[BUFLEN]; /* holds the current line */
static int linepos = 0; /* current position in LineBuf */
static int bufsize = 0; /* current size of buffer string */
static int EOF_flag = FALSE; /* corrects ungetNextChar behavior on EOF */

/* getNextChar fetches the next non-blank character
   from lineBuf, reading in a new line if lineBuf is
   exhausted */
/*linepos로 어디까지 읽었는지 check...*/
static int getNextChar(void)
{ if (!(linepos < bufsize))
  { lineno++;
    if (fgets(lineBuf,BUFLEN-1,source))
    { if (EchoSource) fprintf(listing,"%4d: %s",lineno,lineBuf);
      bufsize = strlen(lineBuf);
      linepos = 0;
      return lineBuf[linepos++];
    }
    else
    { EOF_flag = TRUE;
      return EOF;
    }
  }
  else return lineBuf[linepos++];
}

/* ungetNextChar backtracks one character
   in lineBuf */
/*getNextChar에서 linepos++해서 다음 거 읽어가니까 --하면 이전 거로 reset*/
static void ungetNextChar(void)
{ if (!EOF_flag) linepos-- ;}

/* lookup table of reserved words */
static struct
    { char* str;
      TokenType tok;
    } reservedWords[MAXRESERVED]
   = {{"if",IF},{"else",ELSE},{"while",WHILE},
      {"return",RETURN},{"int",INT},{"void",VOID},};

/* lookup an identifier to see if it is a reserved word */
/* uses linear search */
static TokenType reservedLookup (char * s)
{ int i;
  for (i=0;i<MAXRESERVED;i++)
    if (!strcmp(s,reservedWords[i].str))
      return reservedWords[i].tok;
  return ID;
}

/****************************************/
/* the primary function of the scanner  */
/****************************************/
/* function getToken returns the 
 * next token in source file
 */
TokenType getToken(void)
{  /* index for storing into tokenString */
   int tokenStringIndex = 0;
   /* holds current token to be returned */
   TokenType currentToken;
   /* current state - always begins at START */
   StateType state = START;
   /* flag to indicate save to tokenString */
   int save;
   while (state != DONE)
   { int c = getNextChar();
    int next;
     save = TRUE;
     switch (state)
     {  case START:
         if (isdigit(c))
           state = INNUM;
         else if (isalpha(c))
           state = INID;
         else if (c=='!')
          state = INNE;
         else if (c=='>')
          state = INGT;
         else if (c=='<')
          state = INLT;
         else if (c == '=')
           state = INEQ;
         //incomment_는 incomment 상태에서만 가능하기 때문에 이 경우엔 바로 곱셈
         else if (c == '*') {
            state = DONE;
            currentToken = TIMES;
        }
         else if ((c == ' ') || (c == '\t') || (c == '\n'))
           save = FALSE;
         /* /를 받으면 over일 수도 주석일 수도 있으니 case 나눠서 state 저장*/
         else if (c == '/')
         { 
          next=getNextChar();
          if(next=='*'){
            /*기존 tiny에서 처리하던 문법과 동일*/
            save = FALSE;
            state = INCOMMENT;
          }
          else{
            ungetNextChar();
             state = DONE;
            currentToken = OVER;
          }
         }
         /* state가 START인데 모호성이 없는 경우 */
        else
         { 
          /*TOKEN 결정 나니까 DONE으로 수정*/
          state = DONE;
           switch (c)
           { case EOF:
               save = FALSE;
               currentToken = ENDFILE;
               break;
          
             case '+':
               currentToken = PLUS;
               break;
             case '-':
               currentToken = MINUS;
               break;
             case '(':
               currentToken = LPAREN;
               break;
             case ')':
               currentToken = RPAREN;
               break;
             case '{':
               currentToken = LCURLY;
               break;
             case '}':
               currentToken = RCURLY;
               break;
             case '[':
               currentToken = LBRACE;
               break;
             case ']':
               currentToken = RBRACE;
               break;
             case ';':
               currentToken = SEMI;
               break;
             case ',':
               currentToken = COMMA;
               break;
             default:
               currentToken = ERROR;
               break;
           }
         }
         break;
      /* 주석 내부 상태 / *를 받은 상태! */
       case INCOMMENT:
         save = FALSE;
         if (c == EOF)
         { state=DONE;
               currentToken = ENDFILE;
         }
         else if (c == '*'){
          state = INCOMMENT_;
         }
         break;
       case INCOMMENT_:
        save = FALSE;
        if (c == '/'){ state = START; }
        else if (c == EOF) {
        state = DONE;
        currentToken = ENDFILE;
    }
        else if (c=='*') {state =INCOMMENT_;}
        else {state = INCOMMENT;}
        break;
       case INEQ:
         state = DONE;
         if (c == '=')
           currentToken = EQ;
         else if (c == EOF) {
        state = DONE;
        currentToken = ENDFILE;
    }
         else
         { /* backup in the input */
           ungetNextChar();
           currentToken = ASSIGN;
         }
         break;

       case INGT:
         state = DONE;
         if (c == '=')
           currentToken = GE;
          else if (c == EOF) {
            state = DONE;
            currentToken = ENDFILE;
          }
         else
         { 
           ungetNextChar();
           save = FALSE;
           currentToken = GT;
         }
         break;
       case INLT:
         state = DONE;
         if (c == '=')
           currentToken = LE;
          else if (c == EOF) {
            state = DONE;
            currentToken = ENDFILE;
          }
         else
         { /* backup in the input */
           ungetNextChar();
           save = FALSE;
           currentToken = LT;
         }
         break;
       case INNE:
         state = DONE;
         if (c == '=')
           currentToken = NE;
          else if (c == EOF) {
            state = DONE;
            currentToken = ENDFILE;
          }
         else
         { /* backup in the input */
           ungetNextChar();
           save = FALSE;
           currentToken = ERROR;
         }
         break;
       
       case INNUM:
       if (c == EOF) {
            state = DONE;
            currentToken = ENDFILE;
          }
         else if (!isdigit(c))
         { /* backup in the input */
           ungetNextChar();
           save = FALSE;
           state = DONE;
           currentToken = NUM;
         }
         break;
       case INID:
         if (isalpha(c)||isdigit(c)){
          state = INID;
         }
         else if (c == EOF) {
            state = DONE;
            currentToken = ENDFILE;
          }
         else
         { /* backup in the input */
           ungetNextChar();
           save = FALSE;
           state = DONE;
           currentToken = ID;
         }
         break;
       case DONE:
       default: /* should never happen */
         fprintf(listing,"Scanner Bug: state= %d\n",state);
         state = DONE;
         currentToken = ERROR;
         break;
     }
     if ((save) && (tokenStringIndex <= MAXTOKENLEN))
       tokenString[tokenStringIndex++] = (char) c;
     if (state == DONE)
     { tokenString[tokenStringIndex] = '\0';
       if (currentToken == ID)
         currentToken = reservedLookup(tokenString);
     }
   }
   if (TraceScan) {
     fprintf(listing,"\t%d: ",lineno);
     printToken(currentToken,tokenString);
   }
   return currentToken;
} /* end getToken */

