%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "tokens.h"


    
/* 2. SECAO DE DEFINICOES ------------------------------------- */

DIGIT       [0-9]
LETRA       [a-zA-Z_]
ALFANUM     [a-zA-Z0-9_]

%x COMMENT

%%


 /* -----------------------------------------------------------------------
  * 3. SECAO DE REGRAS
  * --------------------------------------------------------------------- */

 /* --- Fim de arquivo -----------------------------------------------------
  * Tratada explicitamente (em vez de depender do retorno automatico 0 do flex), pois o token UNDEF tambem vale 0 no enum TokenType -- se
  * dependessemos do comportamento padrao, um erro lexico seria confundido com o fim do arquivo pelo main() de teste abaixo. */

<INITIAL><<EOF>>             { return END_OF_FILE; }

 /* --- Espacos em branco e quebras de linha ---------------------------- */
\n                  { linha_atual++; }
[ \t\r]+            { /* ignora espacos em branco */ }




 /* --- Comentarios ------------------------------------------------------
  * Estes ja estao implementados como exemplo de uso de estados (%x) e de tratamento de erro via EOF dentro de um estado especial. */
"//".*              { /* comentario de linha: ignora ate o fim da linha */ }

"/*"                { BEGIN(COMMENT); }
<COMMENT>"*/"       { BEGIN(INITIAL); }
<COMMENT>\n         { linha_atual++; }  /*newline 0x0A*/
<COMMENT><<EOF>>    {
                        BEGIN(INITIAL);/*forçando o autômato para o começo, evitando o loop e continuando a leitura*/
                        microc_yylval.error_msg = "EOF em comentario";
                        return UNDEF;
                    }
<COMMENT>.          { /* consome qualquer outro caractere dentro do comentario */ }

 /* Fechamento de comentario sem abertura correspondente. */
"*/"                {
                        microc_yylval.error_msg = "Comentario nao iniciado";
                        return UNDEF;
                    }


 /* --- Palavras reservadas e identificadores ----------------------------*/

{LETRA}{ALFANUM}* {

                    TokenType tokenT = Checa_Palavra_Reservada(yytext);
                
                    if (tokenT == ID)
                    {
                        guarda_lexema();
                    }
                       
                    return tokenT;
                  }

 /* --- Constantes inteiras -----------------------------------------------*/

    "-"{DIGIT}+ { /*números negativos*/
        if (ultimo_token == ID || ultimo_token == INTEGERCONST || ultimo_token == RPAREN || ultimo_token == RBRACKET)
        {
            yyless(1); 
            return retorna_token(MINUS);
        } else {
            guarda_lexema();
            return retorna_token(INTEGERCONST);
        }
    }

    {DIGIT}+ { /*números positivos*/
        guarda_lexema();
        return retorna_token(INTEGERCONST);
    }

 /* --- Constantes de caractere --------------------------------------------*/


'([^\\'\n]|\\.)' { /* Valores aceitos: caractere comum (exceto \ e ') ou sequencia de escape (\.) */
    guarda_lexema();
    return retorna_token(CHARCONST);
}

'[^'\n]* { /*casos de erro*/
    microc_yylval.error_msg = "Constante de caractere nao terminada";
    return retorna_token(UNDEF);
}

 /* --- Constantes de string -----------------------------------------*/

\"([^"\\\n\0]|\\n|\\t|\\\\|\\\"|\\0)*\" {
        char *lexema_convertido = malloc(yyleng);
        size_t posicao = 0;

        for (int i = 1; i < yyleng - 1; i++)
        {
            if (yytext[i] != '\\')
            {
                lexema_convertido[posicao++] = yytext[i];
                continue;
            }

            i++;
            switch (yytext[i])
            {
                case 'n':  lexema_convertido[posicao++] = '\n'; break;
                case 't':  lexema_convertido[posicao++] = '\t'; break;
                case '\\': lexema_convertido[posicao++] = '\\'; break;
                case '"':  lexema_convertido[posicao++] = '"'; break;
                case '0':
                    free(lexema_convertido);
                    microc_yylval.error_msg = "String contem caractere nulo";
                    return retorna_token(UNDEF);
            }
        }

        lexema_convertido[posicao] = '\0';
        microc_yylval.symbol = inserir(lexema_convertido);
        free(lexema_convertido);
        return retorna_token(STRINGCONST);
    }

 /* --- Erros de string ----------------------------------------------------
  * Cada regra consome a string ate o ponto em que o erro foi encontrado. */

 /* Caractere nulo real dentro da string. */
