/*
 * ll1.c - Verificacao da condicao LL(1) (etapa 4).
 *
 * Condicoes (secao 3 do enunciado):
 *   1. Em a1 | a2 | ... | an, os FIRST das alternativas sao disjuntos dois
 *      a dois e no maximo uma alternativa e anulavel.
 *   2. Se uma alternativa ai e anulavel, FOLLOW(alternativa) e disjunto
 *      dos FIRST das demais.
 *   3. Em [ a ] e { a }, a nao e anulavel e FIRST(a) e disjunto do
 *      FOLLOW da construcao.
 *
 * Violacoes de 1, 2 e da primeira parte de 3 sao ERROS. A violacao de
 * FIRST(a) inter FOLLOW nao vazio em [ a ] ou { a } e reportada como
 * AVISO e resolvida de forma gulosa: o analisador gerado entra na
 * construcao sempre que o token estiver em FIRST(a). E o caso classico
 * do "else pendente":  if = "if" "(" expr ")" cmd [ "else" cmd ] ;
 * em que o else fica com o if mais proximo, como em C.
 *
 * Fornecido: reportar (formata as mensagens).
 * DONE(aluno): verificar_expr e ll1_verificar.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ll1.h"
#include "util.h"

typedef struct {
    Expr *prefixo;
    Expr *resto;
} Fatoracao;

/* Retorna a primeira construcao de uma alternativa quando ela e um simbolo
 * direto. A correcao automatica fica deliberadamente restrita a esse caso:
 * ela e previsivel e equivale a uma fatoracao a esquerda escrita pelo autor. */
static const Expr *primeiro_simbolo(const Expr *e)
{
    if (e == NULL)
        return NULL;
    if (e->tipo == E_SEQ) {
        if (e->nfilhos == 0)
            return NULL;
        e = e->filhos[0];
    }
    if (e->tipo == E_TERM || e->tipo == E_NAOTERM)
        return e;
    return NULL;
}

static int mesmo_primeiro(const Expr *a, const Expr *b)
{
    const Expr *pa = primeiro_simbolo(a);
    const Expr *pb = primeiro_simbolo(b);
    return pa != NULL && pb != NULL
        && pa->tipo == pb->tipo && pa->simbolo == pb->simbolo;
}

static Fatoracao separar_primeiro(Expr *e)
{
    Fatoracao resultado;
    Expr *resto;

    resultado.prefixo = e;
    resultado.resto = NULL;
    if (e->tipo != E_SEQ) {
        resultado.resto = expr_criar(E_VAZIO, e->linha, e->coluna);
        return resultado;
    }

    resultado.prefixo = e->filhos[0];
    if (e->nfilhos == 1) {
        resto = expr_criar(E_VAZIO, e->linha, e->coluna);
        free(e->filhos);
        free(e);
        resultado.resto = resto;
        return resultado;
    }

    memmove(e->filhos, e->filhos + 1,
            (size_t) (e->nfilhos - 1) * sizeof(Expr *));
    e->nfilhos--;
    resultado.resto = e;
    return resultado;
}

static int fatorar_alternativa(Expr *e, int primeiro, const int *grupo)
{
    Expr *alternativas;
    Expr *prefixo = NULL;
    Expr *combinada;
    Fatoracao separada;
    int i, write, linha, coluna;

    alternativas = expr_criar(E_ALT, e->filhos[primeiro]->linha,
                              e->filhos[primeiro]->coluna);
    for (i = 0; i < e->nfilhos; i++) {
        if (!grupo[i])
            continue;
        separada = separar_primeiro(e->filhos[i]);
        if (prefixo == NULL)
            prefixo = separada.prefixo;
        else
            expr_liberar(separada.prefixo);
        expr_adicionar(alternativas, separada.resto);
    }

    linha = prefixo->linha;
    coluna = prefixo->coluna;
    combinada = expr_criar(E_SEQ, linha, coluna);
    expr_adicionar(combinada, prefixo);
    expr_adicionar(combinada, alternativas);

    write = 0;
    for (i = 0; i < e->nfilhos; i++) {
        if (grupo[i]) {
            if (i == primeiro)
                e->filhos[write++] = combinada;
        } else {
            e->filhos[write++] = e->filhos[i];
        }
    }
    e->nfilhos = write;
    return 1;
}

static int fatorar_expr(Expr *e)
{
    int i, j, count, changed = 0;
    int *grupo;
    int tem_sufixo;
    const Expr *base;

    if (e == NULL)
        return 0;

    for (i = 0; i < e->nfilhos; i++)
        changed += fatorar_expr(e->filhos[i]);

    if (e->tipo != E_ALT)
        return changed;

    /* Reinicia a busca depois de cada fatoracao: a nova alternativa pode
     * ainda compartilhar o primeiro simbolo com outra alternativa. */
    for (;;) {
        grupo = (int *) xcalloc((size_t) e->nfilhos, sizeof(int));
        count = 0;
        tem_sufixo = 0;

        for (i = 0; i < e->nfilhos; i++) {
            base = primeiro_simbolo(e->filhos[i]);
            if (base == NULL)
                continue;
            if (count == 0) {
                for (j = i; j < e->nfilhos; j++)
                    if (mesmo_primeiro(e->filhos[i], e->filhos[j])) {
                        grupo[j] = 1;
                        count++;
                        if (e->filhos[j]->tipo == E_SEQ
                            && e->filhos[j]->nfilhos > 1)
                            tem_sufixo = 1;
                    }
            }
            if (count >= 2 && tem_sufixo)
                break;
            memset(grupo, 0, (size_t) e->nfilhos * sizeof(int));
            count = 0;
            tem_sufixo = 0;
        }

        if (count < 2 || !tem_sufixo) {
            free(grupo);
            break;
        }
        for (i = 0; i < e->nfilhos; i++)
            if (grupo[i])
                break;
        fatorar_alternativa(e, i, grupo);
        free(grupo);
        changed++;
    }
    return changed;
}

