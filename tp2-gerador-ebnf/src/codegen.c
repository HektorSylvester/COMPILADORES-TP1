/*
 * codegen.c - Geracao do analisador sintatico (etapa 5).
 *
 * Estrutura do arquivo .c gerado:
 *
 *   1. includes e conjuntos constantes (conj_0, conj_1, ...)
 *   2. estado e primitivas: avancar, match, erro_sintatico, arvore
 *   3. modo proprio: analisador lexico gerado a partir de TOKENS
 *      modo Micro C: tokens obtidos de proximo_token() (scanner do TP1)
 *   4. leitura de um arquivo de tokens (opcao --tokens do analisador)
 *   5. um procedimento parse_X() por nao terminal
 *   6. main
 *
 * Fornecido: toda a estrutura acima, os nomes dos tokens, os conjuntos,
 * as primitivas, o lexico (exceto o reconhecimento das expressoes
 * regulares) e a traducao de terminais e nao terminais.
 *
 * TODO(aluno):
 *   - gen_expr: traducao de sequencia, alternativa, [ ] e { }
 *     (esquema da secao 4 do enunciado);
 *   - gen_casamento_regex: converter uma expressao regular de TOKENS em
 *     codigo C (automato finito) que reconhece o maior prefixo;
 *   - gen_token_de_arquivo: leitura do arquivo de tokens do TP1.
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "microc_mapa.h"
#include "util.h"

#define TRANSICAO_EPSILON -1
#define AFN_EPSILON -1


/* ------------------------------------------------------------------ */
/* Emissao de texto                                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    FILE *f;
    int nivel;          /* nivel de indentacao (4 espacos por nivel) */
} Emissor;

/* Emite uma linha indentada; fmt segue printf. */
static void emit(Emissor *em, const char *fmt, ...)
{
    int i;
    va_list ap;
    for (i = 0; i < em->nivel; i++)
        fputs("    ", em->f);
    va_start(ap, fmt);
    vfprintf(em->f, fmt, ap);
    va_end(ap);
    fputc('\n', em->f);
}

/* Emite linhas fixas, sem interpretar '%'. A lista termina em NULL. */
static void emit_linhas(FILE *f, const char *const *linhas)
{
    for (; *linhas != NULL; linhas++) {
        fputs(*linhas, f);
        fputc('\n', f);
    }
}

/* Texto dentro de comentario C: evita fechar o comentario por engano. */
static void emitir_comentario(FILE *f, const char *s)
{
    for (; *s != '\0'; s++) {
        if (s[0] == '*' && s[1] == '/') {
            fputs("* /", f);
            s++;
        } else if (*s == '\n') {
            fputc(' ', f);
        } else {
            fputc(*s, f);
        }
    }
}

/* String C entre aspas, com escapes. */
static void emitir_string_c(FILE *f, const char *s)
{
    fputc('"', f);
    for (; *s != '\0'; s++) {
        unsigned char c = (unsigned char) *s;
        if (c == '"' || c == '\\')
            fprintf(f, "\\%c", c);
        else if (c == '\n')
            fputs("\\n", f);
        else if (c == '\t')
            fputs("\\t", f);
        else if (c < 32 || c > 126)
            fprintf(f, "\\%03o", c);
        else
            fputc(c, f);
    }
    fputc('"', f);
}

/* ------------------------------------------------------------------ */
/* Nomes dos tokens no codigo gerado                                  */
/* ------------------------------------------------------------------ */

static const char *const nomes_simbolos[][2] = {
    { "+", "MAIS" }, { "-", "MENOS" }, { "*", "VEZES" }, { "/", "BARRA" },
    { "%", "PORCENTO" }, { "(", "ABRE_PAR" }, { ")", "FECHA_PAR" },
    { "[", "ABRE_COL" }, { "]", "FECHA_COL" }, { "{", "ABRE_CHAVE" },
    { "}", "FECHA_CHAVE" }, { ";", "PV" }, { ",", "VIRGULA" },
    { ".", "PONTO" }, { ":", "DOIS_PONTOS" }, { "=", "IGUAL" },
    { "==", "IGUAL_IGUAL" }, { "!=", "DIFERENTE" }, { "<", "MENOR" },
    { ">", "MAIOR" }, { "<=", "MENOR_IGUAL" }, { ">=", "MAIOR_IGUAL" },
    { "&&", "E_LOGICO" }, { "||", "OU_LOGICO" }, { "!", "NAO" },
    { ":=", "ATRIBUICAO" }, { "->", "SETA" }
};

static int eh_palavra(const char *s)
{
    if (!isalpha((unsigned char) *s) && *s != '_')
        return 0;
    for (; *s != '\0'; s++)
        if (!isalnum((unsigned char) *s) && *s != '_')
            return 0;
    return 1;
}

static int nome_em_uso(const Gramatica *g, int ate, const char *nome)
{
    int i;
    for (i = 0; i < ate; i++)
        if (strcmp(g->terms[i].nome_c, nome) == 0)
            return 1;
    return 0;
}

/* Define g->terms[i].nome_c para todos os terminais. */
static void nomear_terminais(Gramatica *g, int microc)
{
    int i;
    size_t k;
    char buf[256];
    Terminal *t;

    for (i = 0; i < g->nterms; i++) {
        t = &g->terms[i];
        free(t->nome_c);
        if (i == TERM_FIM) {
            t->nome_c = xstrdup(microc ? "END_OF_FILE" : "T_FIM");
            continue;
        }
        if (microc) {
            t->nome_c = xstrdup(t->literal ? microc_nome_literal(t->nome) : t->nome);
            continue;
        }
        buf[0] = '\0';
        if (!t->literal) {
            sprintf(buf, "T_%.200s", t->nome);
        } else if (eh_palavra(t->nome) && strlen(t->nome) < 200) {
            sprintf(buf, "T_%s", t->nome);
            for (k = 2; buf[k] != '\0'; k++)
                buf[k] = (char) toupper((unsigned char) buf[k]);
        } else {
            for (k = 0; k < sizeof(nomes_simbolos) / sizeof(nomes_simbolos[0]); k++)
                if (strcmp(nomes_simbolos[k][0], t->nome) == 0)
                    sprintf(buf, "T_%s", nomes_simbolos[k][1]);
            if (buf[0] == '\0')
                sprintf(buf, "T_LIT_%d", i);
        }
        if (nome_em_uso(g, i, buf))
            sprintf(buf + strlen(buf), "_%d", i);
        t->nome_c = xstrdup(buf);
    }
}

/* ------------------------------------------------------------------ */
/* Conjuntos constantes                                               */
/* ------------------------------------------------------------------ */

#define MAX_COMPARACOES 3   /* ate 3 tokens: tok.tipo == A || ...; acima: pertence */

static Bitset *conjuntos = NULL;
static int nconj = 0, capconj = 0;

static int bitset_igual(const Bitset *a, const Bitset *b)
{
    int i;
    for (i = 0; i < a->n; i++)
        if (bitset_tem(a, i) != bitset_tem(b, i))
            return 0;
    return 1;
}

/* Registra um conjunto e devolve seu numero N (emitido como conj_N).
 * Conjuntos iguais compartilham a mesma constante. */
static int registrar_conjunto(const Bitset *b)
{
    int i;
    for (i = 0; i < nconj; i++)
        if (bitset_igual(&conjuntos[i], b))
            return i;
    if (nconj == capconj) {
        capconj = capconj ? 2 * capconj : 16;
        conjuntos = (Bitset *) xrealloc(conjuntos, (size_t) capconj * sizeof(Bitset));
    }
    bitset_iniciar(&conjuntos[nconj], b->n);
    bitset_copiar(&conjuntos[nconj], b);
    return nconj++;
}

static void liberar_conjuntos(void)
{
    int i;
    for (i = 0; i < nconj; i++)
        bitset_liberar(&conjuntos[i]);
    free(conjuntos);
    conjuntos = NULL;
    nconj = capconj = 0;
}

