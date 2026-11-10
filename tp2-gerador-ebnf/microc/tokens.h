#ifndef TOKENS_H
#define TOKENS_H

/* 1. VOCABULARIO DE TOKENS */
typedef enum 
{
    UNDEF, ID, END_OF_FILE,
    INTEGERCONST, CHARCONST, STRINGCONST,
    PLUS, MINUS, MUL, DIV, MOD,
    EQ, NEQ, LT, GT, LEQ, GEQ, AND, OR, NOT,
    ASSIGN, SEMICOLON, COMMA, LPAREN, RPAREN,
    LBRACE, RBRACE, LBRACKET, RBRACKET,
    MAIN, IF, ELSE, FOR, RETURN, INT, CHAR, PRINT
} TokenType;

/* Valor semantico do token corrente */
typedef struct 
{
    char *symbol;
    char *error_msg;
} YYSTYPE;

/* Variaveis globais geradas pelo flex */
extern YYSTYPE microc_yylval;
extern int linha_atual;

/* Funcao de limpeza implementada no flex e chamada pelo scanner.c */
void liberar_tabela_ident(void);

#endif /* TOKENS_H */