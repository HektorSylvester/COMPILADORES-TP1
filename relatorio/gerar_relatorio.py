"""Gera um relatório PDF textual, pesquisável e com hiperlinks."""

from pathlib import Path
from xml.sax.saxutils import escape

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_JUSTIFY, TA_LEFT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import cm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (
    BaseDocTemplate,
    Frame,
    HRFlowable,
    PageBreak,
    PageTemplate,
    Paragraph,
    Spacer,
    Table,
    TableStyle,
)


OUTPUT = Path(__file__).with_name("Relatorio_TP2_Compiladores.pdf")
REPOSITORY = "https://github.com/HektorSylvester/COMPILADORES-TP1/tree/tp2-manim"
WINE = colors.HexColor("#7B1E3A")
DARK_WINE = colors.HexColor("#561428")
LIGHT_WINE = colors.HexColor("#F5E8ED")
INK = colors.HexColor("#252525")
MUTED = colors.HexColor("#696969")


def register_fonts() -> None:
    candidates = [
        Path("C:/Windows/Fonts"),
        Path("/usr/share/fonts/truetype/dejavu"),
        Path("/usr/share/fonts/dejavu"),
        Path("/System/Library/Fonts/Supplemental"),
        Path("/Library/Fonts"),
    ]
    regulars = ["DejaVuSans.ttf", "Arial.ttf", "arial.ttf"]
    bolds = ["DejaVuSans-Bold.ttf", "Arial Bold.ttf", "arialbd.ttf"]
    italics = ["DejaVuSans-Oblique.ttf", "Arial Italic.ttf", "ariali.ttf"]
    for directory in candidates:
        regular = next((directory / name for name in regulars if (directory / name).exists()), None)
        bold = next((directory / name for name in bolds if (directory / name).exists()), None)
        italic = next((directory / name for name in italics if (directory / name).exists()), None)
        if regular and bold and italic:
            pdfmetrics.registerFont(TTFont("ReportRegular", str(regular)))
            pdfmetrics.registerFont(TTFont("ReportBold", str(bold)))
            pdfmetrics.registerFont(TTFont("ReportItalic", str(italic)))
            pdfmetrics.registerFontFamily(
                "Report", normal="ReportRegular", bold="ReportBold", italic="ReportItalic"
            )
            return
    raise RuntimeError("Nenhuma família Unicode completa foi encontrada para gerar o PDF")


register_fonts()
styles = getSampleStyleSheet()
styles.add(ParagraphStyle(
    name="ReportTitle", parent=styles["Title"], fontName="ReportBold",
    fontSize=24, leading=30, textColor=WINE, alignment=TA_CENTER,
    spaceAfter=14,
))
styles.add(ParagraphStyle(
    name="ReportSubtitle", parent=styles["Normal"], fontName="ReportRegular",
    fontSize=12, leading=18, textColor=DARK_WINE, alignment=TA_CENTER,
    spaceAfter=10,
))
styles.add(ParagraphStyle(
    name="ReportAuthor", parent=styles["Normal"], fontName="ReportBold",
    fontSize=12, leading=19, textColor=INK, alignment=TA_CENTER,
))
styles.add(ParagraphStyle(
    name="ReportHeading1", parent=styles["Heading1"], fontName="ReportBold",
    fontSize=16, leading=21, textColor=WINE, spaceBefore=20, spaceAfter=10,
    keepWithNext=True,
))
styles.add(ParagraphStyle(
    name="ReportHeading2", parent=styles["Heading2"], fontName="ReportBold",
    fontSize=12.5, leading=17, textColor=DARK_WINE, spaceBefore=13, spaceAfter=7,
    keepWithNext=True,
))
styles.add(ParagraphStyle(
    name="ReportBody", parent=styles["BodyText"], fontName="ReportRegular",
    fontSize=10, leading=15.2, textColor=INK, alignment=TA_JUSTIFY,
    spaceAfter=7,
))
styles.add(ParagraphStyle(
    name="ReportBullet", parent=styles["ReportBody"], leftIndent=15,
    firstLineIndent=-10, bulletIndent=5, spaceAfter=4,
))
styles.add(ParagraphStyle(
    name="ReportCode", parent=styles["Code"], fontName="ReportRegular",
    fontSize=9, leading=13, textColor=INK, backColor=LIGHT_WINE,
    borderColor=WINE, borderWidth=0.5, borderPadding=8,
    leftIndent=4, rightIndent=4, spaceBefore=5, spaceAfter=10,
))
styles.add(ParagraphStyle(
    name="ReportSmall", parent=styles["Normal"], fontName="ReportRegular",
    fontSize=8.5, leading=12, textColor=INK,
))
styles.add(ParagraphStyle(
    name="ReportTableHeader", parent=styles["ReportSmall"], fontName="ReportBold",
    textColor=colors.white, alignment=TA_LEFT,
))
styles.add(ParagraphStyle(
    name="ReportLink", parent=styles["ReportBody"], textColor=WINE, wordWrap="CJK",
))


