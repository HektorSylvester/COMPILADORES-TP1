/*
 * conjuntos.c - Calculo de anulavel, FIRST e FOLLOW (etapa 3).
 *
 * Fornecido: conj_iniciar e conj_imprimir.
 *
 * DONE(aluno): conj_anulavel, conj_first e conj_follow. Os tres sao
 * calculados por iteracao ate ponto fixo: repita uma passada sobre todas
 * as regras enquanto algum conjunto (ou flag) mudar. bitset_unir devolve
 * 1 quando o conjunto de destino muda, o que ajuda a detectar o ponto fixo.
 */
#include <stdio.h>
#include <string.h>
#include "conjuntos.h"
#include "util.h"

/* ------------------------------------------------------------------ */
/* Fornecido                                                          */
/* ------------------------------------------------------------------ */

static void iniciar_expr(Expr *e, void *ctx)
{
    int n = *(int *) ctx;
    bitset_liberar(&e->first);
    bitset_liberar(&e->follow);
    bitset_iniciar(&e->first, n);
    bitset_iniciar(&e->follow, n);
    e->anulavel = 0;
}

void conj_iniciar(Gramatica *g)
{
    int i, n = g->nterms;
    for (i = 0; i < g->nnts; i++) {
        bitset_liberar(&g->nts[i].first);
        bitset_liberar(&g->nts[i].follow);
        bitset_iniciar(&g->nts[i].first, n);
        bitset_iniciar(&g->nts[i].follow, n);
        g->nts[i].anulavel = 0;
        expr_percorrer(g->nts[i].regra, iniciar_expr, &n);
    }
}

void conj_imprimir(FILE *f, const Gramatica *g)
{
    int i;
    size_t largura = strlen("NAO TERMINAL");
    for (i = 0; i < g->nnts; i++)
        if (strlen(g->nts[i].nome) > largura)
            largura = strlen(g->nts[i].nome);
    fprintf(f, "%-*s  ANULAVEL  FIRST / FOLLOW\n", (int) largura, "NAO TERMINAL");
    for (i = 0; i < g->nnts; i++) {
        fprintf(f, "%-*s  %-8s  FIRST  = ", (int) largura, g->nts[i].nome,
                g->nts[i].anulavel ? "sim" : "nao");
        gram_imprimir_conjunto(f, g, &g->nts[i].first);
        fprintf(f, "\n%-*s            FOLLOW = ", (int) largura, "");
        gram_imprimir_conjunto(f, g, &g->nts[i].follow);
        fputc('\n', f);
    }
}


/* DONE(aluno): calcular e->anulavel para cada no Expr e g->nts[i].anulavel.
 *   E_TERM: 0        E_VAZIO: 1        E_NAOTERM: anulavel do nao terminal
 *   E_SEQ: 1 se todos os filhos forem anulaveis
 *   E_ALT: 1 se algum filho for anulavel
 *   E_OPC, E_REP: 1 (calcule tambem o do filho)
 * Repita ate nada mudar. */

static int calcular_expr_anulavel(Expr *e, const Gramatica *g, int *mudou)
{
    int i, novo_val, todos_anulaveis, algum_anulavel;

    if (e == NULL)
        return 0;

    switch (e->tipo) 
    {
        case E_TERM:
            novo_val = 0;
            break;

        case E_VAZIO:
            novo_val = 1;
            break;

        case E_NAOTERM:
            novo_val = g->nts[e->simbolo].anulavel;
            break;

        case E_OPC:
        case E_REP:
            for (i = 0; i < e->nfilhos; i++)
                calcular_expr_anulavel(e->filhos[i], g, mudou);

            novo_val = 1;
            break;

        case E_SEQ:
        todos_anulaveis = 1;
        for (i = 0; i < e->nfilhos; i++) 
        {
            int filho_anulavel = calcular_expr_anulavel(e->filhos[i], g, mudou);
            if (filho_anulavel == 0) 
                todos_anulaveis = 0;
            
        }
        novo_val = todos_anulaveis;
        break;

        case E_ALT:
        algum_anulavel = 0;
        for (i = 0; i < e->nfilhos; i++) 
        {
            int filho_anulavel = calcular_expr_anulavel(e->filhos[i], g, mudou);

            if (filho_anulavel == 1) 
                algum_anulavel = 1;
            
        }
        novo_val = algum_anulavel;
        break;

        default:
            novo_val = 0;
            break;
    }

    if (e->anulavel != novo_val) 
    {
        e->anulavel = novo_val;
        *mudou = 1;
    }

    return novo_val;
}

void conj_anulavel(Gramatica *g)
{
    int i, mudou, val;

    if (g->nnts == 0)
        return;

    do {
        mudou = 0;

        for (i = 0; i < g->nnts; i++) 
        {
            if (g->nts[i].regra == NULL)
                continue;

            val = calcular_expr_anulavel(g->nts[i].regra, g, &mudou);

            if (g->nts[i].anulavel != val) 
            {
                g->nts[i].anulavel = val;
                mudou = 1;
            }
        }
    } while (mudou);
}

/* DONE(aluno): calcular e->first para cada no Expr e g->nts[i].first.
 *   E_TERM t: {t}     E_NAOTERM B: FIRST(B)     E_VAZIO: {}
 *   E_SEQ a1..an: FIRST(a1) U FIRST(a2) ... ate o primeiro ai nao anulavel
 *   E_ALT, E_OPC, E_REP: uniao dos FIRST dos filhos
 * Calcule o FIRST de TODOS os filhos de uma sequencia, mesmo os que
 * nao contribuem para o FIRST dela: o gerador de codigo usa-os. */


