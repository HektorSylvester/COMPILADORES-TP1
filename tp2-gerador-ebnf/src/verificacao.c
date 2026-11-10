/*
 * verificacao.c - Verificacoes semanticas da gramatica (etapa 2).
 *
 * Ja implementados (use como modelo):
 *   - verif_indefinidos: nao terminais usados mas nao definidos;
 *   - verif_terminais:   terminais nomeados sem declaracao.
 *
 * DONE(aluno):
 *   - verif_inalcancaveis      (aviso)
 *   - verif_improdutivos       (erro)
 *   - verif_recursao_esquerda  (erro; direta e indireta)
 *
 * A duplicidade de definicoes ja e detectada durante a leitura
 * (ebnf_parser.c).
 */
#include <stdlib.h>
#include <string.h>
#include "verificacao.h"
#include "microc_mapa.h"
#include "util.h"

static int verif_indefinidos(Gramatica *g)
{
    int i, erros = 0;
    for (i = 0; i < g->nnts; i++) {
        if (g->nts[i].regra == NULL) {
            diag_erro(g->nts[i].linha, g->nts[i].coluna,
                      "nao terminal %s usado mas nao definido", g->nts[i].nome);
            erros++;
        }
    }
    return erros;
}

static int verif_terminais(Gramatica *g, int modo_microc)
{
    int i, erros = 0, carregado = 0;
    Terminal *t;

    if (modo_microc)
        carregado = microc_carregar_tokens_h(g->tokens_externos) == 0;

    for (i = 1; i < g->nterms; i++) {
        t = &g->terms[i];
        if (modo_microc) {
            if (t->literal && microc_nome_literal(t->nome) == NULL) {
                diag_erro(t->linha, t->coluna, "literal %s nao corresponde a nenhum "
                          "token de Micro C", t->exibicao);
                erros++;
            } else if (!t->literal && carregado && !microc_token_existe(t->nome)) {
                diag_erro(t->linha, t->coluna, "terminal %s nao existe na enumeracao "
                          "TokenType de %s", t->nome, g->tokens_externos);
                erros++;
            }
        } else if (!t->declarado) {
            diag_erro(t->linha, t->coluna, "terminal %s nao declarado em TOKENS", t->nome);
            erros++;
        }
    }
    return erros;
}


/* DONE(aluno): emitir um AVISO para cada nao terminal que nao pode ser
 * alcancado a partir do simbolo inicial (g->inicial). Sugestao: marque
 * o inicial e, ate um ponto fixo, marque todo nao terminal que aparece
 * na regra de um nao terminal ja marcado (expr_percorrer ajuda). */

 
typedef struct {
    int *alcancavel;
    int mudou;
} CtxAlcancavel;

static void coletar_alcancaveis(Expr *e, void *ctx)
{
    CtxAlcancavel *c = (CtxAlcancavel *) ctx;
    if (e != NULL && e->tipo == E_NAOTERM) {
        if (!c->alcancavel[e->simbolo]) {
            c->alcancavel[e->simbolo] = 1;
            c->mudou = 1;
        }
    }
}

static int verif_inalcancaveis(Gramatica *g)
{
    int i, mudou;
    int *alcancavel;
    CtxAlcancavel ctx;

    if (g->inicial < 0 || g->nnts == 0)
        return 0;

    alcancavel = (int *) xcalloc((size_t) g->nnts, sizeof(int));
    alcancavel[g->inicial] = 1;


    do {
        mudou = 0;
        ctx.alcancavel = alcancavel;
        ctx.mudou = 0;

        for (i = 0; i < g->nnts; i++) 
            if (alcancavel[i] && g->nts[i].regra != NULL) 
                expr_percorrer(g->nts[i].regra, coletar_alcancaveis, &ctx);
            
        mudou = ctx.mudou;
    } while (mudou);


    for (i = 0; i < g->nnts; i++) 
        if (!alcancavel[i]) 
            diag_aviso(g->nts[i].linha, g->nts[i].coluna, "nao terminal %s inalcancavel a partir de %s", g->nts[i].nome, g->nts[g->inicial].nome);
        
    free(alcancavel);
    return 0;
}

/* DONE(aluno): emitir um ERRO para cada nao terminal definido que nao
 * deriva nenhuma cadeia de terminais. Ponto fixo: um nao terminal e
 * produtivo se sua regra for produtiva, onde terminais, E_VAZIO, [..] e
 * {..} sao produtivos, uma sequencia exige todos os filhos produtivos
 * e uma alternativa exige ao menos um. Retorne o numero de erros. */

static int expr_produtiva(const Expr *e, const int *produtivo)
{
    int i;
    if (e == NULL)
        return 1;

    switch (e->tipo) 
    {
        case E_TERM:
        case E_VAZIO:
        case E_OPC:
        case E_REP:
            return 1;

        case E_NAOTERM:
            return produtivo[e->simbolo];

        case E_SEQ: /* todos os filhos produtivos */
            for (i = 0; i < e->nfilhos; i++) 
                if (!expr_produtiva(e->filhos[i], produtivo))
                    return 0;
            
            return 1;

        case E_ALT: /* 1 ou mais filhos produtivos */
            for (i = 0; i < e->nfilhos; i++) 
                if (expr_produtiva(e->filhos[i], produtivo))
                    return 1;
            
            return 0;

        default:
            return 0;
    }
}

