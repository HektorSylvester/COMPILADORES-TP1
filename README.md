aluno: <202319040622>
aluno: <202419050336>
uso_ia: <sim>

# Compiladores I — Trabalho Prático 2: Gerador de Analisadores Sintáticos Descendentes Recursivos

Código-base do gerador. Ele lê uma gramática em EBNF e gera, em ANSI C, um analisador sintático descendente recursivo para a linguagem descrita. Veja o enunciado para os requisitos completos.

As duas primeiras linhas deste arquivo seguem o formato do TP1: uma linha `aluno:` por integrante e a linha `uso_ia:` logo abaixo. Registre o uso de IA em `USO_IA.md`.

## Pré-requisitos e diretório de trabalho

Execute os comandos a partir de `tp2-gerador-ebnf/`:

```powershell
Set-Location .\tp2-gerador-ebnf
```

No Linux/WSL, use `cd tp2-gerador-ebnf`. O projeto precisa de GCC; os alvos
`make`, `test` e `microc` também precisam de GNU Make e de um shell POSIX.
No Windows, execute esses alvos pelo WSL ou por um ambiente equivalente. O
gerador e os analisadores gerados podem ser compilados diretamente com GCC no
PowerShell.

## Compilar e executar

O fluxo completo pelo Makefile é:

```sh
make                      # compila o gerador e, se disponível, o analisador Micro C
make test                 # executa os testes oficiais
make test-extras          # executa os testes dos recursos de pontuação extra
make microc               # gera e compila build/microc/parser_microc
make clean                # remove gerador e build/
```

Para gerar e executar uma gramática manualmente no PowerShell:

```powershell
.\gerador.exe testes\gramaticas\expr.ebnf -o build\expr
gcc -ansi -pedantic -Wall -Wextra -o build\expr.exe build\expr.c
.\build\expr.exe testes\programas\expr\aceito_1.txt
```

No Linux/WSL, use o executável sem `.exe` e barras normais:

```sh
./gerador testes/gramaticas/expr.ebnf -o build/expr
gcc -ansi -pedantic -Wall -Wextra -o build/expr build/expr.c
./build/expr testes/programas/expr/aceito_1.txt
```

O gerador aceita as opções `--scanner microc`, `--sets`, `--tree`, `--ast`,
`--recover`, `--auto-conflicts` e `--debug`:

```text
gerador gramatica.ebnf -o saida/parser [opções]
```

As opções `--tree`, `--ast` e `--recover` também podem ser passadas ao
analisador gerado. Para ler tokens já preparados, use
`parser --tokens tokens.txt`. Todo o código C deve compilar sem avisos com
`gcc -ansi -pedantic -Wall -Wextra`; a única exceção é o `lex.yy.c` gerado
pelo Flex.

## Estado atual

O gerador implementa as etapas de leitura da EBNF, verificação semântica,
cálculo de conjuntos, validação LL(1), geração do analisador e integração
opcional com o scanner de Micro C. Os quatro recursos de pontuação extra
estão descritos na seção correspondente abaixo.

O parser gerado continua sendo ANSI C e não depende de Python ou Manim. A
animação é uma extensão opcional que consome a saída textual de `--tree`.

## Integração com o scanner do TP1 (seção 3.1 do enunciado)

1. Copie `microc.flex`, `tokens.h` e os demais `.c`/`.h` do seu TP1, como a tabela de strings, para `microc/`.
2. No `microc.flex`, use `%option prefix="microc_yy" noyywrap` e devolva `END_OF_FILE` na regra `<<EOF>>`.
3. Ajuste `microc/scanner.c` ao seu TP1: lexema vindo da tabela de strings e mensagem de erro de `error_msg` no token UNDEF.
4. Rode `make microc` e `make test`.

A gramática `testes/gramaticas/microc.ebnf` é a do Apêndice B do livro-texto, fatorada para LL(1). O `else` pendente gera um aviso de conflito em `[ "else" comando ]`. O conflito é resolvido de forma gulosa: o `else` fica com o `if` mais próximo.

## Testes

`testes/executar.sh` compara a saída do gerador
(`testes/esperado/*.gerador`) e a dos analisadores gerados
(`testes/programas/<gramatica>/*.esperado`) com as saídas esperadas. Cada
caso termina com a linha `status <código de saída>`.

Execute os testes oficiais pelo Makefile:

```sh
make test
```

Para validar especificamente os quatro extras, use:

```powershell
python -m unittest testes\test_extras.py -v
```

Ou, em Linux/WSL:

