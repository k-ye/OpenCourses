/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
    if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
        YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

/*
 *  Add Your own definitions here
 */

unsigned num_ml_comments = 0;

bool str_overflow();
int strlen_error();
void reset_str_buf();
void add_char(char c);

%}

/*
 * Define names for regular expressions here.
 */

DIGIT           [0-9]
ID_CHAR         [a-zA-Z0-9_]
DARROW          =>

/* Start Conditions */
%x STRING
%x STRING_ERR
%x COMMENT

%%

 /* Rules Section */

 /* Single line comments */

"--".*\n { 
    curr_lineno += 1;
}

"--".* { }

 /*
  *  Nested comments
  */

<INITIAL,COMMENT>"(*" {
    BEGIN(COMMENT);
    num_ml_comments += 1;
}

<COMMENT>\n { curr_lineno += 1; }

<COMMENT><<EOF>> {
    BEGIN(INITIAL);
    cool_yylval.error_msg = "EOF in comment";
    return ERROR;
}

<COMMENT>"*"+")" {
    num_ml_comments -= 1;
    
    if (num_ml_comments == 0) {
        BEGIN(INITIAL);
    }
}

<COMMENT>. { }

<INITIAL>"*"+")" {
    cool_yylval.error_msg = "Unmatched *)";
    return ERROR;
}

 /*
  *  The multiple-character operators.
  */
{DARROW}        { return (DARROW); }
"<-"            { return ASSIGN; }
"<="            { return LE; }
"+"             { return '+'; }
"-"             { return '-'; }
"*"             { return '*'; }
"/"             { return '/'; }
"("             { return '('; }
")"             { return ')'; }
"="             { return '='; }
"<"             { return '<'; }
"."             { return '.'; }
"~"             { return '~'; }
","             { return ','; }
";"             { return ';'; }
":"             { return ':'; }
"@"             { return '@'; }
"{"             { return '{'; }
"}"             { return '}'; }

 /*
  * Keywords are case-insensitive except for the values true and false,
  * which must begin with a lower-case letter.
  */

 /* Boolean const: true */
t(?i:rue) {
    cool_yylval.boolean = 1;
    return BOOL_CONST;
}
 /* Boolean const: false */
f(?i:alse) {
    cool_yylval.boolean = 0;
    return BOOL_CONST;
}

 /* Keywords */
(?i:class)      { return CLASS; }
(?i:else)       { return ELSE; }
(?i:fi)         { return FI; }
(?i:if)         { return IF; }
(?i:in)         { return IN; }
(?i:inherits)   { return INHERITS; }
(?i:isvoid)     { return ISVOID; }
(?i:let)        { return LET; }
(?i:loop)       { return LOOP; }
(?i:pool)       { return POOL; }
(?i:then)       { return THEN; }
(?i:while)      { return WHILE; }
(?i:case)       { return CASE; }
(?i:esac)       { return ESAC; }
(?i:new)        { return NEW; }
(?i:of)         { return OF; }
(?i:not)        { return NOT; }
 
 /* Integers */
{DIGIT}+ {
    cool_yylval.symbol = inttable.add_string(yytext);
    return INT_CONST; 
}

 /* Type identifiers */
[[:upper:]]{ID_CHAR}* {
    cool_yylval.symbol = idtable.add_string(yytext);
    return TYPEID;
}

 /* Object identifiers */
[[:lower:]]{ID_CHAR}* {
    cool_yylval.symbol = idtable.add_string(yytext);
    return OBJECTID;
}
 /*
  *  String constants (C syntax)
  *  Escape sequence \c is accepted for all characters c. Except for 
  *  \n \t \b \f, the result is c.
  *
  */

\" {
    string_buf_ptr = string_buf;
    BEGIN(STRING);
}

<STRING>\" {
    add_char('\0');
    cool_yylval.symbol = stringtable.add_string(string_buf);
    reset_str_buf();
    
    BEGIN(INITIAL);
    return STR_CONST;
}

<STRING>\0 {
    cool_yylval.error_msg = "String contains null character";
    BEGIN(STRING_ERR);
    return ERROR;
}

<STRING><<EOF>> {
    BEGIN(INITIAL);
    cool_yylval.error_msg = "EOF in string constant";
    return ERROR;
}

<STRING>\n {
    curr_lineno += 1;
    reset_str_buf();

    BEGIN(INITIAL);
    cool_yylval.error_msg = "Unterminated string constant";
    return ERROR;
}

<STRING>\\\n {
    curr_lineno += 1;
    add_char('\n');

    if (str_overflow()) {
        return strlen_error();
    }
}

<STRING>\\n {
    add_char('\n');

    if (str_overflow()) {
        return strlen_error();
    }
}

<STRING>\\t {
    add_char('\t');

    if (str_overflow()) {
        return strlen_error();
    }
}

<STRING>\\b {
    add_char('\b');

    if (str_overflow()) {
        return strlen_error();
    }
}

<STRING>\\f {
    add_char('\f');

    if (str_overflow()) {
        return strlen_error();
    }
}

<STRING>\\. {
    add_char(yytext[1]);

    if (str_overflow()) {
        return strlen_error();
    }
}

<STRING>. {
    add_char(yytext[0]);

    if (str_overflow()) {
        return strlen_error();
    }
}

<STRING_ERR>.*[\"\n] {
    BEGIN(INITIAL);
}

\n              { curr_lineno += 1; }
[\r\t\v\f ]     { }
. {
    cool_yylval.error_msg = yytext;
    return ERROR;
}

%%

bool str_overflow() {
    if ((unsigned)(string_buf_ptr - string_buf) >= MAX_STR_CONST) {
        BEGIN(STRING_ERR);
        return true;
    }
    return false;
}

int strlen_error() {
    reset_str_buf();
    cool_yylval.error_msg = "String constant too long";
    return ERROR;
}

void reset_str_buf() {
    string_buf_ptr = string_buf;
    string_buf[0] = '\0';
}

void add_char(char c) {
    *string_buf_ptr = c;
    ++string_buf_ptr;
}