static void emitir_conjuntos(FILE *f, const Gramatica *g)
{
    int i, t;
    for (i = 0; i < nconj; i++) {
        fprintf(f, "static const int conj_%d[] = { ", i);
        for (t = bitset_proximo(&conjuntos[i], 0); t >= 0;
             t = bitset_proximo(&conjuntos[i], t + 1))
            fprintf(f, "%s, ", g->terms[t].nome_c);
        fputs("-1 };\n", f);
    }
}

/* Escreve a condicao "o token corrente pertence a b":
 *   tok.tipo == T_A                     (um token)
 *   tok.tipo == T_A || tok.tipo == T_B  (ate MAX_COMPARACOES)
 *   pertence(tok.tipo, conj_N)          (mais tokens) */
static void emitir_teste(FILE *f, const Gramatica *g, const Bitset *b)
{
    int t, n = bitset_tamanho(b), primeiro = 1;
    if (n == 0) {
        fputs("0", f);
    } else if (n <= MAX_COMPARACOES) {
        for (t = bitset_proximo(b, 0); t >= 0; t = bitset_proximo(b, t + 1)) {
            fprintf(f, "%stok.tipo == %s", primeiro ? "" : " || ", g->terms[t].nome_c);
            primeiro = 0;
        }
    } else {
        fprintf(f, "pertence(tok.tipo, conj_%d)", registrar_conjunto(b));
    }
}

/* Linha "<antes><teste><depois>", ex.: emit_teste(em, "while (", g, b, ") {") */
static void emit_teste(Emissor *em, const char *antes, const Gramatica *g,
                       const Bitset *b, const char *depois)
{
    int i;
    for (i = 0; i < em->nivel; i++)
        fputs("    ", em->f);
    fputs(antes, em->f);
    emitir_teste(em->f, g, b);
    fputs(depois, em->f);
    fputc('\n', em->f);
}

/* Linha "erro_sintatico(conj_N);" com N = conjunto dos tokens esperados. */
static void emit_erro(Emissor *em, const Bitset *esperados)
{
    emit(em, "erro_sintatico(conj_%d);", registrar_conjunto(esperados));
}

/* ------------------------------------------------------------------ */
/* Traducao das expressoes (esquema da secao 4 do enunciado)          */
/* ------------------------------------------------------------------ */
/*
     * DONE(aluno): traduzir as demais construcoes (secao 4):
     *
     *   E_SEQ   a1 a2 ... an   ->  gen_expr de cada filho, em ordem
     *   E_ALT   a1 | ... | an  ->  if (FIRST(a1)) {...} else if ... else erro
     *   E_OPC   [ a ]          ->  if (FIRST(a)) { ... }
     *   E_REP   { a }          ->  while (FIRST(a)) { ... }
     *   E_VAZIO                ->  nada
     *
     * Ferramentas disponiveis neste arquivo:
     *   emit(em, fmt, ...)                 uma linha indentada
     *   em->nivel++ / em->nivel--          indentacao dos blocos
     *   emit_teste(em, "if (", g, &x->first, ") {")
     *   emit_erro(em, &e->first)           erro_sintatico com os esperados
     * Os conjuntos FIRST de cada no foram calculados em conjuntos.c.
     * Uma alternativa anulavel nao tem teste: ela fica no else final,
     * no lugar do erro.
     */

static void gen_expr(Emissor *em, const Gramatica *g, const Expr *e)
{
    int i = 0;
    int idx_anulavel;
    int primeiro_teste;
    const char *prefixo;

    if (e == NULL)
        return;

    switch (e->tipo) 
    {
        case E_VAZIO:
            break;

        case E_TERM:
            emit(em, "match(%s);", g->terms[e->simbolo].nome_c);
            break;

        case E_NAOTERM:
            emit(em, "parse_%s();", g->nts[e->simbolo].nome);
            break;

        case E_SEQ:
            while (i < e->nfilhos) 
            {
                gen_expr(em, g, e->filhos[i]);
                i++;
            }
            break;

        case E_OPC:
            for (i = 0; i < e->nfilhos; i++) 
            {
                emit_teste(em, "if (", g, &e->filhos[i]->first, ") {");
                em->nivel++;

                gen_expr(em, g, e->filhos[i]);
                em->nivel--;
                emit(em, "}");
            }
            break;

        case E_REP:
            emit_teste(em, "while (", g, &e->filhos[0]->first, ") {");
            em->nivel++;

            gen_expr(em, g, e->filhos[0]);
            em->nivel--;
            emit(em, "}");
            break;

        case E_ALT:
            idx_anulavel = -1;
            primeiro_teste = 1;

            for (i = 0; i < e->nfilhos; i++) 
                if (e->filhos[i]->anulavel) 
                {
                    idx_anulavel = i;
                    break;
                }
            

            for (i = 0; i < e->nfilhos; i++) 
            {
                if (i == idx_anulavel)
                    continue;

                prefixo = primeiro_teste ? "if (" : "} else if (";
                emit_teste(em, prefixo, g, &e->filhos[i]->first, ") {");
                em->nivel++;

                gen_expr(em, g, e->filhos[i]);
                em->nivel--;
                primeiro_teste = 0;
            }

            emit(em, "} else {");
            em->nivel++;
            if (idx_anulavel >= 0) 
                gen_expr(em, g, e->filhos[idx_anulavel]);
            else 
                emit_erro(em, &e->first);
            
            em->nivel--;
            emit(em, "}");
            break;

        default:
            break;
    }
}

/* ------------------------------------------------------------------ */
/* Expressoes regulares de TOKENS (modo proprio)                      */
/* ------------------------------------------------------------------ */

/* Emite a funcao
 *     static size_t <nome>(const char *s)
 * que devolve o comprimento do MAIOR prefixo de s reconhecido pela
 * expressao regular, ou 0 se nenhum prefixo for reconhecido.
 * Retorna 0 se a expressao for valida (senao reporte com diag_erro). */

typedef struct {
    unsigned char bits[32]; /* 256 bits para cobrir todos os bytes (0 a 255) */
} ConjuntoChar;

static void conj_limpar(ConjuntoChar *c)
{
    memset(c->bits, 0, sizeof(c->bits));
}

static void conj_adicionar(ConjuntoChar *c, unsigned char ch)
{
    c->bits[ch / 8] |= (unsigned char)(1 << (ch % 8));
}

static int conj_contem(const ConjuntoChar *c, unsigned char ch)
{
    return (c->bits[ch / 8] & (1 << (ch % 8))) != 0;
}

static void conj_inverter(ConjuntoChar *c)
{
    int i;
    for (i = 0; i < 32; i++) 
        c->bits[i] = (unsigned char)~c->bits[i];
}

/* Le um caractere com suporte a escapes (\n, \t, \\, \/, etc.) */
static int ler_char_com_escape(const char *s, size_t *pos, unsigned char *saida)
{
    if (s[*pos] == '\0')
        return 0;

    if (s[*pos] == '\\') 
    {
        (*pos)++;
        switch (s[*pos])
        {
            case 'n':  
                *saida = '\n'; 
                break;
            case 't':  
                *saida = '\t'; 
                break;
            case 'r':  
                *saida = '\r'; 
                break;
            case '\\': 
                *saida = '\\'; 
                break;
            case '/':  
                *saida = '/';  
                    break;
            case '\0': 
                return 0; /* escape incompleto no fim da string */
            default:    
                *saida = (unsigned char)s[*pos]; 
                break;
        }

        (*pos)++;
        return 1;
    }

    *saida = (unsigned char)s[*pos];
    (*pos)++;
    return 1;
}