\"([^"\\\n\0]|\\n|\\t|\\\\|\\\"|\\0)*\0 {
        microc_yylval.error_msg = "String contem caractere nulo";
        return retorna_token(UNDEF);
    }

 /* Quebra de linha sem a aspa de fechamento. */
\"([^"\\\n\0]|\\n|\\t|\\\\|\\\"|\\0)*\n {
        linha_atual++;
        microc_yylval.error_msg = "String nao terminada";
        return retorna_token(UNDEF);
    }

 /* Fim do arquivo antes da aspa de fechamento. */
\"([^"\\\n\0]|\\n|\\t|\\\\|\\\"|\\0)* {
        microc_yylval.error_msg = "EOF em string";
        return retorna_token(UNDEF);
    }



 /* --- Operadores relacionais e logicos -------------------------------- */
    "=="                { return retorna_token(EQ); }
    "="                 { return retorna_token(ASSIGN); }
    "!="                { return retorna_token(NEQ); }
    "!"                 { return retorna_token(NOT); }
    "<="                { return retorna_token(LEQ); }
    "<"                 { return retorna_token(LT); }
    ">="                { return retorna_token(GEQ); }
    ">"                 { return retorna_token(GT); }
    "&&"                { return retorna_token(AND); }
    "||"                { return retorna_token(OR); }

    /* --- Operadores aritmeticos e simbolos de pontuacao ------------------- */
    "+"                 { return retorna_token(PLUS); }
    "-"                 { return retorna_token(MINUS); }
    "*"                 { return retorna_token(MUL); }
    "/"                 { return retorna_token(DIV); }
    "%"                 { return retorna_token(MOD); }
    ";"                 { return retorna_token(SEMICOLON); }
    ","                 { return retorna_token(COMMA); }
    "("                 { return retorna_token(LPAREN); }
    ")"                 { return retorna_token(RPAREN); }
    "{"                 { return retorna_token(LBRACE); }
    "}"                 { return retorna_token(RBRACE); }
    "["                 { return retorna_token(LBRACKET); }
    "]"                 { return retorna_token(RBRACKET); }

 /* --- Caractere invalido -------------------------------------------------
  * Casa com qualquer caractere que nao tenha correspondido a nenhuma regra anterior. Deve ser SEMPRE a ultima regra do arquivo. */
.   {
        microc_yylval.error_msg = strdup(yytext);
        return retorna_token(UNDEF);
    }



%%

/* -----------------------------------------------------------------------
 * 4. SUB-ROTINAS DO USUARIO
 * ------------------------------------------------------------------- */

/* yywrap: informa ao flex que, ao atingir o EOF, a leitura deve simplesmente parar (nao ha um proximo arquivo a processar). */
int yywrap(void) 
{
    return 1;
}

/* main() de teste: le o arquivo passado como argumento e imprime, para cada token reconhecido, seu tipo, lexema e linha -- no mesmo espirito
 * do utilitario "lexer" mencionado no enunciado (Secao 6). Este main() e apenas uma ferramenta de depuracao para voce testar seu scanner de
 * forma isolada; ele NAO faz parte da interface formal entre o scanner e o parser (isso sera tratado nos trabalhos seguintes). */

int main(int argc, char **argv) 
{
    if (argc < 2) 
    {
        fprintf(stderr, "Uso: %s <arquivo.mc>\n", argv[0]);
        return 1;
    }

    FILE *arquivo_fonte = fopen(argv[1], "r");
    if (!arquivo_fonte) 
    {
        fprintf(stderr, "Erro: nao foi possivel abrir o arquivo '%s'\n", argv[1]);
        return 1;
    }
    yyin = arquivo_fonte;

    int tipo;
    while ((tipo = yylex()) != END_OF_FILE) 
    {
        if (tipo == UNDEF) 
        {
            fprintf(stderr, "ERRO LEXICO (linha %d): %s\n",
                    linha_atual, microc_yylval.error_msg);
            continue;
        }
        printf("Token: tipo = %-13s lexema = '%s'  linha = %d\n",
               nome_token[tipo], yytext, linha_atual);
    }

    fclose(arquivo_fonte);
    /*imprime_tabela();*/
    return 0;

}
