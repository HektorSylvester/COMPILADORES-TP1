/*
 * microc_mapa.c - Correspondencia com os tokens de Micro C (ver microc_mapa.h).
 *
 * A tabela de literais segue o tokens.h do livro-texto (secao 2.3.1):
 *
 *   UNDEF, ID, END_OF_FILE, INTEGERCONST, CHARCONST, STRINGCONST,
 *   PLUS, MINUS, MUL, DIV, MOD, EQ, NEQ, LT, GT, LEQ, GEQ, AND, OR, NOT,
 *   ASSIGN, SEMICOLON, COMMA, LPAREN, RPAREN, LBRACE, RBRACE,
 *   LBRACKET, RBRACKET, MAIN, IF, ELSE, FOR, RETURN, INT, CHAR, PRINT
 *
 * Se o seu tokens.h do TP1 usar outros nomes, ajuste a tabela.
 *
 * DONE(aluno): implementar microc_carregar_tokens_h.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "microc_mapa.h"
#include "util.h"

static const char *const literais[][2] = {
    { "+",  "PLUS" },      { "-",  "MINUS" },     { "*",  "MUL" },
    { "/",  "DIV" },       { "%",  "MOD" },
    { "==", "EQ" },        { "!=", "NEQ" },       { "<",  "LT" },
    { ">",  "GT" },        { "<=", "LEQ" },       { ">=", "GEQ" },
    { "&&", "AND" },       { "||", "OR" },        { "!",  "NOT" },
    { "=",  "ASSIGN" },    { ";",  "SEMICOLON" }, { ",",  "COMMA" },
    { "(",  "LPAREN" },    { ")",  "RPAREN" },    { "{",  "LBRACE" },
    { "}",  "RBRACE" },    { "[",  "LBRACKET" },  { "]",  "RBRACKET" },
    { "main", "MAIN" },    { "if", "IF" },        { "else", "ELSE" },
    { "for", "FOR" },      { "return", "RETURN" },{ "int", "INT" },
    { "char", "CHAR" },    { "print", "PRINT" }
};

static char **nomes = NULL;
static int nnomes = 0, capnomes = 0;

const char *microc_nome_literal(const char *literal)
{
    size_t i;
    for (i = 0; i < sizeof(literais) / sizeof(literais[0]); i++)
        if (strcmp(literais[i][0], literal) == 0)
            return literais[i][1];
    return NULL;
}

static void adicionar_nome(const char *s, size_t n)
{
    if (nnomes == capnomes) {
        capnomes = capnomes ? 2 * capnomes : 64;
        nomes = (char **) xrealloc(nomes, (size_t) capnomes * sizeof(char *));
    }
    nomes[nnomes++] = xstrndup(s, n);
}

/* DONE(aluno): abrir o arquivo (ler_arquivo ajuda), localizar a
 * enumeracao "typedef enum { ... } TokenType;" e chamar adicionar_nome
 * para cada identificador da lista, ignorando comentarios e valores
 * explicitos ("X = 3"). Retorne 0 em caso de sucesso.
 * Enquanto nao implementada, a verificacao dos nomes e pulada. */

int microc_carregar_tokens_h(const char *caminho)
{
    char *texto;
    char *inicio_enum;
    char *fim_enum;
    char *p;

    if (caminho == NULL)
        return -1;

    texto = ler_arquivo(caminho);
    if (texto == NULL) 
    {
        diag_aviso(0, 0, "nao foi possivel abrir %s", caminho);
        return -1;
    }


    inicio_enum = strstr(texto, "enum");

    if (inicio_enum != NULL) 
        inicio_enum = strchr(inicio_enum, '{');
    
    if (inicio_enum == NULL) 
    {
        free(texto);
        return -1;
    }

    inicio_enum++; /* pula '{' */

    fim_enum = strchr(inicio_enum, '}');

    if (fim_enum == NULL) 
    {
        free(texto);
        return -1;
    }

    p = inicio_enum;
    while (p < fim_enum)
     {
       
        while (p < fim_enum && (isspace((unsigned char)*p) || *p == ',')) 
        {
            p++;
        }

        /* Ignora comentarios */
        if (p < fim_enum - 1 && p[0] == '/' && p[1] == '*') 
        {
            p += 2;
            while (p < fim_enum - 1 && !(p[0] == '*' && p[1] == '/')) 
            {
                p++;
            }

            if (p < fim_enum - 1)
                p += 2;
            continue;
        }

        /* Ignora comentarios de linha */
        if (p < fim_enum - 1 && p[0] == '/' && p[1] == '/') 
        {
            while (p < fim_enum && *p != '\n') 
            {
                p++;
            }

            continue;
        }

        /* Se comeca com caractere de identificador*/
        if (p < fim_enum && (isalpha((unsigned char)*p) || *p == '_')) 
        {
            char *ini_id = p;
            while (p < fim_enum && (isalnum((unsigned char)*p) || *p == '_'))
            {
                p++;
            }
            adicionar_nome(ini_id, (size_t)(p - ini_id));

            /* Pula possivel atribuicao de valor numerico ex.: ID = 1 */
            while (p < fim_enum && isspace((unsigned char)*p)) 
            {
                p++;
            }

            if (p < fim_enum && *p == '=') 
            {
                p++;
                while (p < fim_enum && *p != ',' && *p != '}' && *p != '\n') 
                {
                    p++;
                }
            }
        } else 
        {
            p++;
        }
    }

    free(texto);

    return 0;
}

int microc_token_existe(const char *nome)
{
    int i;
    for (i = 0; i < nnomes; i++)
        if (strcmp(nomes[i], nome) == 0)
            return 1;
    return 0;
}

void microc_liberar(void)
{
    int i;
    for (i = 0; i < nnomes; i++)
        free(nomes[i]);
    free(nomes);
    nomes = NULL;
    nnomes = capnomes = 0;
}