/* Analisa ¡[a-z], [^0-9]¡ e preenche o ConjuntoChar */
static int parse_classe_chars(const char *s, size_t *pos, ConjuntoChar *conj)
{
    int invertido = 0;
    unsigned char c1;
    unsigned char c2;
    unsigned char ch;

    conj_limpar(conj);
    (*pos)++; /* pula o '[' inicial */

    if (s[*pos] == '^') 
    {
        invertido = 1;
        (*pos)++;
    }

    /* Caso especial: ']' logo no comeco da classe e tratado como caractere literal */
    if (s[*pos] == ']') 
    {
        conj_adicionar(conj, ']');
        (*pos)++;
    }

    while (s[*pos] != '\0' && s[*pos] != ']') 
    {
        if (!ler_char_com_escape(s, pos, &c1))
            return 0;

        /* Verifica a-z ou 0-9 */
        if (s[*pos] == '-' && s[*pos + 1] != '\0' && s[*pos + 1] != ']') 
        {
            (*pos)++; /* pula o '-' */
            if (!ler_char_com_escape(s, pos, &c2))
                return 0;

            if (c1 > c2)
                return 0; /* algo como [z-a] e erro */

            for (ch = c1; ch <= c2; ch++) 
            {
                conj_adicionar(conj, ch);
                if (ch == 255) break; /* previne overflow de unsigned char */
            }
        } else 
        {
            conj_adicionar(conj, c1);
        }
    }

    if (s[*pos] != ']')
        return 0; /* Erro: colchete nunca foi fechado */

    (*pos)++; /* pula o ']' */

    if (invertido)
        conj_inverter(conj);

    return 1;
}

typedef enum 
{
    REGEX_CHARSET,   /* caractere ConjuntoChar */
    REGEX_CONCAT,    /* Sequência*/
    REGEX_UNIAO,     /* Alternativa */
    REGEX_ESTRELA,   /* Fechamento */
    REGEX_MAIS,      /* (1 ou mais) */
    REGEX_OPCIONAL   /* Opcional */
} TipoNoRegex;

typedef struct NoRegex 
{
    TipoNoRegex tipo;
    ConjuntoChar conjunto;
    struct NoRegex *esq;
    struct NoRegex *dir;
} NoRegex;

static NoRegex *criar_no_regex(TipoNoRegex tipo)
{
    NoRegex *no = (NoRegex *)malloc(sizeof(NoRegex));

    if (no == NULL)
        return NULL;

    no->tipo = tipo;
    conj_limpar(&no->conjunto);
    no->esq = NULL;
    no->dir = NULL;

    return no;
}

static void liberar_arvore_regex(NoRegex *no)
{
    if (no == NULL)
        return;

    liberar_arvore_regex(no->esq);
    liberar_arvore_regex(no->dir);

    free(no);
}

/* Declarações prévias para recursão */
static NoRegex *parse_regex_uniao(const char *s, size_t *pos);
static NoRegex *parse_regex_concat(const char *s, size_t *pos);
static NoRegex *parse_regex_sufixo(const char *s, size_t *pos);
static NoRegex *parse_regex_atomo(const char *s, size_t *pos);

/* Átomo */
static NoRegex *parse_regex_atomo(const char *s, size_t *pos)
{
    NoRegex *no;
    unsigned char ch;
    int i;

    if (s[*pos] == '\0')
        return NULL;

    /*exceto quebra de linha '\n' */
    if (s[*pos] == '.') 
    {
        (*pos)++;
        no = criar_no_regex(REGEX_CHARSET);

        if (no == NULL) return NULL;

        for (i = 0; i < 256; i++)
            if (i != '\n')
                conj_adicionar(&no->conjunto, (unsigned char)i);
        
        return no;
    }

    /* Classe de caracteres */
    if (s[*pos] == '[') 
    {
        no = criar_no_regex(REGEX_CHARSET);

        if (no == NULL) 
            return NULL;

        if (!parse_classe_chars(s, pos, &no->conjunto)) 
        {
            liberar_arvore_regex(no);
            return NULL;
        }

        return no;
    }

    /* Subexpressão entre parênteses*/
    if (s[*pos] == '(') 
    {
        (*pos)++; /*'(' */

        no = parse_regex_uniao(s, pos);

        if (no == NULL)
            return NULL;

        if (s[*pos] != ')') 
        {
            liberar_arvore_regex(no);
            return NULL; /* Erro: parêntese não fechado */
        }

        (*pos)++; /* ')' */

        return no;
    }

    /* Caracteres reservados não podem iniciar um átomo solto */
    if (s[*pos] == ')' || s[*pos] == '|' || s[*pos] == '*' || s[*pos] == '+' || s[*pos] == '?')
        return NULL;

    /* Caractere escapado */
    if (!ler_char_com_escape(s, pos, &ch))
        return NULL;

    no = criar_no_regex(REGEX_CHARSET);

    if (no == NULL) 
        return NULL;

    conj_adicionar(&no->conjunto, ch);

    return no;
}

/* Sufixos: *, + e ? */
static NoRegex *parse_regex_sufixo(const char *s, size_t *pos)
{
    NoRegex *no = parse_regex_atomo(s, pos);
    NoRegex *pai;

    if (no == NULL)
        return NULL;

    while (s[*pos] == '*' || s[*pos] == '+' || s[*pos] == '?') 
    {
        TipoNoRegex tipo_op;

        if (s[*pos] == '*')     
             tipo_op = REGEX_ESTRELA;
        else if (s[*pos] == '+') 
            tipo_op = REGEX_MAIS;
        else                     
            tipo_op = REGEX_OPCIONAL;

        (*pos)++;

        pai = criar_no_regex(tipo_op);

        if (pai == NULL) 
        {
            liberar_arvore_regex(no);
            return NULL;
        }

        pai->esq = no;
        no = pai;
    }

    return no;
}

/* Concatenação*/
static NoRegex *parse_regex_concat(const char *s, size_t *pos)
{
    NoRegex *esq = parse_regex_sufixo(s, pos);
    NoRegex *dir;
    NoRegex *no_concat;

    if (esq == NULL)
        return NULL;

    /* Enquanto houver caracteres que não fecham parêntese nem unem alternativas */
    while (s[*pos] != '\0' && s[*pos] != '|' && s[*pos] != ')') 
    {
        dir = parse_regex_sufixo(s, pos);
        if (dir == NULL) 
        {
            liberar_arvore_regex(esq);
            return NULL;
        }

        no_concat = criar_no_regex(REGEX_CONCAT);

        if (no_concat == NULL) 
        {
            liberar_arvore_regex(esq);
            liberar_arvore_regex(dir);
            return NULL;
        }

        no_concat->esq = esq;
        no_concat->dir = dir;
        esq = no_concat;
    }

    return esq;
}

/* União: menor precedência */
static NoRegex *parse_regex_uniao(const char *s, size_t *pos)
{
    NoRegex *esq = parse_regex_concat(s, pos);
    NoRegex *dir;
    NoRegex *no_uniao;

    if (esq == NULL)
        return NULL;

    while (s[*pos] == '|') 
    {
        (*pos)++; /* pula '|' */

        dir = parse_regex_concat(s, pos);

        if (dir == NULL) 
        {
            liberar_arvore_regex(esq);
            return NULL;
        }

        no_uniao = criar_no_regex(REGEX_UNIAO);

        if (no_uniao == NULL) 
        {
            liberar_arvore_regex(esq);
            liberar_arvore_regex(dir);
            return NULL;
        }

        no_uniao->esq = esq;
        no_uniao->dir = dir;
        esq = no_uniao;
    }

    return esq;
}


static NoRegex *compilar_arvore_regex(const char *regex)
{
    size_t pos = 0;
    NoRegex *raiz = parse_regex_uniao(regex, &pos);

    if (raiz == NULL || regex[pos] != '\0') 
    {
        liberar_arvore_regex(raiz);
        return NULL; /* Erro de sintaxe */
    }

    return raiz;
}


typedef struct 
{
    int destino;
    int rotulo;   /* AFN_EPSILON | 0 para transicao com ConjuntoChar */
    ConjuntoChar conjunto;
} TransicaoAFN;

typedef struct 
{
    TransicaoAFN transicoes[2];
    int num_transicoes;
} EstadoAFN;

typedef struct 
{
    EstadoAFN *estados;
    int total_estados;
    int capacidade;
} GrafoAFN;

typedef struct 
{
    int inicio;
    int fim;
} FragmentoAFN;