def paragraph(text: str, style: str = "ReportBody") -> Paragraph:
    return Paragraph(escape(text).replace("\n", "<br/>"), styles[style])


def rich(text: str, style: str = "ReportBody") -> Paragraph:
    return Paragraph(text, styles[style])


def code(text: str) -> Paragraph:
    return paragraph(text, "ReportCode")


def bullet(text: str) -> Paragraph:
    return rich(f"• {escape(text)}", "ReportBullet")


def link(label: str, url: str) -> Paragraph:
    return rich(
        f'<link href="{url}" color="#7B1E3A"><u>{escape(label)}</u></link><br/>'
        f'<font size="8" color="#696969">{escape(url)}</font>',
        "ReportLink",
    )


def table(headers, rows, widths):
    data = [[paragraph(value, "ReportTableHeader") for value in headers]]
    data.extend([[paragraph(str(value), "ReportSmall") for value in row] for row in rows])
    result = Table(data, colWidths=widths, repeatRows=1, hAlign="LEFT")
    result.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), WINE),
        ("ROWBACKGROUNDS", (0, 1), (-1, -1), [colors.white, LIGHT_WINE]),
        ("GRID", (0, 0), (-1, -1), 0.4, colors.HexColor("#D6BBC4")),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
        ("LEFTPADDING", (0, 0), (-1, -1), 7),
        ("RIGHTPADDING", (0, 0), (-1, -1), 7),
        ("TOPPADDING", (0, 0), (-1, -1), 7),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 7),
    ]))
    return result


def footer(canvas, doc):
    canvas.saveState()
    canvas.setFont("ReportRegular", 8)
    canvas.setFillColor(MUTED)
    canvas.drawString(2.2 * cm, 1.25 * cm, "Compiladores I — Trabalho Prático 2")
    canvas.drawRightString(A4[0] - 2.2 * cm, 1.25 * cm, f"Página {doc.page}")
    canvas.setStrokeColor(WINE)
    canvas.setLineWidth(0.5)
    canvas.line(2.2 * cm, 1.55 * cm, A4[0] - 2.2 * cm, 1.55 * cm)
    canvas.restoreState()