int ll1_corrigir_conflitos_simples(Gramatica *g)
{
    int i, corrigidos = 0;

    for (i = 0; i < g->nnts; i++)
        if (g->nts[i].regra != NULL)
            corrigidos += fatorar_expr(g->nts[i].regra);

    if (corrigidos > 0)
        diag_aviso(0, 0, "%d conflito(s) LL(1) simples corrigido(s) por "
                   "fatoracao automatica", corrigidos);
    return corrigidos;
}

/* Reporta um conflito na regra do nao terminal nt, na posicao de e.
 * comum (pode ser NULL) e o conjunto de tokens em conflito. */
static void reportar(const Gramatica *g, int nt, const Expr *e, int eh_aviso,
                     const Bitset *comum, const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "%s:%d:%d: %s: conflito LL(1) em %s: ", g->arquivo, e->linha,
            e->coluna, eh_aviso ? "aviso" : "erro", g->nts[nt].nome);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    if (comum != NULL) {
        fprintf(stderr, " ");
        gram_imprimir_conjunto(stderr, g, comum);
    }
    fputc('\n', stderr);
}

/* DONE(aluno): percorrer as regras e, para cada no E_ALT, E_OPC e E_REP,
 * verificar as condicoes do cabecalho. Use bitset_intersecao/bitset_vazio
 * e reporte com reportar(...), por exemplo:
 *
 *   reportar(g, nt, e, 0, &comum, "alternativas %d e %d comecam com", i+1, j+1);
 *
 * que imprime:
 *   expr.ebnf:7:10: erro: conflito LL(1) em stmt: alternativas 1 e 2 comecam com {ID}
 *
 * Retorne o numero de ERROS (avisos nao contam). */


static int verificar_expr(const Gramatica *g, int nt, const Expr *e)
{
    int i, j, erros = 0;
    Bitset inter;

    if (e == NULL)
        return 0;

    bitset_iniciar(&inter, g->nterms);

    switch (e->tipo) 
    {
        case E_ALT:
            /*FIRSTs disjuntos */
            for (i = 0; i < e->nfilhos; i++) 
                for (j = i + 1; j < e->nfilhos; j++) 
                {
                    bitset_intersecao(&inter, &e->filhos[i]->first, &e->filhos[j]->first);

                    if (!bitset_vazio(&inter)) 
                    {
                        reportar(g, nt, e, 0, &inter, "alternativas %d e %d comecam com", i + 1, j + 1);
                        erros++;
                    }
                }
            

            /* Alternativas anulaveis */
            {
                int qtd_anulaveis = 0;
                int idx_anulavel = -1;

                for (i = 0; i < e->nfilhos; i++) 
                    if (e->filhos[i]->anulavel)
                    {
                        qtd_anulaveis++;
                        idx_anulavel = i;
                    }
                

                if (qtd_anulaveis > 1) 
                {
                    reportar(g, nt, e, 0, NULL,
                            "mais de uma alternativa e anulavel");
                    erros++;
                } else if (qtd_anulaveis == 1) 
                {
                    /* Se ai e anulavel, FOLLOW(ALT) deve ser disjunto do FIRST das outras */
                    for (j = 0; j < e->nfilhos; j++) 
                    {
                        if (j == idx_anulavel)
                            continue;

                        bitset_intersecao(&inter, &e->follow, &e->filhos[j]->first);

                        if (!bitset_vazio(&inter)) 
                        {
                            reportar(g, nt, e, 0, &inter,
                                    "alternativa %d anulavel conflita com", idx_anulavel + 1);
                            erros++;
                        }
                    }
                }
            }
            break;

        case E_OPC:
        case E_REP:
            for (i = 0; i < e->nfilhos; i++) 
            {
                const Expr *filho = e->filhos[i];

                if (filho->anulavel) 
                {
                    reportar(g, nt, e, 0, NULL, "conteudo de %s e anulavel", e->tipo == E_OPC ? "[ ... ]" : "{ ... }");
                    erros++;
                }

                bitset_intersecao(&inter, &filho->first, &e->follow);

                if (!bitset_vazio(&inter)) 
                    reportar(g, nt, e, 1, &inter, "FIRST e FOLLOW de %s se intersectam (resolvido de forma gulosa) em", e->tipo == E_OPC ? "[ ]" : "{ }");
            }
            break;

        default:
            break;
    }

    bitset_liberar(&inter);


    for (i = 0; i < e->nfilhos; i++) 
        erros += verificar_expr(g, nt, e->filhos[i]);
    

    return erros;
}

int ll1_verificar(const Gramatica *g)
{
    int i, erros = 0;

    for (i = 0; i < g->nnts; i++) 
        if (g->nts[i].regra != NULL) 
            erros += verificar_expr(g, i, g->nts[i].regra);
        
    return erros;
}