static int afn_novo_estado(GrafoAFN *afn)
{
    int id;
    if (afn->total_estados >= afn->capacidade) 
    {
        int nova_cap = (afn->capacidade == 0) ? 16 : afn->capacidade * 2;

        EstadoAFN *novo = (EstadoAFN *)realloc(afn->estados, (size_t)nova_cap * sizeof(EstadoAFN));
        
        if (novo == NULL)
            return -1;

        afn->estados = novo;
        afn->capacidade = nova_cap;
    }

    id = afn->total_estados++;
    afn->estados[id].num_transicoes = 0;

    return id;
}

static void afn_add_transicao(GrafoAFN *afn, int origem, int destino, int rotulo, const ConjuntoChar *conj)
{
    EstadoAFN *est = &afn->estados[origem];
    int idx = est->num_transicoes++;

    est->transicoes[idx].destino = destino;
    est->transicoes[idx].rotulo = rotulo;

    if (rotulo != AFN_EPSILON && conj != NULL) 
        est->transicoes[idx].conjunto = *conj;
    else 
        conj_limpar(&est->transicoes[idx].conjunto);
    
}


static FragmentoAFN thompson_construir(GrafoAFN *afn, const NoRegex *no)
{
    FragmentoAFN frag;
    FragmentoAFN esq;
    FragmentoAFN dir;

    frag.inicio = -1;
    frag.fim = -1;

    if (no == NULL)
        return frag;

    switch (no->tipo) 
    {
        case REGEX_CHARSET:
            frag.inicio = afn_novo_estado(afn);
            frag.fim = afn_novo_estado(afn);
            afn_add_transicao(afn, frag.inicio, frag.fim, 0, &no->conjunto);

            break;

        case REGEX_CONCAT:
            esq = thompson_construir(afn, no->esq);
            dir = thompson_construir(afn, no->dir);
            afn_add_transicao(afn, esq.fim, dir.inicio, AFN_EPSILON, NULL);
            frag.inicio = esq.inicio;
            frag.fim = dir.fim;

            break;

        case REGEX_UNIAO:
            esq = thompson_construir(afn, no->esq);
            dir = thompson_construir(afn, no->dir);
            frag.inicio = afn_novo_estado(afn);
            frag.fim = afn_novo_estado(afn);
            afn_add_transicao(afn, frag.inicio, esq.inicio, AFN_EPSILON, NULL);
            afn_add_transicao(afn, frag.inicio, dir.inicio, AFN_EPSILON, NULL);
            afn_add_transicao(afn, esq.fim, frag.fim, AFN_EPSILON, NULL);
            afn_add_transicao(afn, dir.fim, frag.fim, AFN_EPSILON, NULL);

            break;

        case REGEX_ESTRELA:
            esq = thompson_construir(afn, no->esq);
            frag.inicio = afn_novo_estado(afn);
            frag.fim = afn_novo_estado(afn);
            afn_add_transicao(afn, frag.inicio, esq.inicio, AFN_EPSILON, NULL);
            afn_add_transicao(afn, frag.inicio, frag.fim, AFN_EPSILON, NULL);
            afn_add_transicao(afn, esq.fim, esq.inicio, AFN_EPSILON, NULL);
            afn_add_transicao(afn, esq.fim, frag.fim, AFN_EPSILON, NULL);

            break;

        case REGEX_MAIS:
            esq = thompson_construir(afn, no->esq);
            frag.inicio = esq.inicio;
            frag.fim = afn_novo_estado(afn);
            afn_add_transicao(afn, esq.fim, esq.inicio, AFN_EPSILON, NULL);
            afn_add_transicao(afn, esq.fim, frag.fim, AFN_EPSILON, NULL);

            break;

        case REGEX_OPCIONAL:
            esq = thompson_construir(afn, no->esq);
            frag.inicio = afn_novo_estado(afn);
            frag.fim = afn_novo_estado(afn);
            afn_add_transicao(afn, frag.inicio, esq.inicio, AFN_EPSILON, NULL);
            afn_add_transicao(afn, frag.inicio, frag.fim, AFN_EPSILON, NULL);
            afn_add_transicao(afn, esq.fim, frag.fim, AFN_EPSILON, NULL);
            
            break;
    }
    return frag;
}

typedef struct 
{
    unsigned char *bits;
    int nbytes;
} ConjuntoEstadosAFN;

static void conjunto_afn_alocar(ConjuntoEstadosAFN *c, int total_estados)
{
    c->nbytes = (total_estados + 7) / 8;
    c->bits = (unsigned char *)calloc((size_t)c->nbytes, 1);
}

static void conjunto_afn_liberar(ConjuntoEstadosAFN *c)
{
    if (c->bits != NULL) 
    {
        free(c->bits);
        c->bits = NULL;
    }
}

static void conjunto_afn_adicionar(ConjuntoEstadosAFN *c, int estado)
{
    c->bits[estado / 8] |= (unsigned char)(1 << (estado % 8));
}

static int conjunto_afn_contem(const ConjuntoEstadosAFN *c, int estado)
{
    return (c->bits[estado / 8] & (1 << (estado % 8))) != 0;
}

static int conjunto_afn_igual(const ConjuntoEstadosAFN *a, const ConjuntoEstadosAFN *b)
{
    return memcmp(a->bits, b->bits, (size_t)a->nbytes) == 0;
}


static void afn_fecho_epsilon(const GrafoAFN *afn, ConjuntoEstadosAFN *conj)
{
    int *pilha;
    int topo = 0;
    int i;

    pilha = (int *)malloc((size_t)afn->total_estados * sizeof(int));
    if (pilha == NULL)
        return;

    for (i = 0; i < afn->total_estados; i++) 
        if (conjunto_afn_contem(conj, i)) 
            pilha[topo++] = i;

    while (topo > 0) 
    {
        int u = pilha[--topo];
        const EstadoAFN *est = &afn->estados[u];

        for (i = 0; i < est->num_transicoes; i++) 
            if (est->transicoes[i].rotulo == AFN_EPSILON) 
            {
                int v = est->transicoes[i].destino;

                if (!conjunto_afn_contem(conj, v)) 
                {
                    conjunto_afn_adicionar(conj, v);
                    pilha[topo++] = v;
                }
            }
    }

    free(pilha);
}

typedef struct 
{
    int transicoes[256];
    int eh_final;
} EstadoAFD;

typedef struct 
{
    EstadoAFD *estados;
    int total_estados;
    int capacidade;
} GrafoAFD;

static int afd_novo_estado(GrafoAFD *afd, int eh_final)
{
    int id;
    int b;

    if (afd->total_estados >= afd->capacidade) 
    {
        int nova_cap = (afd->capacidade == 0) ? 16 : afd->capacidade * 2;

        EstadoAFD *novo = (EstadoAFD *)realloc(afd->estados, (size_t)nova_cap * sizeof(EstadoAFD));

        if (novo == NULL)
            return -1;

        afd->estados = novo;
        afd->capacidade = nova_cap;
    }

    id = afd->total_estados++;
    afd->estados[id].eh_final = eh_final;

    for (b = 0; b < 256; b++) 
        afd->estados[id].transicoes[b] = -1;
    
    return id;
}

