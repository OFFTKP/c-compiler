D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			([Ee][+-]?{D}+)
P                       ([Pp][+-]?{D}+)
FS			(f|F|l|L)
IS                      ((u|U)|(u|U)?(l|L|ll|LL)|(l|L|ll|LL)(u|U))

%{
#include <stdio.h>
#define DEF(x, n) x = n,
enum {
	#include <token/tokens.def>
};
#undef DEF

void count(void);
%}

%%
"/*"			{ comment(); }
"//"[^\n]*              { /* consume //-comment */ }


"auto"			{ count(); return(Auto); }
"_Bool"			{ count(); return(Bool); }
"break"			{ count(); return(Break); }
"case"			{ count(); return(Case); }
"char"			{ count(); return(Char); }
"_Complex"		{ count(); return(Complex); }
"const"			{ count(); return(Const); }
"continue"		{ count(); return(Continue); }
"default"		{ count(); return(Default); }
"do"			{ count(); return(Do); }
"double"		{ count(); return(Double); }
"else"			{ count(); return(Else); }
"enum"			{ count(); return(Enum); }
"extern"		{ count(); return(Extern); }
"float"			{ count(); return(Float); }
"for"			{ count(); return(For); }
"goto"			{ count(); return(Goto); }
"if"			{ count(); return(If); }
"_Imaginary"		{ count(); return(Imaginary); }
"inline"		{ count(); return(Inline); }
"int"			{ count(); return(Int); }
"long"			{ count(); return(Long); }
"register"		{ count(); return(Register); }
"restrict"		{ count(); return(Restrict); }
"return"		{ count(); return(Return); }
"short"			{ count(); return(Short); }
"signed"		{ count(); return(Signed); }
"sizeof"		{ count(); return(Sizeof); }
"static"		{ count(); return(Static); }
"struct"		{ count(); return(Struct); }
"switch"		{ count(); return(Switch); }
"typedef"		{ count(); return(Typedef); }
"union"			{ count(); return(Union); }
"unsigned"		{ count(); return(Unsigned); }
"void"			{ count(); return(Void); }
"volatile"		{ count(); return(Volatile); }
"while"			{ count(); return(While); }

{L}({L}|{D})*		{ count(); return(check_type()); }

0[xX]{H}+{IS}?		{ count(); return(Constant); }
0[0-7]*{IS}?		{ count(); return(Constant); }
[1-9]{D}*{IS}?		{ count(); return(Constant); }
L?'(\\.|[^\\'\n])+'	{ count(); return(Constant); }

{D}+{E}{FS}?		{ count(); return(Constant); }
{D}*"."{D}+{E}?{FS}?	{ count(); return(Constant); }
{D}+"."{D}*{E}?{FS}?	{ count(); return(Constant); }
0[xX]{H}+{P}{FS}?	{ count(); return(Constant); }
0[xX]{H}*"."{H}+{P}{FS}?     { count(); return(Constant); }
0[xX]{H}+"."{H}*{P}{FS}?     { count(); return(Constant); }


L?\"(\\.|[^\\"\n])*\"	{ count(); return(StringLiteral); }

"..."			{ count(); return(Ellipsis); }
">>="			{ count(); return(RightAssign); }
"<<="			{ count(); return(LeftAssign); }
"+="			{ count(); return(AddAssign); }
"-="			{ count(); return(SubAssign); }
"*="			{ count(); return(MulAssign); }
"/="			{ count(); return(DivAssign); }
"%="			{ count(); return(ModAssign); }
"&="			{ count(); return(AndAssign); }
"^="			{ count(); return(XorAssign); }
"|="			{ count(); return(OrAssign); }
">>"			{ count(); return(RightOp); }
"<<"			{ count(); return(LeftOp); }
"++"			{ count(); return(IncOp); }
"--"			{ count(); return(DecOp); }
"->"			{ count(); return(PtrOp); }
"&&"			{ count(); return(AndOp); }
"||"			{ count(); return(OrOp); }
"<="			{ count(); return(LeOp); }
">="			{ count(); return(GeOp); }
"=="			{ count(); return(EqOp); }
"!="			{ count(); return(NeOp); }
";"			{ count(); return(Punctuator); }
("{"|"<%")		{ count(); return(Punctuator); }
("}"|"%>")		{ count(); return(Punctuator); }
","			{ count(); return(Punctuator); }
":"			{ count(); return(Punctuator); }
"="			{ count(); return(Punctuator); }
"("			{ count(); return(Punctuator); }
")"			{ count(); return(Punctuator); }
("["|"<:")		{ count(); return(Punctuator); }
("]"|":>")		{ count(); return(Punctuator); }
"."			{ count(); return(Punctuator); }
"&"			{ count(); return(Punctuator); }
"!"			{ count(); return(Punctuator); }
"~"			{ count(); return(Punctuator); }
"-"			{ count(); return(Punctuator); }
"+"			{ count(); return(Punctuator); }
"*"			{ count(); return(Punctuator); }
"/"			{ count(); return(Punctuator); }
"%"			{ count(); return(Punctuator); }
"<"			{ count(); return(Punctuator); }
">"			{ count(); return(Punctuator); }
"^"			{ count(); return(Punctuator); }
"|"			{ count(); return(Punctuator); }
"?"			{ count(); return(Punctuator); }

[ \t\v\n\f]		{ count(); }
.			{ count(); return(Empty); }

%%

int yywrap(void)
{
	return 1;
}


void comment(void)
{
	char c, prev = 0;
  
	while ((c = input()) != 0)      /* (EOF maps to 0) */
	{
		if (c == '/' && prev == '*')
			return;
		prev = c;
	}
	error("unterminated comment");
}


int column = 0;

void count(void)
{
	int i;

	for (i = 0; yytext[i] != '\0'; i++)
		if (yytext[i] == '\n')
			column = 0;
		else if (yytext[i] == '\t')
			column += 8 - (column % 8);
		else
			column++;
}


int check_type(void)
{
/*
* pseudo code --- this is what it should check
*
*	if (yytext == type_name)
*		return TYPE_NAME;
*
*	return IDENTIFIER;
*/

/*
*	it actually will only return IDENTIFIER
*/

	return Identifier;
}