```sh
python3 -m unittest testes/test_extras.py -v
```

Ao adicionar casos próprios, mantenha a convenção dos arquivos esperados e
inclua pelo menos uma entrada aceita e uma rejeitada para cada gramática
válida. O enunciado do trabalho recomenda pelo menos 5 gramáticas e 3
programas aceitos e 3 rejeitados por gramática.

Os programas de Micro C só rodam depois de `make microc`. O caso `rejeitado_lexico.mc` espera a mensagem "String nao terminada" do seu TP1.

## Extensão opcional: animação da árvore

A pasta `tp2-gerador-ebnf/animacao/` contém uma extensão didática em Python que
reutiliza a saída de `--tree` e usa Manim Community para animar a criação de
cada nó. Ela exporta um GIF da construção e um PNG da árvore final.

O parser principal continua independente de Python. Para instalar a dependência
opcional e executar um exemplo, a partir de `tp2-gerador-ebnf/`:

```powershell
python -m pip install manim
python animacao\arvore_manim.py --parser build\expr.exe --entrada testes\programas\expr\aceito_1.txt --saida build\animacao\expr
```

O analisador precisa ter sido gerado com `--tree` e compilado previamente. As
instruções completas, incluindo o comando direto `manim ... ArvoreDerivacao`,
estão em `tp2-gerador-ebnf/animacao/README.md`.

## Pontuação extra

Os quatro recursos abaixo são opcionais e não alteram o comportamento padrão
quando suas opções não são usadas.

| Recurso | Opção no gerador | Opção na execução | Resultado |
| --- | --- | --- | --- |
| Recuperação após erros | `--recover` | `--recover` | relata vários erros e continua até o fim |
| Árvore sintática abstrata | `--ast` | `--ast` | imprime a AST dos símbolos relevantes |
| Correção de conflitos simples | `--auto-conflicts` | não se aplica | fatora alternativas com o mesmo primeiro símbolo |
| EBNF descrita em EBNF | `ebnf.ebnf` | depende do parser gerado | valida a gramática da própria notação |

O fluxo geral é gerar o analisador com a opção correspondente, compilá-lo com
GCC e, quando indicado na tabela, repetir a opção ao executar o analisador.

### Continuar a análise após erros (+1,0)

Gere o analisador com `--recover`:

```powershell
.\gerador testes\gramaticas\expr.ebnf -o build\expr_recover --recover
gcc -ansi -pedantic -Wall -Wextra -o build\expr_recover.exe build\expr_recover.c
.\build\expr_recover.exe --recover entrada_com_erros.txt
```

O analisador usa recuperação por descarte de tokens, relata cada erro
encontrado e termina com código diferente de zero. Erros léxicos do scanner
próprio também são descartados para que a análise prossiga.

### Árvore sintática abstrata (AST, +1,0)

Gere com `--ast` e execute com `--ast`:

```powershell
.\gerador testes\gramaticas\expr.ebnf -o build\expr_ast --ast
gcc -ansi -pedantic -Wall -Wextra -o build\expr_ast.exe build\expr_ast.c
.\build\expr_ast.exe --ast testes\programas\expr\aceito_1.txt
```

A AST mantém os não terminais relevantes e os terminais nomeados com seus
lexemas. Literais de pontuação e agrupamentos da EBNF são omitidos, evitando
que símbolos auxiliares dominem a árvore.

### Correção automática de conflitos simples (+0,5)

`--auto-conflicts` aplica fatoração à esquerda quando alternativas compartilham
diretamente o mesmo primeiro terminal ou não terminal:

```powershell
.\gerador testes\gramaticas\conflito_ll1.ebnf -o build\conflito --auto-conflicts
```

Conflitos ambíguos que não têm essa forma continuam sendo rejeitados. Sem a
opção, a gramática de exemplo continua produzindo o diagnóstico LL(1)
original.

### EBNF descrita em EBNF (+0,5)

`testes\gramaticas\ebnf.ebnf` descreve formalmente a própria linguagem de
gramáticas aceita pelo gerador, incluindo `TOKENS`/`EXTERN`, `RULES`, regras,
alternativas, sequências, opcionais e repetições. Ela pode ser processada
normalmente pelo próprio gerador:

```powershell
.\gerador testes\gramaticas\ebnf.ebnf -o build\ebnf --ast --recover
```

Nesse exemplo, a própria gramática é lida pelo gerador e o parser resultante
pode ser compilado e executado como qualquer outro analisador. O uso de
`--ast` e `--recover` também exercita os dois recursos correspondentes no
mesmo fluxo.
