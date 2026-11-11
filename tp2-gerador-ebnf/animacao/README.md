# Animação da árvore de derivação (extensão opcional)

Esta extensão torna a construção da árvore mais didática: cada não terminal e
cada token consumido aparece como um nó, na ordem em que o analisador o
processa. As arestas são criadas junto com os filhos.

**O trabalho principal não depende desta pasta.** O gerador e os analisadores
continuam sendo ANSI C e funcionam sem Python, Manim ou qualquer dependência
adicional. A animação apenas consome a saída já existente de `--tree`.

## Requisitos

- Python 3.10 ou superior;
- Manim Community (`manim`);
- as dependências nativas exigidas pelo Manim para o sistema operacional;
- um analisador gerado com `--tree` e compilado.

No Windows, recomenda-se usar o mesmo ambiente Python para instalar e executar
o Manim. Em Linux/WSL e macOS, siga também as instruções oficiais do Manim
Community para instalar as bibliotecas nativas e o FFmpeg.

No macOS, instale as ferramentas de linha de comando do Xcode e use `clang`,
`python3` e o executável gerado sem extensão:

```sh
xcode-select --install
python3 -m pip install manim
```

Instalação do pacote Python:

```powershell
python -m pip install manim
```

Para conferir a instalação:

```powershell
python -c "import manim; print(manim.__version__)"
```

## Uso pela CLI

Execute a partir da raiz `tp2-gerador-ebnf`. Primeiro gere e compile um
analisador com suporte à árvore:

```powershell
.\gerador testes\gramaticas\expr.ebnf -o build\expr --tree
gcc -ansi -pedantic -Wall -Wextra -o build\expr.exe build\expr.c
```

Depois execute a extensão:

```powershell
python animacao\arvore_manim.py `
  --parser build\expr.exe `
  --entrada testes\programas\expr\aceito_1.txt `
  --saida build\animacao\expr `
  --qualidade media
```

Em Linux/WSL, os mesmos argumentos usam barras normais e o executável pode ser
`build/expr`:

```sh
python3 animacao/arvore_manim.py \
  --parser build/expr \
  --entrada testes/programas/expr/aceito_1.txt \
  --saida build/animacao/expr \
  --qualidade media
```

No macOS, use o mesmo formato do Linux/WSL:

```sh
python3 animacao/arvore_manim.py \
  --parser build/expr \
  --entrada testes/programas/expr/aceito_1.txt \
  --saida build/animacao/expr \
  --qualidade media
```

A CLI executa o analisador sem shell, exige que a entrada seja aceita e grava:

- `arvore.txt`: saída textual original de `--tree`;
- `arvore.gif`: animação da criação dos nós;
- `arvore.png`: estado final da árvore.

Ao finalizar, a CLI imprime o diretório absoluto e os três caminhos completos,
por exemplo:

```text
Diretório da animação: /Users/usuario/projeto/tp2-gerador-ebnf/build/animacao/expr
GIF: /Users/usuario/projeto/tp2-gerador-ebnf/build/animacao/expr/arvore.gif
PNG: /Users/usuario/projeto/tp2-gerador-ebnf/build/animacao/expr/arvore.png
Árvore textual: /Users/usuario/projeto/tp2-gerador-ebnf/build/animacao/expr/arvore.txt
```

No macOS, abra o diretório no Finder com:

```sh
open build/animacao/expr
```

O enquadramento da cena mantém proporção 16:9. A resolução em pixels depende de
`--qualidade` e do perfil de renderização do Manim.

`--qualidade` aceita `baixa`, `media` (padrão) e `alta`. Para uma demonstração
rápida, use `baixa`; para a apresentação, use `media` ou `alta`.

## Uso direto do Manim

Também é possível separar a captura da renderização. Execute o analisador e
salve sua saída em um arquivo UTF-8:

```powershell
.\build\expr.exe --tree testes\programas\expr\aceito_1.txt | `
  Out-File -Encoding utf8 build\animacao\expr\arvore.txt
$env:ARVORE_TXT = (Resolve-Path build\animacao\expr\arvore.txt).Path
manim -pqm --format gif animacao\arvore_manim.py ArvoreDerivacao
```

Em Linux/WSL:

```sh
./build/expr --tree testes/programas/expr/aceito_1.txt > build/animacao/expr/arvore.txt
ARVORE_TXT="$PWD/build/animacao/expr/arvore.txt" \
  manim -pqm --format gif animacao/arvore_manim.py ArvoreDerivacao
```

No macOS, o comando direto também usa a sintaxe POSIX:

```sh
./build/expr --tree testes/programas/expr/aceito_1.txt > build/animacao/expr/arvore.txt
ARVORE_TXT="$PWD/build/animacao/expr/arvore.txt" \
  manim -pqm --format gif animacao/arvore_manim.py ArvoreDerivacao
```

A variável `ARVORE_TXT` é necessária no uso direto porque o Manim carrega a
classe `ArvoreDerivacao` do módulo. O diretório de saída padrão do Manim é
`media/`; use `--media_dir build/animacao/manim` para escolher outro local.
Para salvar o último frame como PNG pelo Manim, use `-s` (ou
`--save_last_frame`) em uma segunda execução, sem `--format gif`.

Exemplo:

```sh
ARVORE_TXT="$PWD/build/animacao/expr/arvore.txt" \
  manim -pqm -s --media_dir build/animacao/manim \
  animacao/arvore_manim.py ArvoreDerivacao
```

## Testes da extensão

Os testes não precisam do Manim para validar o parsing da árvore:

```powershell
python -m unittest discover -s animacao\testes -v
```

No macOS/Linux:

```sh
python3 -m unittest discover -s animacao/testes -v
```

O teste de integração com o analisador `expr` usa `cc`, `gcc` ou outro
compilador definido em `CC`, quando disponível, e valida que a saída normal
continua sendo `entrada aceita`.

## Limitações

- A entrada precisa ser aceita pelo analisador; uma árvore parcial de erro não
  é apresentada como se fosse uma derivação válida.
- A saída deve ter sido produzida por um analisador gerado com `--tree`.
- A animação representa a árvore de derivação textual, não a árvore `Expr`
  interna da gramática EBNF nem o AFN das expressões regulares.
- Árvores muito grandes podem precisar de uma qualidade menor ou de ajustes de
  layout para continuarem legíveis.