static GrafoAFD *determinizacao_subconjuntos(const GrafoAFN *afn, int inicio_afn, int fim_afn)
{
    GrafoAFD *afd;
    ConjuntoEstadosAFN *mapa_estados;
    int cap_mapa = 16;
    int total_d = 0;
    int i;
    int byte_ch;

    afd = (GrafoAFD *)malloc(sizeof(GrafoAFD));

    if (afd == NULL)
        return NULL;

    afd->estados = NULL;
    afd->total_estados = 0;
    afd->capacidade = 0;

    mapa_estados = (ConjuntoEstadosAFN *)malloc((size_t)cap_mapa * sizeof(ConjuntoEstadosAFN));
    
    if (mapa_estados == NULL)
    {
        free(afd);
        return NULL;
    }

    /* Estado inicial do AFD */
    conjunto_afn_alocar(&mapa_estados[0], afn->total_estados);
    conjunto_afn_adicionar(&mapa_estados[0], inicio_afn);
    afn_fecho_epsilon(afn, &mapa_estados[0]);

    afd_novo_estado(afd, conjunto_afn_contem(&mapa_estados[0], fim_afn));
    total_d = 1;

    /* Itera sobre cada estado do AFD criado */
    for (i = 0; i < total_d; i++) 
    {
        for (byte_ch = 0; byte_ch < 256; byte_ch++) 
        {
            ConjuntoEstadosAFN delta;
            int u;
            int tem_transicao = 0;
            int destino_id = -1;

            conjunto_afn_alocar(&delta, afn->total_estados);

            /* Calcula o conjunto alcancado pelo byte atual */
            for (u = 0; u < afn->total_estados; u++) 
                if (conjunto_afn_contem(&mapa_estados[i], u)) 
                {
                    const EstadoAFN *est = &afn->estados[u];
                    int t;

                    for (t = 0; t < est->num_transicoes; t++) 
                        if (est->transicoes[t].rotulo != AFN_EPSILON) 
                            if (conj_contem(&est->transicoes[t].conjunto, (unsigned char)byte_ch)) 
                            {
                                conjunto_afn_adicionar(&delta, est->transicoes[t].destino);
                                tem_transicao = 1;
                            }                   
                }
            

            if (!tem_transicao) 
            {
                conjunto_afn_liberar(&delta);
                continue;
            }

            /* Aplica o fecho-epsilon */
            afn_fecho_epsilon(afn, &delta);

            /* Verifica se esse subconjunto de estados ja foi descoberto */
            for (u = 0; u < total_d; u++) 
                if (conjunto_afn_igual(&delta, &mapa_estados[u])) {
                    destino_id = u;
                    break;
                }
            

            /* Se e um estado novo, regista*/
            if (destino_id == -1)
            {
                if (total_d >= cap_mapa)
                {
                    cap_mapa *= 2;
                    mapa_estados = (ConjuntoEstadosAFN *)realloc(mapa_estados, (size_t)cap_mapa * sizeof(ConjuntoEstadosAFN));
                }

                mapa_estados[total_d] = delta;
                destino_id = afd_novo_estado(afd, conjunto_afn_contem(&delta, fim_afn));

                total_d++;
            } else 
            {
                conjunto_afn_liberar(&delta);
            }

            afd->estados[i].transicoes[byte_ch] = destino_id;
        }
    }

    for (i = 0; i < total_d; i++) 
        conjunto_afn_liberar(&mapa_estados[i]);
    
    free(mapa_estados);

    return afd;
}


static void emitir_transicoes_estado(FILE *f, const EstadoAFD *est)
{
    int b = 0;
    int primeiro_ramo = 1;

    while (b < 256) 
    {
        int dest = est->transicoes[b];
        int inicio_faixa;
        int fim_faixa;

        if (dest == -1) 
        {
            b++;
            continue;
        }

        inicio_faixa = b;
        while (b + 1 < 256 && est->transicoes[b + 1] == dest) 
        {
            b++;
        }

        fim_faixa = b;

        if (primeiro_ramo) 
        {
            fprintf(f, "            if (");
            primeiro_ramo = 0;
        } else 
        {
            fprintf(f, "            else if (");
        }

        if (inicio_faixa == fim_faixa) 
        {
            if (inicio_faixa >= 32 && inicio_faixa <= 126 && inicio_faixa != '\'' && inicio_faixa != '\\') 
            {
                fprintf(f, "c == '%c'", inicio_faixa);
            } else 
            {
                fprintf(f, "c == %d", inicio_faixa);
            }
        } else 
        {
            if (inicio_faixa >= 32 && inicio_faixa <= 126 && fim_faixa >= 32 && fim_faixa <= 126 && inicio_faixa != '\'' && fim_faixa != '\'' && inicio_faixa != '\\' && fim_faixa != '\\') 
            {
                fprintf(f, "c >= '%c' && c <= '%c'", inicio_faixa, fim_faixa);
            } else 
            {
                fprintf(f, "c >= %d && c <= %d", inicio_faixa, fim_faixa);
            }
        }

        fprintf(f, ") {\n");
        fprintf(f, "                estado = %d;\n", dest);
        fprintf(f, "            }\n");

        b++;
    }

    if (!primeiro_ramo) 
    {
        fprintf(f, "            else {\n");
        fprintf(f, "                goto fim;\n");
        fprintf(f, "            }\n");
    } else 
    {
        /* Estado sem transicoes de saida */
        fprintf(f, "            goto fim;\n");
    }
}


static void afd_liberar(GrafoAFD *afd)
{
    if (afd != NULL) 
    {
        if (afd->estados != NULL) 
            free(afd->estados);
        
        free(afd);
    }
}

static void afn_liberar(GrafoAFN *afn)
{
    if (afn != NULL && afn->estados != NULL) 
        free(afn->estados);
    
}


static int gen_casamento_regex(FILE *f, const char *nome, const char *regex)
{
    /* DONE(aluno): converter a expressao regular em automato finito
    * deterministico (por exemplo: analisar a expressao, construir o AFN
    * de Thompson e determiniza-lo pela construcao de subconjuntos) e
    * emitir o automato como codigo C: um switch sobre o estado, com as
    * transicoes por faixas de caracteres, guardando a ultima posicao em
    * que o automato passou por um estado final. Subconjunto minimo:
    * concatenacao, |, *, +, ?, parenteses, classes [a-z] e [^...], o
    * ponto e escapes como \n, \t, \\ e \/. */


    NoRegex *raiz;
    GrafoAFN afn;
    FragmentoAFN frag;
    GrafoAFD *afd;
    int est;

    raiz = compilar_arvore_regex(regex);
    if (raiz == NULL) 
    {
        /* Regex sintaticamente invalida */
        return -1;
    }

    /* Construcao de Thompson */
    afn.estados = NULL;
    afn.total_estados = 0;
    afn.capacidade = 0;
    frag = thompson_construir(&afn, raiz);
    liberar_arvore_regex(raiz);

    if (frag.inicio == -1) 
    {
        afn_liberar(&afn);
        return -1;
    }

    /* AFN -> AFD) */
    afd = determinizacao_subconjuntos(&afn, frag.inicio, frag.fim);
    afn_liberar(&afn);

    if (afd == NULL) 
        return -1;

    fprintf(f, "static size_t %s(const char *s)\n", nome);
    fprintf(f, "{\n");
    fprintf(f, "    size_t pos = 0;\n");
    fprintf(f, "    size_t melhor = 0;\n");
    fprintf(f, "    int estado = 0;\n");
    fprintf(f, "    unsigned char c;\n\n");

    /* cadeia vazia aceita imediatamente se estado 0 for final */
    if (afd->estados[0].eh_final) 
        fprintf(f, "    melhor = 0;\n");
    

    fprintf(f, "    while (s[pos] != '\\0') {\n");
    fprintf(f, "        c = (unsigned char)s[pos];\n");
    fprintf(f, "        switch (estado) {\n");

    for (est = 0; est < afd->total_estados; est++) 
    {
        fprintf(f, "        case %d:\n", est);
        emitir_transicoes_estado(f, &afd->estados[est]);
        fprintf(f, "            break;\n");
    }

    fprintf(f, "        default:\n");
    fprintf(f, "            goto fim;\n");
    fprintf(f, "        }\n\n");

    fprintf(f, "        pos++;\n");

    /* Verificacao de aceitacao apos transitar */
    fprintf(f, "        switch (estado) {\n");

    for (est = 0; est < afd->total_estados; est++) 
        if (afd->estados[est].eh_final) 
            fprintf(f, "        case %d:\n", est);
        
    
    fprintf(f, "            melhor = pos;\n");
    fprintf(f, "            break;\n");
    fprintf(f, "        default:\n");
    fprintf(f, "            break;\n");
    fprintf(f, "        }\n");

    fprintf(f, "    }\n\n");
    fprintf(f, "fim:\n");
    fprintf(f, "    return melhor;\n");
    fprintf(f, "}\n\n");

    afd_liberar(afd);

    return 0;
}



/* ------------------------------------------------------------------ */
/* Leitura do arquivo de tokens (opcao --tokens do analisador gerado) */
/* ------------------------------------------------------------------ */

