%option noyywrap
%x incl newl
%{
#define MAX_INCLUDE_DEPTH 10
YY_BUFFER_STATE include_stack[MAX_INCLUDE_DEPTH];
char fname[MAX_INCLUDE_DEPTH][128];
int line[MAX_INCLUDE_DEPTH];
int include_stack_ptr = 0;
%}

%%
\n[ \t\r]*$                { /* remove empty lines */ line[include_stack_ptr]++; }
\n                         { line[include_stack_ptr]++; return EOL; }
[ \t\r]                    ;
[0-9]+                     { yylval.val=atoi(yytext); return INTEGER; }
0[xX][0-9a-fA-F]+          { sscanf(&yytext[2],"%x",&yylval.val); return HEXINT; }
0[bB][01]+                 { {int i=2; yylval.val=0; while((yytext[i]=='0')||(yytext[i]=='1')) {  
                             yylval.val<<=1; if(yytext[i]=='1') yylval.val|=1; i++; } } return BININT; }
#define                    { return DEFINE; }
doc                        { return DOCUMENT; }
dbauthor                   { return AUTHOR; }
dbname                     { return DBNAME; }
dbdebug                    { return DBDEBUG; }
level                      { return LEVEL; }
tile                       { return TILE; }
solid                      { return TSOLID; }
path                       { return TPATH; }
\".*\"                     { strcpy(string_buffer,&yytext[1]); string_buffer[strlen(string_buffer)-1]=0; return STRING; } 
extern[^\n]*\n             { return EXTERN; }
[A-Za-z_][A-Za-z0-9_]*     { strncpy(yylval.name,yytext,STR_LEN); yylval.name[STR_LEN-1]=0; return IDENTIFIER; }
"<<"                       { return(LEFTSHIFT); }
">>"                       { return(RIGHTSHIFT); }
[+\-*/()|&~{}.]            { return(yytext[0]); }
\/\/[^\n]*\n               { line[include_stack_ptr]++; return EOL; }

#include[ \t]*       BEGIN(incl);
<incl>\"[^ \t\n]+\"        {
                             if ( include_stack_ptr >= MAX_INCLUDE_DEPTH ) {
                               yyerror( "Includes nested too deeply" );
			       exit( 1 );
			     }

			     include_stack[include_stack_ptr++] = YY_CURRENT_BUFFER;

			     yytext[strlen(yytext)-1]=0;
			     
			     strcpy(fname[include_stack_ptr],&yytext[1]);
			     line[include_stack_ptr]=1;

			     yyin = fopen( &yytext[1], "r" );
     
			     if ( ! yyin ) {
			       include_stack_ptr--;
			       yyerrorf(0,"error opening include file '%s'\n", &yytext[1]);
			       include_stack_ptr++;
			       exit( 1 );
			     }
     
			     yy_switch_to_buffer(yy_create_buffer( yyin, YY_BUF_SIZE ) );
			     
			     BEGIN(INITIAL);
                           }

<<EOF>>                    {
                             if ( --include_stack_ptr < 0 ) {
			       yyterminate();
			     } else {
			       yy_delete_buffer( YY_CURRENT_BUFFER );
			       yy_switch_to_buffer(include_stack[include_stack_ptr] );
			     }
                           }
.                          { yyerrorf(0,"illegal character '%c' in input", yytext[0]); };
%%