def build_report() -> None:
    doc = BaseDocTemplate(
        str(OUTPUT), pagesize=A4,
        rightMargin=2.2 * cm, leftMargin=2.2 * cm,
        topMargin=2.0 * cm, bottomMargin=2.1 * cm,
        title="Compiladores I — Trabalho Prático 2",
        author="Heitor Rodrigues Marques Silvestre; Jenniffer Oliveira Checchia",
        subject="Gerador de analisadores sintáticos descendentes recursivos",
        keywords="compiladores, EBNF, ANSI C, AST, Manim, macOS",
    )
    frame = Frame(
        doc.leftMargin, doc.bottomMargin, doc.width, doc.height,
        leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=0,
    )
    doc.addPageTemplates([PageTemplate(id="report", frames=[frame], onPage=footer)])
    story = []

    story.extend([
        Spacer(1, 1.1 * cm),
        paragraph("Compiladores I", "ReportSubtitle"),
        paragraph("Trabalho Prático 2", "ReportTitle"),
        paragraph("Gerador de Analisadores Sintáticos Descendentes Recursivos", "ReportSubtitle"),
        HRFlowable(width="75%", thickness=1.2, color=WINE, spaceBefore=8, spaceAfter=24, hAlign="CENTER"),
        paragraph("Alunos", "ReportSubtitle"),
        paragraph("Heitor Rodrigues Marques Silvestre", "ReportAuthor"),
        paragraph("Jenniffer Oliveira Checchia", "ReportAuthor"),
        Spacer(1, 1.0 * cm),
        paragraph("Resumo", "ReportHeading1"),
        paragraph(
            "Este relatório apresenta o Trabalho Prático 2 de Compiladores I: um gerador que lê uma gramática em EBNF e produz, em ANSI C, um analisador sintático descendente recursivo. O projeto contempla leitura e verificação da gramática, cálculo dos conjuntos usados na análise LL(1), geração do analisador e integração opcional com o scanner de Micro C. Como extensões, foram desenvolvidos quatro recursos de pontuação extra e uma animação didática da árvore de derivação com Manim Community."
        ),
        paragraph(
            "O resultado é uma solução de plataforma cruzada, com o núcleo independente de Python e documentação para Windows, Linux/WSL e macOS. O parser gerado permanece em C; Python é usado apenas pela extensão visual. O relatório enfatiza a arquitetura, o processo de execução, as extensões adicionais e as evidências obtidas por testes e renderização real."
        ),
        paragraph(
            "O trabalho está publicado na branch tp2-manim. A branch principal não foi modificada no desenvolvimento destas extensões. O texto deste documento descreve o estado do projeto no commit a04e4d8."
        ),
        rich(
            f'Acesse o repositório na branch do trabalho: <link href="{REPOSITORY}" color="#7B1E3A"><u>COMPILADORES-TP1 — tp2-manim</u></link>.',
            "ReportBody",
        ),
        PageBreak(),
    ])

    story += [
        paragraph("1. Objetivo e visão geral", "ReportHeading1"),
        paragraph(
            "O objetivo central é transformar uma especificação gramatical escrita em EBNF em um analisador sintático funcional. Em vez de exigir a implementação manual de um parser para cada linguagem, o gerador interpreta a notação, verifica suas restrições e emite código que implementa o reconhecimento sintático de programas da linguagem definida."
        ),
        paragraph(
            "A arquitetura separa o processamento da gramática da execução dos programas aceitos pelo parser gerado. A primeira etapa realiza leitura léxica e sintática da EBNF. Depois, ocorrem verificações semânticas, análise de alcançabilidade e produtividade, e cálculo dos conjuntos anulável, FIRST e FOLLOW. A verificação LL(1) determina se a gramática pode ser reconhecida por análise recursiva descente preditiva antes da geração do código C."
        ),
        paragraph("Principais componentes", "ReportHeading2"),
        bullet("Leitura da EBNF: seções TOKENS, EXTERN e RULES, com regras, alternativas, sequências, opcionais e repetições."),
        bullet("Verificação e análise: símbolos indefinidos ou duplicados, não terminais inalcançáveis, improdutividade e recursão à esquerda."),
        bullet("Conjuntos de análise: representação por mapas de bits e cálculo iterativo de ponto fixo para FIRST e FOLLOW."),
        bullet("Validação LL(1): diagnóstico de conflitos nas condições necessárias para previsão determinística da derivação."),
        bullet("Geração do analisador: código ANSI C, suporte léxico e opções de árvore, AST e recuperação de erros."),
        bullet("Micro C: integração opcional com o scanner e os tokens fornecidos pelo Trabalho Prático 1."),
        paragraph(
            "As estruturas que representam a gramática são utilizadas ao longo das etapas, de modo que as decisões da análise LL(1) sejam refletidas no analisador emitido. A separação também permite que testes exercitem o gerador e cada parser gerado de forma independente da extensão de animação."
        ),
        paragraph("2. Compilação e execução", "ReportHeading1"),
        paragraph(
            "Os comandos devem ser executados no diretório tp2-gerador-ebnf. O fluxo pelo Makefile oferece compilação do gerador, testes, geração do analisador Micro C e limpeza dos artefatos. GCC ou Clang pode ser utilizado, desde que o código seja validado com as opções ANSI C do projeto. Os exemplos abaixo são reproduzidos diretamente da documentação."
        ),
        paragraph("Compilação e testes no ambiente POSIX", "ReportHeading2"),
        code("make\nmake test\nmake test-extras\nmake microc"),
        paragraph("Exemplo no Windows com PowerShell", "ReportHeading2"),
        code(".\\gerador.exe testes\\gramaticas\\expr.ebnf -o build\\expr\ngcc -ansi -pedantic -Wall -Wextra -o build\\expr.exe build\\expr.c\n.\\build\\expr.exe testes\\programas\\expr\\aceito_1.txt"),
        paragraph("Exemplo no macOS com Clang", "ReportHeading2"),
        code("xcode-select --install\nmake CC=clang\n./gerador testes/gramaticas/expr.ebnf -o build/expr\nclang -ansi -pedantic -Wall -Wextra -o build/expr build/expr.c\n./build/expr testes/programas/expr/aceito_1.txt"),
        paragraph(
            "O Makefile aceita a seleção explícita do compilador com CC e do interpretador com PYTHON. O padrão configurável para testes é python3, enquanto o compilador padrão pode ser cc, gcc ou clang, conforme o ambiente. Entradas já tokenizadas podem ser processadas com a opção --tokens do parser gerado."
        ),
    ]

    story += [
        paragraph("3. Recursos de pontuação extra", "ReportHeading1"),
        paragraph(
            "Os quatro recursos extras são opcionais e não alteram o comportamento padrão quando suas opções não são utilizadas. Essa característica preserva a compatibilidade com o fluxo original de geração e reconhecimento. A tabela a seguir relaciona a ativação de cada recurso e seu resultado principal."
        ),
        table(
            ["Recurso", "Ativação", "Resultado"],
            [
                ["Continuar após erros", "--recover no gerador e no parser", "Acumula erros e segue até o fim."],
                ["Árvore sintática abstrata", "--ast no gerador e no parser", "Imprime os símbolos relevantes e seus lexemas."],
                ["Conflitos LL(1) simples", "--auto-conflicts no gerador", "Fatora alternativas com primeiro símbolo comum."],
                ["EBNF auto-descrita", "testes/gramaticas/ebnf.ebnf", "A gramática da notação é processada pelo próprio gerador."],
            ],
            [4.0 * cm, 5.1 * cm, 7.5 * cm],
        ),
        paragraph("3.1 Continuar a análise após erros", "ReportHeading2"),
        paragraph(
            "A recuperação utiliza descarte de tokens inesperados. Ao detectar uma inconsistência, o analisador registra o diagnóstico e procura prosseguir em vez de encerrar imediatamente. Erros léxicos do scanner próprio também são descartados para permitir a continuação. Ao final, o processo retorna código diferente de zero quando ocorreram erros; portanto, recuperar-se não significa aceitar um programa inválido."
        ),
        code(".\\gerador testes\\gramaticas\\expr.ebnf -o build\\expr_recover --recover\ngcc -ansi -pedantic -Wall -Wextra -o build\\expr_recover.exe build\\expr_recover.c\n.\\build\\expr_recover.exe --recover entrada_com_erros.txt"),
        paragraph("3.2 Construção da árvore sintática abstrata", "ReportHeading2"),
        paragraph(
            "A opção --ast habilita a criação e a impressão da AST durante a análise. A representação preserva não terminais e terminais nomeados com seus lexemas. Literais usados apenas como pontuação e agrupamentos auxiliares da EBNF são omitidos, tornando a árvore mais legível e evitando que detalhes de agrupamento dominem o resultado."
        ),
        code(".\\gerador testes\\gramaticas\\expr.ebnf -o build\\expr_ast --ast\ngcc -ansi -pedantic -Wall -Wextra -o build\\expr_ast.exe build\\expr_ast.c\n.\\build\\expr_ast.exe --ast testes\\programas\\expr\\aceito_1.txt"),
        paragraph("3.3 Correção automática de conflitos simples", "ReportHeading2"),
        paragraph(
            "A opção --auto-conflicts aplica fatoração à esquerda quando alternativas compartilham diretamente o mesmo primeiro terminal ou não terminal. O escopo limitado é deliberado: conflitos ambíguos ou mais complexos que não apresentam essa forma continuam sendo rejeitados. Sem a opção, mantém-se o diagnóstico LL(1) original."
        ),
        code(".\\gerador testes\\gramaticas\\conflito_ll1.ebnf -o build\\conflito --auto-conflicts"),
        paragraph("3.4 Descrição da própria EBNF", "ReportHeading2"),
        paragraph(
            "A gramática ebnf.ebnf formaliza a linguagem de gramáticas aceita pelo gerador. Inclui as seções TOKENS e EXTERN, a seção RULES, regras, alternativas, sequências, opcionais e repetições. Comentários não fazem parte dessa gramática sintática, pois são removidos pelo meta-analisador léxico antes do reconhecimento."
        ),
        code(".\\gerador testes\\gramaticas\\ebnf.ebnf -o build\\ebnf --ast --recover"),
        paragraph(
            "Esse processamento demonstra que a especificação da própria notação pode ser lida pelo gerador. O analisador resultante pode ser compilado e utilizado para reconhecer arquivos de gramática, além de permitir o exercício simultâneo da AST e da recuperação de erros."
        ),
    ]

    story += [
        PageBreak(),
        paragraph("4. Animação didática da árvore de derivação", "ReportHeading1"),
        paragraph(
            "A extensão em Python utiliza Manim Community para tornar visível a construção da árvore textual produzida pela opção --tree. Cada não terminal e cada token consumido aparece como um nó, na ordem em que o analisador o processa. As arestas são apresentadas junto com os filhos, de modo que a relação entre a estrutura sintática e o reconhecimento do programa fica mais compreensível para apresentação e estudo."
        ),
        paragraph(
            "O fluxo não duplica o parser em Python. O script executa o binário C compilado com --tree, captura sua saída sem shell, interpreta a indentação de dois espaços por nível e reconstrói a hierarquia. Rejeições do analisador impedem a apresentação de uma árvore parcial como se fosse uma derivação válida."
        ),
        paragraph("Artefatos produzidos pela CLI", "ReportHeading2"),
        bullet("arvore.txt: saída textual original produzida pelo analisador com --tree."),
        bullet("arvore.gif: animação da criação progressiva dos nós e das arestas."),
        bullet("arvore.png: imagem correspondente ao estado final da árvore."),
        paragraph(
            "A cena mantém proporção 16:9 e permite escolher a qualidade baixa, média ou alta. A resolução final é determinada pelo perfil de renderização do Manim. A extensão é independente do processamento principal: um ambiente sem Python ou Manim continua podendo compilar e executar o gerador e seus analisadores."
        ),
        paragraph("Execução no macOS", "ReportHeading2"),
        code("python3 -m pip install manim\npython3 animacao/arvore_manim.py \\\n  --parser build/expr \\\n  --entrada testes/programas/expr/aceito_1.txt \\\n  --saida build/animacao/expr \\\n  --qualidade media"),
        paragraph(
            "O analisador utilizado no parâmetro --parser precisa ter sido gerado com --tree. Esse detalhe é essencial: um binário sem suporte a árvore não fornece os nós necessários para a reconstrução visual. Após a renderização, a CLI informa o diretório absoluto e os caminhos completos dos três artefatos."
        ),
        code("Diretório da animação: /Users/usuario/projeto/tp2-gerador-ebnf/build/animacao/expr\nGIF: /Users/usuario/projeto/tp2-gerador-ebnf/build/animacao/expr/arvore.gif\nPNG: /Users/usuario/projeto/tp2-gerador-ebnf/build/animacao/expr/arvore.png\nÁrvore textual: /Users/usuario/projeto/tp2-gerador-ebnf/build/animacao/expr/arvore.txt"),
        paragraph(
            "Esse retorno elimina a necessidade de procurar os arquivos na estrutura interna do Manim. No macOS, o diretório pode ser aberto diretamente no Finder com o seguinte comando:"
        ),
        code("open build/animacao/expr"),
        paragraph("4.1 Compatibilidade multiplataforma", "ReportHeading2"),
        paragraph(
            "O núcleo C utiliza construções ANSI C e caminhos que são tratados pelo ambiente de compilação. O Makefile oferece a seleção CC=clang e os testes detectam o sufixo .exe somente no Windows. No macOS, são utilizados caminhos POSIX e o analisador sem extensão. A documentação inclui a instalação das ferramentas de linha de comando do Xcode e orienta o uso das dependências nativas do Manim."
        ),
        paragraph(
            "As alterações de compatibilidade cobrem o Makefile, o script oficial de testes, os testes dos extras e os testes da animação. A CLI centraliza a resolução do diretório e utiliza pathlib para tratar caminhos no sistema operativo. A saída dos artefatos é apresentada tanto no Windows como no Linux e macOS por meio das mesmas funções."
        ),
    ]

    story += [
        PageBreak(),
        paragraph("5. Testes e evidências de funcionamento", "ReportHeading1"),
        paragraph(
            "A validação do projeto combina testes automatizados, compilação do gerador em ANSI C, geração e compilação de analisadores auxiliares e uma renderização efetiva da animação. Essa combinação permite verificar tanto o comportamento interno do gerador como a integração entre o parser real, a leitura da árvore e o Manim."
        ),
        table(
            ["Conjunto de validação", "Resultado observado"],
            [
                ["Gramáticas", "5: expr, calc e microc válidas; conflito_ll1 e recursao_esquerda com erros."],
                ["Casos das gramáticas válidas", "Expr: 3 aceitos/4 rejeitados; calc: 3/3; Micro C: 3/3."],
                ["Suíte oficial", "18/18 testes não-Micro C aprovados; 6/6 casos Micro C aprovados."],
                ["Recursos extras", "4/4 testes aprovados: AST, recuperação, fatoração e auto-EBNF."],
                ["Animação", "12/12 testes aprovados; GIF, PNG e TXT renderizados."],
            ],
            [8.2 * cm, 8.4 * cm],
        ),
        paragraph(
            "A matriz atende ao requisito mínimo do enunciado: cinco gramáticas, sendo três válidas — expr, calc e Micro C — e duas destinadas a erros — conflito_ll1 e recursao_esquerda. Para cada gramática válida há pelo menos três entradas aceitas e três rejeitadas. Os testes dos extras cobrem a impressão da AST sem pontuação literal desnecessária, o acúmulo de múltiplos erros com --recover, a fatoração automática e o reconhecimento da EBNF auto-descrita."
        ),
        paragraph(
            "A suíte da animação verifica a reconstrução da hierarquia, a ordem dos nós, lexemas com espaços, a leitura da saída UTF-16 do Windows PowerShell, a rejeição de formatos inválidos, a proporção 16:9 e a apresentação dos caminhos absolutos. O teste de integração compila o gerador e um analisador real para confirmar a aceitação da entrada e a exportação da árvore."
        ),
        code("python -m unittest discover -s animacao\\testes -v\npython -m unittest testes\\test_extras.py -v"),
        paragraph(
            "No Linux e macOS, o equivalente utiliza python3 e barras normais. O teste de integração pode selecionar o compilador definido por CC. Na última validação local, o Manim Community 0.22.1 estava instalado e uma execução com um parser temporário gerado com --tree confirmou a criação dos três artefatos. O diretório temporário foi removido após a conferência."
        ),
        paragraph(
            "A compatibilidade foi estruturada e verificada no Windows, com caminhos POSIX na implementação e Clang selecionável pelo Makefile. A execução nativa do Manim no macOS ainda depende das bibliotecas instaladas na máquina de destino."
        ),
        paragraph("6. Conformidade com os requisitos e entrega", "ReportHeading1"),
        paragraph(
            "O projeto atende ao requisito de linguagem com código do gerador e código gerado em ANSI C, compilados com -ansi -pedantic -Wall -Wextra. A leitura da EBNF e a geração do analisador são implementadas nos fontes do grupo; o Flex permanece restrito ao scanner do TP1. O código gerado conserva a regra de origem em comentário, e as rotinas de destruição liberam as estruturas alocadas ao final."
        ),
        paragraph(
            "Os entregáveis previstos estão no repositório: código-fonte, scanner adaptado, testes, README, USO_IA.md e este relatório. Para a apresentação, o fluxo recomendado é criar ao vivo uma gramática nova, por exemplo uma lista de identificadores separada por vírgulas, gerar o parser com --tree, compilar com GCC ou Clang e processar uma entrada aceita e uma rejeitada, explicando os diagnósticos e a árvore produzida."
        ),
        paragraph(
            "O desenvolvimento da animação, da documentação e dos recursos extras foi publicado na branch tp2-manim, preservando a branch principal. A branch reúne o suporte multiplataforma, a seleção do compilador nos testes e a exibição explícita dos caminhos dos artefatos da animação."
        ),
        paragraph("Hiperlinks do projeto", "ReportHeading2"),
        link("Repositório na branch tp2-manim", REPOSITORY),
        Spacer(1, 4),
        link("Leia-me e comandos de execução", "https://github.com/HektorSylvester/COMPILADORES-TP1/blob/tp2-manim/README.md"),
        Spacer(1, 4),
        link("Documentação da extensão Manim", "https://github.com/HektorSylvester/COMPILADORES-TP1/blob/tp2-manim/tp2-gerador-ebnf/animacao/README.md"),
        Spacer(1, 4),
        link("Código-fonte da animação", "https://github.com/HektorSylvester/COMPILADORES-TP1/blob/tp2-manim/tp2-gerador-ebnf/animacao/arvore_manim.py"),
        Spacer(1, 4),
        link("Testes dos recursos extras", "https://github.com/HektorSylvester/COMPILADORES-TP1/blob/tp2-manim/tp2-gerador-ebnf/testes/test_extras.py"),
        Spacer(1, 4),
        link("Registro de uso de IA", "https://github.com/HektorSylvester/COMPILADORES-TP1/blob/tp2-manim/USO_IA.md"),
        paragraph("7. Considerações finais", "ReportHeading1"),
        paragraph(
            "O trabalho entrega um gerador funcional e extensível, com diagnóstico, AST, tratamento de conflitos e visualização da análise. A animação com Manim acompanha a criação dos nós; a documentação e os hiperlinks tornam a reprodução e a localização dos artefatos diretas. O uso de IA está registrado no link USO_IA.md acima.",
        ),
    ]

    # Frame compartilhado pelo único template; evita condensar o texto em uma única página.
    doc.build(story)
    print(f"PDF gerado: {OUTPUT}")


if __name__ == "__main__":
    build_report()
