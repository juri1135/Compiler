/****************************************************/
/* File: tiny.l                                     */
/* Lex specification for TINY                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

%{
#include "globals.h"
#include "util.h"
#include "scan.h"
/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];
int ch=0;
%}

digit       [0-9]
number      {digit}+
letter      [a-zA-Z]
identifier  {letter}({letter}|{digit})*
newline     \n
whitespace  [ \t]+

%%
"int"              { return INT; }
"if"               { return IF; }
"else"             { return ELSE; }
"while"            { return WHILE; }
"return"           { return RETURN; }
"void"             { return VOID; }
"+"                { return PLUS; }
"-"                { return MINUS; }
"*"                { return TIMES; }
"/"                { return OVER; }
"<"                { return LT; }
"<="               { return LE; }
">"                { return GT; }
">="               { return GE; }
"=="               { return EQ; }
"!="               { return NE; }
"="                { return ASSIGN; }
";"                { return SEMI; }
","                { return COMMA; }
"("                { return LPAREN; }
")"                { return RPAREN; }
"["                { return LBRACE; }
"]"                { return RBRACE; }
"{"                { return LCURLY; }
"}"                { return RCURLY; }
{number}           { return NUM; }
{identifier}       { return ID; }
{newline}          { lineno++; }
{whitespace}       {/* skip whitespace */}
.                  {return ERROR; }
<<EOF>>            {return ENDFILE; }
"/*" { 
    char current;
    char next;
    while (1) {
      current = input();
      /* 만나면 다음 input /이거인지 확인 */
      if (current == '*') { 
        next=input();
        if(next=='/') break;
        else unput(next);
      }
      // 주석 다 안 끝나고 EOF 만나면 그냥 정상 종료... 
      else if (current == 0 || current == EOF) {
        return ENDFILE;
      } 
      
      else if (current == '\n') {
        lineno++; 
      }
    }
}

%%

TokenType getToken(void)
{ static int firstTime = TRUE;
  TokenType currentToken;
  if (firstTime)
  { firstTime = FALSE;
    lineno++;
    yyin = source;
    yyout = listing;
  }
  currentToken = yylex();
  strncpy(tokenString,yytext,MAXTOKENLEN);
  if (TraceScan) {
    fprintf(listing,"\t%d: ",lineno);
    printToken(currentToken,tokenString);
  }
  return currentToken;
}