static int verif_improdutivos(Gramatica *g)
{
    int i, mudou, erros;
    int *produtivo;

    if (g->nnts == 0)
        return 0;

    produtivo = (int *) xcalloc((size_t) g->nnts, sizeof(int));
    erros = 0;

    do {
        mudou = 0;
        for (i = 0; i < g->nnts; i++) 
        {
            if (produtivo[i])
                continue;

            if (g->nts[i].regra == NULL)
                continue;

            if (expr_produtiva(g->nts[i].regra, produtivo)) 
            {
                produtivo[i] = 1;
                mudou = 1;
            }
        }
    } while (mudou);

    /* Emite erro para quem permaneceu improdutivo */
    for (i = 0; i < g->nnts; i++) 
    {
        if (produtivo[i])
            continue;

        if (g->nts[i].regra == NULL)
            continue;

        diag_erro(g->nts[i].linha, g->nts[i].coluna, "nao terminal %s nao deriva nenhuma cadeia de terminais", g->nts[i].nome);
        erros++;
    }

    free(produtivo);
    return erros;
}

/* DONE(aluno): detectar recursao a esquerda direta (a = a ...) e
 * indireta (a = b ... ; b = a ...), inclusive atraves de prefixos
 * anulaveis (a = [ x ] a ...). Use o campo anulavel de Expr, ja
 * calculado quando esta funcao e chamada. Reporte um erro por nao
 * terminal recursivo e retorne o numero de erros. */

 static void coletar_iniciais(const Expr *e, int *alcancados)
{
    int i;
    if (e == NULL)
        return;

    switch (e->tipo) 
    {
        case E_NAOTERM:
            alcancados[e->simbolo] = 1;
            break;

        case E_OPC:
        case E_REP:
            for (i = 0; i < e->nfilhos; i++)
                coletar_iniciais(e->filhos[i], alcancados);
            break;

        case E_ALT:
            for (i = 0; i < e->nfilhos; i++)
                coletar_iniciais(e->filhos[i], alcancados);
            break;

        case E_SEQ:
            for (i = 0; i < e->nfilhos; i++) {
                coletar_iniciais(e->filhos[i], alcancados);
                /* Se o filho nao pode ser anulado, o proximo filho não está no início */
                if (!e->filhos[i]->anulavel)
                    break;
            }
            break;

        default:
            break;
    }
}

static int alcanca(const Gramatica *g, int atual, int alvo, int *visitado)
{
    int i, alcancou = 0;
    int *primeiros;

    if (atual == alvo)
        return 1;
    if (visitado[atual] || g->nts[atual].regra == NULL)
        return 0;

    visitado[atual] = 1;
    primeiros = (int *) xcalloc((size_t) g->nnts, sizeof(int));
    coletar_iniciais(g->nts[atual].regra, primeiros);

    for (i = 0; i < g->nnts; i++) 
        if (primeiros[i] && alcanca(g, i, alvo, visitado)) {
            alcancou = 1;
            break;
        }
    

    free(primeiros);
    return alcancou;
}

int verif_recursao_esquerda(Gramatica *g)
{
    int i, j, erros, linha;
    int *primeiros;
    int *visitado;

    if (g->nnts == 0)
        return 0;

    erros = 0;
    primeiros = (int *) xcalloc((size_t) g->nnts, sizeof(int));
    visitado  = (int *) xcalloc((size_t) g->nnts, sizeof(int));

    for (i = 0; i < g->nnts; i++) 
    {
        if (g->nts[i].regra == NULL)
            continue;

        linha = g->nts[i].linha;
        if (g->nts[i].linha_def > 0) 
            linha = g->nts[i].linha_def;
        
        memset(primeiros, 0, (size_t) g->nnts * sizeof(int));
        coletar_iniciais(g->nts[i].regra, primeiros);

        if (primeiros[i]) 
        {
            diag_erro(linha, 0, "nao terminal %s tem recursao a esquerda direta", g->nts[i].nome);
            erros++;
            continue;
        }

        for (j = 0; j < g->nnts; j++) 
        {
            if (primeiros[j]) 
            {
                memset(visitado, 0, (size_t) g->nnts * sizeof(int));

                if (alcanca(g, j, i, visitado)) 
                {
                    diag_erro(linha, 0, "nao terminal %s tem recursao a esquerda indireta", g->nts[i].nome);
                    erros++;
                    break;
                }
            }
        }
    }

    free(primeiros);
    free(visitado);
    return erros;
}


int verif_basica(Gramatica *g, int modo_microc)
{
    int erros = 0;
    if (g->inicial < 0) {
        diag_erro(0, 0, "a gramatica nao tem nenhuma regra");
        return 1;
    }
    erros += verif_indefinidos(g);
    erros += verif_terminais(g, modo_microc);
    erros += verif_inalcancaveis(g);
    if (erros == 0)
        erros += verif_improdutivos(g);
    return erros;
}
