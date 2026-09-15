## USO_IA . md

Ferramenta : Gemini Flash 3.8
Trecho : Constantes inteiras
Finalidade : Achei necessário criar uma variável global para ajudar na estratégia de verificação do lookahead MINUS, todavia seria ruim atualizar todas as funções para comportar essa nova variável (ainda mais tratando-se um trabalho versionado). Portanto, quis saber se havia uma forma mais direta de fazer isso. Além de entender como reverter caracteres lidos usando a macro 'yyless'.
O que fiz : Entendi que poderia criar uma função complementrar ao return nas seções 'Operadores' e 'ID': a função retorna_token, que atualiza a variável.


Ferramenta : Gemini Flash 3.8
Trecho : Constantes inteiras
Finalidade : Pedi sugestões de casos de teste para essa seçáo.
O que fiz : Adicionei os casos sugeridos ao meu arquivo de testes, executei o scanner e utilizei a saída para diagnosticar e corrigir um caso em que o operador 'MINUS' não estava sendo emitido após identificadores grudados ('b-5').

Ferramenta : Gemini Flash 3.8
Trecho : Constantes de caractere
Finalidade : Compreender funções nativas do Flex.
O que fiz : Pedi ao chat que me explicasse o funcionamento das rotinas, variáveis e convenções indicadas na Seção 4 e 5 do enunciado do trabalho. Compreendi as funcionalidades e às incorporei ao trabalho.

Ferramenta : Gemini Flash 3.8
Trecho : Constantes de caractere
Finalidade : Pedi sugestões de casos de teste para essa seção.
O que fiz : O chat me sugeriu um caso de teste que eu havia deixado passar: em vetores. Dito isso, adicionei os casos sugeridos ao arquivo e validei a emissão dos tokens CHARCONST e UNDEF.

Ferramenta : Gemini Flash 3.8
Trecho : test.mc
Finalidade : Pedi sugestões de casos de teste faltantes no texto.
O que fiz : Expliquei à ferramenta o objetivo do documentos test.mc e pedi que propusesse casos de teste ausentes, dessa forma descobri que não havia testado o EOF dentro de comentários e nem tratado o erro de 'loop' apresentado por ele, inspecionei a lógica do código e corrigi o erro.


Ferramenta : Gemini Flash 3.8
Trecho : Seção 4.2 (Tabela de Identificadores (String Table))
Finalidade : Por algum motivo eu achava que essa tabela seria usada ativamente nessa etapa do trabalho e isso estava dificultando a minha compreensão de como devia ser a implementação dela, portanto precisei de ajuda para entender essa parte do enunciado (basicamente eu estava confundindo 'palavras reservadas' e 'armazenamento de identificadores').
O que fiz : Passei a descrição da seção ao chat junto à minha dúvida e pedi a explicação teórica do funcionamento da tabela, compreendi o objetivo e implementei o código sozinha, ao final, pedi ao chat que conferisse erros lógicos de amarração da estrutura (sintaxe e boas práticas) e ele apenas me sugeriu uma verificação de 'checagem de falta de memória'.