%option yylineno
%pointer

%{
    #include <cstdlib>
    #include <cstring>
    #include "ast.hpp"
    #include "parser.hpp"

    char* remove_first_and_last_char(const char *str) {
    int len = strlen(str);

    if (len <= 2) {  // if the string is too short to trim
        return strdup("");  // return an empty string
    }

    // Allocate memory for the new string (len - 1 for length, +1 for null terminator)
    char *new_str = (char*)malloc(len - 1);
    if (!new_str) {
        return NULL; // return NULL if memory allocation fails
    }

    // Copy the substring excluding the first and last characters
    strncpy(new_str, str + 1, len - 2);
    new_str[len - 2] = '\0';  // null-terminate the new string

    return new_str;
}

    /* void yyerror(const char *); */
%}

/** WRITE ME:
 *  Put any definitions you need here. Add your own regular expressions
 *  below for ALL tokens defined in the manual. You can choose not to use
 *  definitions here.
 */

/* HERE */

/** The example below is very simple, but it shows almost everything you will
 *  need to do for this first part. The syntax is as follows:
 *
 *  regular_expression      {  c code  }
 *
 * The lexer will try to match each regular expression. When it does it will
 * execute the corresponding C code.
*/


%%
"boolean"     {return BOOLEAN; }
"char"        {return CHARACTER; }
"integer"     {return INTEGER; }
"string"      {return STRING; }
"intptr"      {return INTEGERPOINTER; }
"charptr"     {return CHARPOINTER; }
"if"          {return IF; }
"else"        {return ELSE; }
"while"       {return WHILE; }
"var"         {return VAR; }
"procedure"   { return PROCEDURE; }
"return"      {return RETURN; }
"null"        {return NULLT; }
"true"        {return TRUE; }
"false"       {return FALSE; }
"&&"          {return AND; }
"/"         {return SLASH; }
"="           {return ASSIGNMENT; }
"=="          {return EQUALITY; }
">"           {return GREATER; }
">="          {return GREATEREQUAL; }
"<="          {return LESSEQUAL; }
"<"           {return LESS; }
"-"           {return MINUS; }
"!"           {return NOT; }
"!="          {return NOTEQUAL; }
"||"          {return OR; }
"+"         {return PLUS; }
"*"         {return MULTIPLY; }
"&"         {return REFERENCE; }
"^"         {return DEREFERENCE; }
";"         {return SEMICOLON; }
":"         {return COLON; }
","         {return COMMA; }
"|"         {return BAR; }
"{"         {return OPENBRACES; }
"}"         {return CLOSEBRACES; }
"("         {return OPENPARENTHESIS; }
")"         {return CLOSEPARENTHESIS; }
"["         {return OPENBRACKETS; }
"]"         {return CLOSEBRACKETS; }
[a-zA-Z][a-zA-Z0-9_]* {yylval.u_base_charptr = strdup(yytext);
                        return IDENTIFIER; }
[1-9][0-9]*|0 { yylval.u_base_int = (int)strtol(yytext,0, 10);  return INTEGERVAL_DEC; }
0x[0-9A-F]+|0X[0-9A-F]+ {yylval.u_base_int = (int)strtol(yytext,0,16); return INTEGERVAL_HEX;}
0[0-7]+ {yylval.u_base_int = (int)strtol(yytext,0,8); return INTEGERVAL_OCT;}
[01]*b {yylval.u_base_int = (int)strtol(yytext,0,2); return INTEGERVAL_BI;}
\'[a-zA-Z0-9 \\!@|~`#$%^&*()_\-\+\=\'\<\,\>\.\/\?\:\;\[\{\}\]"]\' { yylval.u_base_int = (int)(yytext[1]);
                                                                    return CHARVAL; }
\"[a-zA-Z0-9 \\!@|~`#$%^&*()_\-\+\=\'\<\,\>\.\/\?\:\;\[\{\}\]]*\" { yylval.u_base_charptr = remove_first_and_last_char(strdup(yytext));
                                                                return STRINGVAL; }
"/%"([^%]|(\*+[^%/]))*\%+\/ {};
[ \t\n]             {};



%%

/** You shall not pass!
 *  You should not  have to do or edit anything past this.
 */

int yywrap(void) {
    return 1;
}