/* Emite "static Token token_de_arquivo(void)", que le o proximo token
 * do arquivo aberto em arquivo_tokens, no formato gravado pelo lexer do
 * TP1 (livro-texto, secao 3.3.2):
 *
 *   Token: tipo = <numero>, lexema = '<texto>', linha = <numero>
 *
 * Ao chegar ao fim do arquivo devolve um token TOKEN_FIM. */


static void gen_token_de_arquivo(FILE *f, int microc)
{
/* TODO(aluno): emitir a leitura real com fgets + sscanf (formato
 * acima). Linhas em branco sao ignoradas; uma linha malformada deve
 * ser reportada com o numero da linha do arquivo de tokens. */

    (void) microc;

    fprintf(f, "static Token token_de_arquivo(void)\n");
    fprintf(f, "{\n");
    fprintf(f, "    static int linha_arq = 0;\n");
    fprintf(f, "    char buf[1024];\n");
    fprintf(f, "    Token t;\n");
    fprintf(f, "    int tipo_lido;\n");
    fprintf(f, "    int linha_token;\n");
    fprintf(f, "    char lex[512];\n");
    fprintf(f, "\n");
    fprintf(f, "    memset(&t, 0, sizeof t);\n");
    fprintf(f, "\n");
    fprintf(f, "    while (fgets(buf, sizeof buf, arquivo_tokens) != NULL) {\n");
    fprintf(f, "        char *p = buf;\n");
    fprintf(f, "        linha_arq++;\n");
    fprintf(f, "\n");
    fprintf(f, "        while (*p == ' ' || *p == '\\t' || *p == '\\r' || *p == '\\n') {\n");
    fprintf(f, "            p++;\n");
    fprintf(f, "        }\n");
    fprintf(f, "        if (*p == '\\0') {\n");
    fprintf(f, "            continue;\n");
    fprintf(f, "        }\n");
    fprintf(f, "\n");
    fprintf(f, "        lex[0] = '\\0';\n");
    fprintf(f, "        if (sscanf(p, \"Token: tipo = %%d, lexema = '%%[^']', linha = %%d\",\n");
    fprintf(f, "                   &tipo_lido, lex, &linha_token) == 3) {\n");
    fprintf(f, "            t.tipo = tipo_lido;\n");
    fprintf(f, "            t.linha = (size_t)linha_token;\n");
    fprintf(f, "            t.tamanho = strlen(lex);\n");
    fprintf(f, "            t.lexema = (char *)malloc(t.tamanho + 1);\n");
    fprintf(f, "            if (t.lexema != NULL) {\n");
    fprintf(f, "                strcpy(t.lexema, lex);\n");
    fprintf(f, "            }\n");
    fprintf(f, "            return t;\n");
    fprintf(f, "        }\n");
    fprintf(f, "\n");
    fprintf(f, "        if (sscanf(p, \"Token: tipo = %%d, lexema = '', linha = %%d\",\n");
    fprintf(f, "                   &tipo_lido, &linha_token) == 2) {\n");
    fprintf(f, "            t.tipo = tipo_lido;\n");
    fprintf(f, "            t.linha = (size_t)linha_token;\n");
    fprintf(f, "            t.tamanho = 0;\n");
    fprintf(f, "            t.lexema = (char *)malloc(1);\n");
    fprintf(f, "            if (t.lexema != NULL) {\n");
    fprintf(f, "                t.lexema[0] = '\\0';\n");
    fprintf(f, "            }\n");
    fprintf(f, "            return t;\n");
    fprintf(f, "        }\n");
    fprintf(f, "\n");
    fprintf(f, "        fprintf(stderr, \"linha %%d do arquivo de tokens malformada: %%s\\n\",\n");
    fprintf(f, "                linha_arq, buf);\n");
    fprintf(f, "        exit(EXIT_FAILURE);\n");
    fprintf(f, "    }\n");
    fprintf(f, "\n");
    fprintf(f, "    t.tipo = TOKEN_FIM;\n");
    fprintf(f, "    t.linha = (size_t)linha_arq;\n");
    fprintf(f, "    t.lexema = \"\";\n");
    fprintf(f, "    t.tamanho = 0;\n");
    fprintf(f, "    return t;\n");
    fprintf(f, "}\n\n");
}

/* ------------------------------------------------------------------ */
/* Partes fixas do codigo gerado                                      */
/* ------------------------------------------------------------------ */

static const char *const rt_estado[] = {
    "/* ---- estado do analisador ---- */",
    "",
    "static Token tok;                  /* token corrente (lookahead) */",
    "static int imprimir_arvore = 0;    /* opcao --tree */",
    "static int nivel_arvore = 0;",
    "static FILE *arquivo_tokens = NULL; /* opcao --tokens */",
    "",
    "static Token token_de_arquivo(void);",
    "",
    NULL
};

static const char *const rt_primitivas[] = {
    "/* ---- primitivas ---- */",
    "",
    "int pertence(int tipo, const int *conjunto)",
    "{",
    "    for (; *conjunto != -1; conjunto++)",
    "        if (*conjunto == tipo)",
    "            return 1;",
    "    return 0;",
    "}",
    "",
    "/* Reporta o erro com os tokens esperados (lista terminada em -1) e encerra. */",
    "void erro_sintatico(const int *esperados)",
    "{",
    "    int i;",
    "    escrever_posicao();",
    "    if ((int) tok.tipo == TOKEN_FIM)",
    "        fprintf(stderr, \"encontrado fim de arquivo, \");",
    "    else",
    "        fprintf(stderr, \"encontrado '%s', \", tok.lexema);",
    "    if (esperados[0] != -1 && esperados[1] == -1) {",
    "        fprintf(stderr, \"esperado %s\\n\", nome_token(esperados[0]));",
    "    } else {",
    "        fprintf(stderr, \"esperado um de {\");",
    "        for (i = 0; esperados[i] != -1; i++)",
    "            fprintf(stderr, \"%s%s\", i > 0 ? \", \" : \"\", nome_token(esperados[i]));",
    "        fprintf(stderr, \"}\\n\");",
    "    }",
    "    exit(EXIT_FAILURE);",
    "}",
    "",
    "static void recuo(void)",
    "{",
    "    int i;",
    "    for (i = 0; i < nivel_arvore; i++)",
    "        fputs(\"  \", stdout);",
    "}",
    "",
    "void arvore_entra(const char *nao_terminal)",
    "{",
    "    if (imprimir_arvore) {",
    "        recuo();",
    "        printf(\"%s\\n\", nao_terminal);",
    "    }",
    "    nivel_arvore++;",
    "}",
    "",
    "void arvore_sai(void)",
    "{",
    "    nivel_arvore--;",
    "}",
    "",
    "/* Consome o token corrente se for do tipo esperado; senao, erro. */",
    "void match(int esperado)",
    "{",
    "    int lista[2];",
    "    if ((int) tok.tipo != esperado) {",
    "        lista[0] = esperado;",
    "        lista[1] = -1;",
    "        erro_sintatico(lista);",
    "    }",
    "    if (SUPORTE_ARVORE && imprimir_arvore) {",
    "        recuo();",
    "        if (nome_token(tok.tipo)[0] == '\\'')",
    "            printf(\"%s\\n\", nome_token(tok.tipo));",
    "        else",
    "            printf(\"%s '%s'\\n\", nome_token(tok.tipo), tok.lexema);",
    "    }",
    "    avancar();",
    "}",
    "",
    NULL
};

