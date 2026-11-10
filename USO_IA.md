## USO_IA.md

Modelo de entrada:

    Ferramenta: Gemini (Google)
    Trecho:     src/ebnf_parser.c 
    Finalidade: Auxílio na tradução de trechos de código enxuto em código C funcional, garantindo a sintaxe correta da API do projeto.
    O que fiz:  Adaptado (a lógica de agrupamento e laços foi discutida conceitualmente e adaptada para usar as funções nativas do esqueleto do gerador, pois eram muitos arquivos e eu me perdia em relação a quais funções nativas existiam e acabava complicando mais que o necessário).

    Ferramenta: Gemini (Google)
    Trecho:     verificacao.c 
    Finalidade: Auxílio na tradução de trechos de código enxuto em código C funcional.
    O que fiz:  Adaptado, alocação de memória não é um ponto forte meu, ainda cometo muitos erros.

    Ferramenta: Gemini (Google)
    Trecho:     verificacao.c 
    Finalidade: eu não sabia o que era o 'ponto fixo' em ' marque o inicial e, ate um ponto fixo'.
    O que fiz: Consultado, o chat me explicou a ideia de circularidade na busca.

    Ferramenta: Gemini (Google)
    Trecho:     bitset.h, conjuntos.c, ll1.c
    Finalidade: Compreender a estrutura e utilidade de BitSet para o cálculo de FIRST e FOLLOW.
    O que fiz:  Consultado, o chat me explicou a representação de conjuntos por mapas de bits (operações bitwise) e como utilizar funções como bitset_unir para gerenciar as iterações de ponto fixo. Além de me ajudar a entender as funções em bitset.h 

    Ferramenta: Gemini (Google)
    Trecho:     codegen.c 
    Finalidade: Auxílio na compreensão das funções e ferramentas disponibilizadas no arquivo (como utilizá-las corretamente). 
    O que fiz:  Consultado, o chat me explicou as funções que eu não tinha entendido.

    Ferramenta: Gemini (Google)
    Trecho:     codegen.c 
    Finalidade: Auxílio na tradução de trechos de código enxuto em código C funcional. 
    O que fiz:  Adaptado.

    Ferramenta: Gemini (Google)
    Trecho:     codegen.c 
    Finalidade: Sobre o AFN de Thompson: eu estava cometendo relevantes conceituais e o chat me ajudou a planejar a implementação da seção em 4 etapas (como sugerido pelo enunciado). 
    O que fiz: Consultado.

    Ferramenta: Gemini (Google)
    Trecho:     codegen.c 
    Finalidade: Precisei de ajuda (e quase que de uma intervenção divina kkk) na implementação do AFN (eu já cometi todos os erros de sintaxe imagináveis à esse ponto), além disso o chat me sugeriu funções auxiliares que poderiam "descomplicar" as funções iniciais que eu havia planejado. 
    O que fiz: Adaptado.

    Ferramenta: Gemini (Google)
    Trecho:     microc_mapa.c 
    Finalidade: Auxílio na compreensão das funções usadas nesse arquivo, mas que estavam em 'util.h'. 
    O que fiz:  Consultado.

    Ferramenta: Gemini (Google)
    Trecho:     microc_mapa.c 
    Finalidade: Ajuda na verificação de se deixei passar algum identificador da lista. 
    O que fiz:  Adaptado.

    Ferramenta: Gemini (Google)
    Trecho:     tokens.h 
    Finalidade: Estava conferindo se estavam corretas às alterações feitas após o TP1 (eu percebi que tinha deixado uma variável local e ela devia ser global) e o chat me sugeriu criar uma função para liberar a memória da tabela de identificadores (isso não tinha sido implementado no TP1). 
    O que fiz:  Adaptado.

    Ferramenta: Gemini (Google)
    Trecho:     scanner.c, scanner.h e tokens.h 
    Finalidade: Fiz passar em 15 de 17 testes. Precisei de ajuda com os 2 que faltaram.
    O que fiz:  Adaptado.

    Ferramenta: Gemini (Google)
    Trecho:     calc 
    Finalidade: Pedi a IA que gerasse mais uma gramática para testar ainda mais a ferramenta, incluí exemplos aceitos e rejeitados.
    O que fiz:  Completo.

    Ferramenta: Copilot SDK (VS Code)
    Trecho:     animacao/arvore_manim.py, animacao/testes/test_arvore_manim.py e animacao/README.md
    Finalidade: Implementar uma extensão opcional e didática que reconstrói a saída textual de --tree e anima a criação dos nós com Manim Community, exportando GIF e PNG sem alterar o parser em C.
    O que fiz:  Implementação assistida, com revisão do formato da árvore, testes de parsing/CLI e validação da renderização; documentação de instalação e execução incluída.