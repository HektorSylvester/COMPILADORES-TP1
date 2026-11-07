
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    extern char *yytext;
    /* ---------------------------------------------------------------------
    * 1. VOCABULARIO DE TOKENS 
    * ------------------------------------------------------------------- */

    typedef enum 
    {
        /* Tokens fundamentais */
        UNDEF,          /* token indefinido (usado para reportar erros) */
        ID,             /* identificador                                */
        END_OF_FILE,    /* fim de arquivo                                */

        /* Constantes literais */
        INTEGERCONST,
        CHARCONST,
        STRINGCONST,

        /* Operadores aritmeticos */
        PLUS, MINUS, MUL, DIV, MOD,

        /* Operadores relacionais e logicos */
        EQ, NEQ, LT, GT, LEQ, GEQ, AND, OR, NOT,

        /* Simbolos de atribuicao e pontuacao */
        ASSIGN, SEMICOLON, COMMA, LPAREN, RPAREN,
        LBRACE, RBRACE, LBRACKET, RBRACKET,

        /* Palavras reservadas */
        MAIN, IF, ELSE, FOR, RETURN, INT, CHAR, PRINT
    } TokenType;


    /* Nomes dos tokens, usados apenas pelo main() de teste abaixo para imprimir o tipo de cada token de forma legivel. Mantenha esta lista
    * na MESMA ORDEM do enum TokenType. */

    static const char *nome_token[] = 
    {
        "UNDEF", "ID", "END_OF_FILE",
        "INTEGERCONST", "CHARCONST", "STRINGCONST",
        "PLUS", "MINUS", "MUL", "DIV", "MOD",
        "EQ", "NEQ", "LT", "GT", "LEQ", "GEQ", "AND", "OR", "NOT",
        "ASSIGN", "SEMICOLON", "COMMA", "LPAREN", "RPAREN",
        "LBRACE", "RBRACE", "LBRACKET", "RBRACKET",
        "MAIN", "IF", "ELSE", "FOR", "RETURN", "INT", "CHAR", "PRINT"
    };

    /* Valor semantico do token corrente. */
    typedef struct 
    {
        char *symbol;      /* lexema para ID, INTEGERCONST, CHARCONST, STRINGCONST */
        char *error_msg;   /* mensagem de erro, usada apenas quando tipo == UNDEF   */
    } YYSTYPE;

    YYSTYPE microc_yylval;

    /* Linha atual do arquivo-fonte sendo processado. Deve ser incrementada toda vez que uma quebra de linha for consumida pelo scanner (seja em
    * codigo "normal", dentro de comentarios ou dentro de strings). */
    int linha_atual = 1;


    /* Variável de estado global*/
    static int ultimo_token = 0;

    /* Função auxiliar para registrar o histórico e retornar o token */
    static inline int retorna_token(int tok) 
    {
        ultimo_token = tok;
        return tok;
    }


    /*Tabela de Identificadores*/

    typedef struct identificador 
    {
        char *lexema;
        struct identificador *prox;
    } identificador;

    
    char* inserir(const char *texto)
    {
        identificador *tabela_ident = NULL;
        identificador *atual = tabela_ident;
    
        while(atual != NULL)
        {
            if (strcmp(atual->lexema, texto) == 0)
                return atual->lexema;
            
            atual = atual->prox;
        }

        identificador *novo = (identificador*)calloc(1, sizeof(identificador));
        
        if (!novo) 
        {
            fprintf(stderr, "Erro: falha de alocacao de memoria na tabela de simbolos.\n");
            exit(1);
        }
        
        novo->lexema = strdup(texto);
        novo->prox = tabela_ident;
        tabela_ident = novo;

        return novo->lexema; 
    }

    
    /* Funcao auxiliar para preencher microc_yylval.symbol com uma copia do texto reconhecido (yytext). Sinta-se livre para usar/adaptar. */
    static void guarda_lexema(void) 
    {
        microc_yylval.symbol = inserir(yytext);
    }

    TokenType Checa_Palavra_Reservada(const char *lexema) 
    {

        const char *palavras_R[] = {"main", "if", "else", "for", "return", "int", "char", "print"};
        
        /* Vetor com os enums correspondentes na mesma ordem */
        TokenType tokens_R[] = {MAIN, IF, ELSE, FOR, RETURN, INT, CHAR, PRINT};
        
        /* Percorre as 8 palavras reservadas */
        for(int i = 0; i < 8; i++) 
        {
            /* Compara o lexema lido com a palavra do vetor */
            if (strcmp(lexema, palavras_R[i]) == 0) 
            {
                /* Se achou, retorna o token correspondente */
                return tokens_R[i];
            }
        }
        /* Se terminou o loop e não achou, é um identificador comum */
        return retorna_token(ID);
    }

   