/* Lexico proprio: partes fixas antes e depois das funcoes de regex. */
static const char *const rt_lexico_estado[] = {
    "/* ---- analisador lexico gerado a partir de TOKENS ---- */",
    "",
    "static char *fonte = NULL;",
    "static size_t pos = 0;",
    "static int linha = 1, coluna = 1;",
    "",
    "static char *ler_tudo(FILE *f)",
    "{",
    "    size_t cap = 4096, n = 0, lidos;",
    "    char *buf = (char *) malloc(cap), *novo;",
    "    if (buf == NULL) {",
    "        fprintf(stderr, \"memoria insuficiente\\n\");",
    "        exit(EXIT_FAILURE);",
    "    }",
    "    while ((lidos = fread(buf + n, 1, cap - n - 1, f)) > 0) {",
    "        n += lidos;",
    "        if (cap - n - 1 == 0) {",
    "            cap *= 2;",
    "            novo = (char *) realloc(buf, cap);",
    "            if (novo == NULL) {",
    "                fprintf(stderr, \"memoria insuficiente\\n\");",
    "                exit(EXIT_FAILURE);",
    "            }",
    "            buf = novo;",
    "        }",
    "    }",
    "    buf[n] = '\\0';",
    "    return buf;",
    "}",
    "",
    "static void consumir(size_t n)",
    "{",
    "    for (; n > 0 && fonte[pos] != '\\0'; n--, pos++) {",
    "        if (fonte[pos] == '\\n') {",
    "            linha++;",
    "            coluna = 1;",
    "        } else {",
    "            coluna++;",
    "        }",
    "    }",
    "}",
    "",
    NULL
};

static void escrever_cabecalho_c(FILE *f, const OpcoesGeracao *op)
{
    fputs("/*\n * Analisador sintatico descendente recursivo gerado a partir de ", f);
    emitir_comentario(f, op->origem);
    fputs(".\n * Arquivo gerado automaticamente: nao edite.\n */\n", f);
}

/* ------------------------------------------------------------------ */
/* Arquivo .h                                                         */
/* ------------------------------------------------------------------ */

static int gerar_h(const Gramatica *g, const OpcoesGeracao *op)
{
    char *caminho = (char *) xmalloc(strlen(op->prefixo) + 3);
    FILE *f;
    int i;

    sprintf(caminho, "%s.h", op->prefixo);
    f = fopen(caminho, "w");
    if (f == NULL) {
        diag_erro(0, 0, "nao foi possivel criar %s", caminho);
        free(caminho);
        return -1;
    }
    escrever_cabecalho_c(f, op);
    fputs("#ifndef PARSER_GERADO_H\n#define PARSER_GERADO_H\n\n", f);
    if (op->microc) {
        fputs("#include \"scanner.h\"   /* scanner do TP1: Token, TokenType, proximo_token */\n\n", f);
        fputs("#define TOKEN_FIM END_OF_FILE\n\n", f);
    } else {
        fputs("typedef enum {\n", f);
        for (i = 0; i < g->nterms; i++) {
            fprintf(f, "    %s%s   /* ", g->terms[i].nome_c, i + 1 < g->nterms ? "," : "");
            emitir_comentario(f, g->terms[i].exibicao);
            fputs(" */\n", f);
        }
        fputs("} TipoToken;\n\n", f);
        fputs("typedef struct {\n    int tipo;\n    char lexema[100];\n"
              "    int linha;\n    int coluna;\n} Token;\n\n", f);
        fputs("#define TOKEN_FIM T_FIM\n\n", f);
    }
    fputs("const char *nome_token(int tipo);\n", f);
    fputs("int  pertence(int tipo, const int *conjunto);\n", f);
    fputs("void avancar(void);\n", f);
    fputs("void match(int esperado);\n", f);
    fputs("void erro_sintatico(const int *esperados);\n", f);
    fputs("void arvore_entra(const char *nao_terminal);\n", f);
    fputs("void arvore_sai(void);\n\n", f);
    for (i = 0; i < g->nnts; i++)
        if (g->nts[i].regra != NULL)
            fprintf(f, "void parse_%s(void);\n", g->nts[i].nome);
    fputs("\n#endif\n", f);
    fclose(f);
    free(caminho);
    return 0;
}

/* ------------------------------------------------------------------ */
/* Arquivo .c                                                         */
/* ------------------------------------------------------------------ */

static void gerar_nome_token(FILE *f, const Gramatica *g)
{
    int i;
    fputs("const char *nome_token(int tipo)\n{\n    switch (tipo) {\n", f);
    for (i = 0; i < g->nterms; i++) {
        fprintf(f, "    case %s: return ", g->terms[i].nome_c);
        emitir_string_c(f, i == TERM_FIM ? "fim de arquivo" : g->terms[i].exibicao);
        fputs(";\n", f);
    }
    fputs("    default: return \"token desconhecido\";\n    }\n}\n\n", f);
}

static void gerar_posicao_e_avancar(FILE *f, int microc)
{
    fputs("static void escrever_posicao(void)\n{\n", f);
    if (microc)
        fputs("    fprintf(stderr, \"linha %d: \", tok.linha);\n", f);
    else
        fputs("    fprintf(stderr, \"linha %d, coluna %d: \", tok.linha, tok.coluna);\n", f);
    fputs("}\n\n", f);

    fputs("/* Le o proximo token para tok. */\nvoid avancar(void)\n{\n", f);
    fputs("    if (arquivo_tokens != NULL)\n        tok = token_de_arquivo();\n    else\n", f);
    if (microc) {
        fputs("        tok = proximo_token();\n", f);
        fputs("    if (tok.tipo == UNDEF) {\n", f);
        fputs("        fprintf(stderr, \"linha %d: erro lexico: %s\\n\", tok.linha, tok.lexema);\n", f);
        fputs("        exit(EXIT_FAILURE);\n    }\n", f);
    } else {
        fputs("        tok = proximo_token_gerado();\n", f);
    }
    fputs("}\n\n", f);
}

static int gerar_lexico(FILE *f, const Gramatica *g)
{
    int i, nlit = 0, erros = 0;
    char nome[300];

    emit_linhas(f, rt_lexico_estado);

    for (i = 0; i < g->nskips; i++) {
        sprintf(nome, "casa_skip_%d", i);
        fputs("/* SKIP: ", f);
        emitir_comentario(f, g->skips[i]);
        fputs(" */\n", f);
        if (gen_casamento_regex(f, nome, g->skips[i]) != 0)
            erros++;
    }
    for (i = 1; i < g->nterms; i++) {
        if (g->terms[i].literal) {
            nlit++;
            continue;
        }
        sprintf(nome, "casa_%.250s", g->terms[i].nome_c);
        fprintf(f, "/* %s: ", g->terms[i].nome);
        emitir_comentario(f, g->terms[i].regex ? g->terms[i].regex : "");
        fputs(" */\n", f);
        if (gen_casamento_regex(f, nome, g->terms[i].regex ? g->terms[i].regex : "")
            != 0)
            erros++;
    }

    /* literais: comparacao direta, com prioridade em caso de empate */
    fputs("static const char *const texto_literais[] = {\n", f);
    for (i = 1; i < g->nterms; i++)
        if (g->terms[i].literal) {
            fputs("    ", f);
            emitir_string_c(f, g->terms[i].nome);
            fputs(",\n", f);
        }
    fputs("    NULL\n};\n\nstatic const int tipo_literais[] = {\n", f);
    for (i = 1; i < g->nterms; i++)
        if (g->terms[i].literal)
            fprintf(f, "    %s,\n", g->terms[i].nome_c);
    fputs("    -1\n};\n\n", f);

    fputs("/* Reconhece o maior prefixo da entrada; em caso de empate vence\n"
          " * o literal (palavras reservadas tem prioridade sobre ID). */\n", f);
    fputs("static Token proximo_token_gerado(void)\n{\n", f);
    fputs("    Token t;\n    size_t n, melhor;\n    int i, tipo, pulou;\n\n", f);
    fputs("    do {\n        pulou = 0;\n", f);
    for (i = 0; i < g->nskips; i++)
        fprintf(f, "        if ((n = casa_skip_%d(fonte + pos)) > 0) {\n"
                   "            consumir(n);\n            pulou = 1;\n        }\n", i);
    fputs("    } while (pulou);\n\n", f);
    fputs("    memset(&t, 0, sizeof t);\n", f);
    fputs("    t.linha = linha;\n    t.coluna = coluna;\n", f);
    fputs("    if (fonte[pos] == '\\0') {\n        t.tipo = T_FIM;\n        return t;\n    }\n", f);
    fputs("    melhor = 0;\n    tipo = -1;\n", f);
    fputs("    for (i = 0; texto_literais[i] != NULL; i++) {\n", f);
    fputs("        n = strlen(texto_literais[i]);\n", f);
    fputs("        if (n > melhor && strncmp(fonte + pos, texto_literais[i], n) == 0) {\n", f);
    fputs("            melhor = n;\n            tipo = tipo_literais[i];\n        }\n    }\n", f);
    for (i = 1; i < g->nterms; i++)
        if (!g->terms[i].literal)
            fprintf(f, "    if ((n = casa_%s(fonte + pos)) > melhor) {\n"
                       "        melhor = n;\n        tipo = %s;\n    }\n",
                    g->terms[i].nome_c, g->terms[i].nome_c);
    fputs("    if (melhor == 0) {\n", f);
    fputs("        fprintf(stderr, \"linha %d, coluna %d: erro lexico: caractere "
          "invalido '%c'\\n\",\n                linha, coluna, fonte[pos]);\n", f);
    fputs("        exit(EXIT_FAILURE);\n    }\n", f);
    fputs("    n = melhor < sizeof t.lexema ? melhor : sizeof t.lexema - 1;\n", f);
    fputs("    memcpy(t.lexema, fonte + pos, n);\n    t.lexema[n] = '\\0';\n", f);
    fputs("    t.tipo = tipo;\n    consumir(melhor);\n    return t;\n}\n\n", f);
    (void) nlit;
    return erros;
}