static void calcular_expr_first(Expr *e, Gramatica *g, int *mudou)
{
    int i;
    if (e == NULL)
        return;

    switch (e->tipo) 
    {
        case E_VAZIO:
            break;

        case E_TERM:
        if (!bitset_tem(&e->first, e->simbolo)) 
        {
            bitset_add(&e->first, e->simbolo);
            *mudou = 1;
        }
        break;

        case E_NAOTERM:
            if (bitset_unir(&e->first, &g->nts[e->simbolo].first)) 
                *mudou = 1;
            
            break;

        case E_ALT:
        case E_OPC:
        case E_REP:
            
            for (i = 0; i < e->nfilhos; i++) 
            {
                calcular_expr_first(e->filhos[i], g, mudou);
                
                if (bitset_unir(&e->first, &e->filhos[i]->first)) 
                    *mudou = 1;
            }
            break;

        case E_SEQ:
            /* Calcula o FIRST de todos os filhos*/
            for (i = 0; i < e->nfilhos; i++) 
                calcular_expr_first(e->filhos[i], g, mudou);
            

            /*Vai ate achar o primeiro nao-anulavel */
            for (i = 0; i < e->nfilhos; i++) 
            {
                if (bitset_unir(&e->first, &e->filhos[i]->first)) 
                    *mudou = 1;

                if (!e->filhos[i]->anulavel) 
                    break;
                
            }
            break;

        default:
            break;
    }
}

void conj_first(Gramatica *g)
{
    int i, mudou;

    if (g->nnts == 0)
        return;

    do {
        mudou = 0;

        for (i = 0; i < g->nnts; i++) 
        {
            if (g->nts[i].regra == NULL)
                continue;

            calcular_expr_first(g->nts[i].regra, g, &mudou);

            if (bitset_unir(&g->nts[i].first, &g->nts[i].regra->first)) 
                mudou = 1;
            
        }
    } while (mudou);
}

/* DONE(aluno): calcular e->follow (o que pode vir logo apos cada
 * construcao) e g->nts[i].follow. FOLLOW(inicial) contem $ (TERM_FIM).
 * Para cada regra A = alfa, faca follow(alfa) U= FOLLOW(A) e propague:
 *   E_SEQ a1..an:  follow(ai) U= FIRST(ai+1 ... an), mais follow(seq)
 *                  se ai+1 ... an for anulavel
 *   E_ALT, E_OPC:  follow(filho) U= follow(no)
 *   E_REP { a }:   follow(a) U= follow(no) U FIRST(a)
 *   E_NAOTERM B:   FOLLOW(B) U= follow(no)
 * Repita ate nada mudar. */


static void calcular_expr_follow(Expr *e, Gramatica *g, int *mudou)
{
    int i, j;
    if (e == NULL)
        return;

    switch (e->tipo) 
    {
        case E_NAOTERM:
            if (bitset_unir(&g->nts[e->simbolo].follow, &e->follow)) 
                *mudou = 1;
            
            break;

        case E_ALT:
        case E_OPC:
            for (i = 0; i < e->nfilhos; i++) 
            {
                if (bitset_unir(&e->filhos[i]->follow, &e->follow)) 
                    *mudou = 1;
                
                calcular_expr_follow(e->filhos[i], g, mudou);
            }
            break;


            for (i = 0; i < e->nfilhos; i++) 
            {
                if (bitset_unir(&e->filhos[i]->follow, &e->follow)) 
                    *mudou = 1;
                
                if (bitset_unir(&e->filhos[i]->follow, &e->filhos[i]->first)) 
                    *mudou = 1;
                
                calcular_expr_follow(e->filhos[i], g, mudou);
            }
            break;

        case E_SEQ:
            for (i = 0; i < e->nfilhos; i++) 
            {
                int sufixo_anulavel = 1;

                for (j = i + 1; j < e->nfilhos; j++) 
                {
                    if (bitset_unir(&e->filhos[i]->follow, &e->filhos[j]->first)) 
                        *mudou = 1;
                    
                    if (!e->filhos[j]->anulavel) 
                    {
                        sufixo_anulavel = 0;
                        break;
                    }
                }


                if (sufixo_anulavel) 
                    if (bitset_unir(&e->filhos[i]->follow, &e->follow)) 
                        *mudou = 1;

                calcular_expr_follow(e->filhos[i], g, mudou);
            }
            break;

        default:
            break;
    }
}

void conj_follow(Gramatica *g)
{
    int i, mudou;

    if (g->nnts == 0)
        return;

    /* FOLLOW(inicial) contem $ */
    if (g->inicial >= 0) 
        bitset_add(&g->nts[g->inicial].follow, TERM_FIM);


    do {
        mudou = 0;
        for (i = 0; i < g->nnts; i++)
         {
            if (g->nts[i].regra == NULL)
                continue;

            /* Para cada regra A = alfa, follow(alfa) U = FOLLOW(A) */
            if (bitset_unir(&g->nts[i].regra->follow, &g->nts[i].follow)) 
                mudou = 1;
            

            calcular_expr_follow(g->nts[i].regra, g, &mudou);
        }
    } while (mudou);
}