static void gerar_procedimentos(FILE *f, const Gramatica *g, const OpcoesGeracao *op)
{
    int i;
    Emissor em;
    em.f = f;
    em.nivel = 0;

    fputs("/* ---- procedimentos (um por nao terminal) ---- */\n\n", f);
    for (i = 0; i < g->nnts; i++) {
        if (g->nts[i].regra == NULL)
            continue;
        fprintf(f, "/* %s = ", g->nts[i].nome);
        {
            /* a regra em EBNF, sem "* /" */
            FILE *tmp = tmpfile();
            int c;
            if (tmp != NULL) {
                char buf[2] = { 0, 0 };
                expr_imprimir(tmp, g, g->nts[i].regra);
                rewind(tmp);
                while ((c = fgetc(tmp)) != EOF) {
                    buf[0] = (char) c;
                    emitir_comentario(f, buf);
                }
                fclose(tmp);
            }
        }
        fputs(" ; */\n", f);
        fprintf(f, "void parse_%s(void)\n{\n", g->nts[i].nome);
        em.nivel = 1;
        if (op->arvore)
            emit(&em, "arvore_entra(\"%s\");", g->nts[i].nome);
        gen_expr(&em, g, g->nts[i].regra);
        if (op->arvore)
            emit(&em, "arvore_sai();");
        fputs("}\n\n", f);
    }
}

static void gerar_main(FILE *f, const Gramatica *g, const OpcoesGeracao *op)
{
    fputs("/* ---- programa principal ---- */\n\n", f);
    fputs("int main(int argc, char **argv)\n{\n", f);
    fputs("    int i, fim[2];\n    const char *arquivo = NULL, *tokens = NULL;\n", f);
    fputs("    FILE *entrada = stdin;\n\n", f);
    fputs("    for (i = 1; i < argc; i++) {\n", f);
    fputs("        if (strcmp(argv[i], \"--tree\") == 0) {\n", f);
    fputs("            imprimir_arvore = 1;\n", f);
    fputs("        } else if (strcmp(argv[i], \"--tokens\") == 0 && i + 1 < argc) {\n", f);
    fputs("            tokens = argv[++i];\n", f);
    fputs("        } else if (argv[i][0] == '-' && argv[i][1] != '\\0') {\n", f);
    fputs("            fprintf(stderr, \"uso: %s [--tree] [--tokens arquivo.tok | "
          "programa]\\n\", argv[0]);\n", f);
    fputs("            return 2;\n        } else {\n            arquivo = argv[i];\n"
          "        }\n    }\n", f);
    fputs("    if (imprimir_arvore && !SUPORTE_ARVORE)\n", f);
    fputs("        fprintf(stderr, \"aviso: gere o analisador com --tree para "
          "imprimir a arvore\\n\");\n\n", f);
    fputs("    if (tokens != NULL) {\n", f);
    fputs("        arquivo_tokens = fopen(tokens, \"r\");\n", f);
    fputs("        if (arquivo_tokens == NULL) {\n", f);
    fputs("            fprintf(stderr, \"nao foi possivel abrir %s\\n\", tokens);\n", f);
    fputs("            return 2;\n        }\n    } else {\n", f);
    fputs("        if (arquivo != NULL && (entrada = fopen(arquivo, \"r\")) == NULL) {\n", f);
    fputs("            fprintf(stderr, \"nao foi possivel abrir %s\\n\", arquivo);\n", f);
    fputs("            return 2;\n        }\n", f);
    if (op->microc)
        fputs("        scanner_iniciar(entrada);\n", f);
    else
        fputs("        fonte = ler_tudo(entrada);\n", f);
    fputs("    }\n\n", f);
    fputs("    avancar();\n", f);
    fprintf(f, "    parse_%s();\n", g->nts[g->inicial].nome);
    fputs("    if (tok.tipo != TOKEN_FIM) {\n", f);
    fputs("        fim[0] = TOKEN_FIM;\n        fim[1] = -1;\n", f);
    fputs("        erro_sintatico(fim);\n    }\n", f);
    fputs("    printf(\"entrada aceita\\n\");\n\n", f);
    fputs("    if (arquivo_tokens != NULL) {\n        fclose(arquivo_tokens);\n", f);
    fputs("    } else {\n", f);
    if (op->microc)
        fputs("        scanner_finalizar();\n", f);
    else
        fputs("        free(fonte);\n", f);
    fputs("        if (entrada != stdin)\n            fclose(entrada);\n    }\n", f);
    fputs("    return 0;\n}\n", f);
}

static const char *nome_base(const char *caminho)
{
    const char *b = strrchr(caminho, '/');
    return b != NULL ? b + 1 : caminho;
}

static int gerar_c(const Gramatica *g, const OpcoesGeracao *op)
{
    char *caminho = (char *) xmalloc(strlen(op->prefixo) + 3);
    FILE *f, *corpo;
    int c, erros = 0;

    /* Os procedimentos sao gerados primeiro num arquivo temporario porque
     * registram os conjuntos (conj_N), que precisam vir antes no .c. */
    corpo = tmpfile();
    if (corpo == NULL) {
        diag_erro(0, 0, "nao foi possivel criar arquivo temporario");
        free(caminho);
        return -1;
    }
    gerar_procedimentos(corpo, g, op);

    sprintf(caminho, "%s.c", op->prefixo);
    f = fopen(caminho, "w");
    if (f == NULL) {
        diag_erro(0, 0, "nao foi possivel criar %s", caminho);
        fclose(corpo);
        free(caminho);
        return -1;
    }
    escrever_cabecalho_c(f, op);
    fputs("#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n", f);
    fprintf(f, "#include \"%s.h\"\n\n", nome_base(op->prefixo));
    fprintf(f, "#define SUPORTE_ARVORE %d\n\n", op->arvore ? 1 : 0);

    fputs("/* ---- conjuntos de tokens ---- */\n\n", f);
    emitir_conjuntos(f, g);
    fputc('\n', f);

    emit_linhas(f, rt_estado);
    if (!op->microc)
        erros += gerar_lexico(f, g);
    gerar_nome_token(f, g);
    gerar_posicao_e_avancar(f, op->microc);
    emit_linhas(f, rt_primitivas);
    gen_token_de_arquivo(f, op->microc);

    rewind(corpo);
    while ((c = fgetc(corpo)) != EOF)
        fputc(c, f);
    fclose(corpo);

    gerar_main(f, g, op);
    fclose(f);
    free(caminho);
    return erros == 0 ? 0 : -1;
}

int codegen_gerar(Gramatica *g, const OpcoesGeracao *op)
{
    int r;
    nomear_terminais(g, op->microc);
    r = gerar_h(g, op);
    if (r == 0)
        r = gerar_c(g, op);
    liberar_conjuntos();
    return r;
}
