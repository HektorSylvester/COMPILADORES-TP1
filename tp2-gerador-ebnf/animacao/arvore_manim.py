"""Anima a árvore textual produzida por um analisador gerado com ``--tree``.

O parser em C continua sendo a única fonte da derivação. Este módulo apenas
executa o binário, interpreta a indentação e apresenta o resultado com Manim.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import sys
from dataclasses import dataclass, field
from typing import Sequence


@dataclass
class NoArvore:
    """Um nó da derivação, na ordem em que aparece na saída do analisador."""

    rotulo: str
    filhos: list["NoArvore"] = field(default_factory=list)
    ordem: int = 0

    def folha(self) -> bool:
        return not self.filhos


class ErroArvore(ValueError):
    """A saída não contém uma árvore textual válida."""


def _rotulo(linha: str, numero: int) -> tuple[int, str]:
    if "\t" in linha:
        raise ErroArvore(f"linha {numero}: use espaços, não tabulações, no recuo")
    recuo = len(linha) - len(linha.lstrip(" "))
    if recuo % 2:
        raise ErroArvore(f"linha {numero}: recuo deve ser múltiplo de dois espaços")
    texto = linha.strip()
    if not texto:
        raise ErroArvore(f"linha {numero}: rótulo vazio")
    return recuo // 2, texto


def ler_arvore(texto: str) -> NoArvore:
    """Reconstrói a árvore de ``--tree`` e ignora a mensagem final de aceitação.

    Cada linha tem dois espaços por nível. A mensagem ``entrada aceita`` é
    produzida pelo ``main`` do analisador e não pertence à derivação.
    """
    linhas = texto.splitlines()
    while linhas and not linhas[-1].strip():
        linhas.pop()
    if linhas and linhas[-1].strip() == "entrada aceita":
        linhas.pop()
    while linhas and not linhas[-1].strip():
        linhas.pop()
    if not linhas:
        raise ErroArvore("a saída não contém nós; gere o analisador com --tree")

    raiz: NoArvore | None = None
    pilha: list[NoArvore] = []
    ordem = 0
    for numero, linha in enumerate(linhas, 1):
        if not linha.strip():
            continue
        nivel, rotulo = _rotulo(linha, numero)
        if raiz is None:
            if nivel != 0:
                raise ErroArvore("a primeira linha deve ser a raiz, sem recuo")
            raiz = NoArvore(rotulo, ordem=ordem)
            pilha = [raiz]
        else:
            if nivel == 0:
                raise ErroArvore(f"linha {numero}: há mais de uma raiz")
            if nivel > len(pilha):
                raise ErroArvore(f"linha {numero}: nível sem nó pai")
            pai = pilha[nivel - 1]
            no = NoArvore(rotulo, ordem=ordem)
            pai.filhos.append(no)
            pilha = pilha[:nivel]
            pilha.append(no)
        ordem += 1
    if raiz is None:
        raise ErroArvore("árvore vazia")
    return raiz


def nos_em_ordem(raiz: NoArvore) -> list[NoArvore]:
    """Devolve os nós na ordem de construção registrada pelo parser."""
    resultado: list[NoArvore] = []

    def visitar(no: NoArvore) -> None:
        resultado.append(no)
        for filho in no.filhos:
            visitar(filho)

    visitar(raiz)
    return sorted(resultado, key=lambda no: no.ordem)


def executar_parser(parser: Path, entrada: Path) -> str:
    """Executa sem shell e devolve stdout; stderr é preservado em caso de erro."""
    if not parser.is_file():
        raise ErroArvore(f"analisador não encontrado: {parser}")
    if not entrada.is_file():
        raise ErroArvore(f"programa de entrada não encontrado: {entrada}")
    try:
        processo = subprocess.run(
            [str(parser.resolve()), "--tree", str(entrada.resolve())],
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            check=False,
        )
    except OSError as exc:
        raise ErroArvore(f"não foi possível executar o analisador: {exc}") from exc
    if processo.returncode != 0:
        detalhe = processo.stderr.strip() or processo.stdout.strip()
        raise ErroArvore(
            f"o analisador rejeitou a entrada (status {processo.returncode})"
            + (f":\n{detalhe}" if detalhe else "")
        )
    return processo.stdout


def _manim():
    try:
        import manim
    except ImportError as exc:
        raise ErroArvore(
            "Manim Community não está instalado. Execute: python -m pip install manim"
        ) from exc
    return manim


def _dimensoes(raiz: NoArvore) -> tuple[int, int]:
    """Calcula posições por folhas, evitando sobreposição de irmãos."""
    folhas = 0
    profundidade = 0

    def visitar(no: NoArvore, nivel: int) -> None:
        nonlocal folhas, profundidade
        profundidade = max(profundidade, nivel)
        if no.folha():
            folhas += 1
        for filho in no.filhos:
            visitar(filho, nivel + 1)

    visitar(raiz, 0)
    return max(folhas, 1), profundidade + 1


def _dimensoes_cena(raiz: NoArvore) -> tuple[float, float]:
    """Calcula o enquadramento da cena mantendo a proporção 16:9."""
    folhas, niveis = _dimensoes(raiz)
    largura = max(12.0, min(30.0, folhas * 1.35))
    altura = max(8.0, min(20.0, niveis * 1.35 + 1.5))
    proporcao = 16 / 9
    if largura / altura < proporcao:
        largura = altura * proporcao
    else:
        altura = largura / proporcao
    return largura, altura


def _encurtar(rotulo: str, limite: int = 25) -> str:
    return rotulo if len(rotulo) <= limite else rotulo[: limite - 3] + "..."


def construir_cena(raiz: NoArvore, qualidade: str = "media"):
    """Cria uma cena Manim sem exigir Manim para analisar a árvore."""
    m = _manim()
    nos = nos_em_ordem(raiz)
    folhas, niveis = _dimensoes(raiz)
    largura, altura = _dimensoes_cena(raiz)
    m.config.frame_width = largura
    m.config.frame_height = altura
    m.config.frame_rate = 24
    m.config.background_color = "#101827"

    class CenaArvore(m.Scene):
        def construct(self):
            self.camera.frame_width = largura
            self.camera.frame_height = altura
            self.camera.frame_center = [0, 0, 0]
            self.camera.background_color = "#101827"
            titulo = m.Text("Construção da árvore de derivação", font_size=30, color=m.WHITE)
            titulo.move_to([0, altura / 2 - 0.45, 0])
            self.play(m.Write(titulo), run_time=0.7)
            posicoes: dict[int, object] = {}
            objetos: dict[int, object] = {}
            pais = {
                filho.ordem: no
                for no in nos
                for filho in no.filhos
            }
            contador = [0]

            def posicionar(no: NoArvore, nivel: int) -> float:
                if no.folha():
                    x = contador[0]
                    contador[0] += 1
                else:
                    xs = [posicionar(filho, nivel + 1) for filho in no.filhos]
                    x = sum(xs) / len(xs)
                posicoes[no.ordem] = (x, nivel)
                return x

            posicionar(raiz, 0)
            escala_x = (largura - 3.0) / max(folhas - 1, 1)
            escala_y = (altura - 3.0) / max(niveis - 1, 1)
            for no in nos:
                x, nivel = posicoes[no.ordem]
                px = (x - (folhas - 1) / 2) * escala_x
                py = altura / 2 - 1.85 - nivel * escala_y
                posicoes[no.ordem] = m.np.array([px, py, 0])

            for no in nos:
                x, y, _ = posicoes[no.ordem]
                cor = "#60A5FA" if no.filhos else "#34D399"
                caixa = m.RoundedRectangle(
                    corner_radius=0.12,
                    width=min(2.25, max(1.0, escala_x * 0.88)),
                    height=0.62,
                    stroke_color=cor,
                    stroke_width=2,
                    fill_color="#172554" if no.filhos else "#052E2B",
                    fill_opacity=0.95,
                )
                texto = m.Text(
                    _encurtar(no.rotulo),
                    font_size=18 if len(no.rotulo) < 18 else 13,
                    color=m.WHITE,
                )
                texto.move_to(caixa.get_center())
                grupo = m.VGroup(caixa, texto)
                grupo.move_to([x, y, 0])
                objetos[no.ordem] = grupo
                if no is raiz:
                    self.play(m.FadeIn(grupo, scale=0.65), run_time=0.35)
                else:
                    pai = pais[no.ordem]
                    grupo_pai = objetos[pai.ordem]
                    aresta = m.Line(
                        grupo_pai[0].get_bottom(),
                        caixa.get_top(),
                        color="#64748B",
                        stroke_width=2,
                    )
                    self.play(m.Create(aresta), m.FadeIn(grupo, scale=0.65), run_time=0.28)
            self.wait(0.8)

    CenaArvore.__name__ = "ArvoreDerivacao"
    return CenaArvore


def renderizar(raiz: NoArvore, destino: Path, qualidade: str = "media") -> tuple[Path, Path]:
    """Renderiza GIF e PNG usando a API do Manim, sem alterar o parser C."""
    m = _manim()
    destino = destino.expanduser().resolve()
    destino.mkdir(parents=True, exist_ok=True)
    gif = destino / "arvore.gif"
    png = destino / "arvore.png"
    qualidades = {"baixa": "low_quality", "media": "medium_quality", "alta": "high_quality"}
    if qualidade not in qualidades:
        raise ErroArvore(f"qualidade desconhecida: {qualidade}")
    m.config.quality = qualidades[qualidade]
    m.config.media_dir = str(destino.resolve())
    m.config.output_file = "arvore"
    m.config.save_last_frame = False
    m.config.format = "gif"
    cena = construir_cena(raiz, qualidade)()
    cena.render()
    candidatos = list(destino.rglob("*.gif"))
    if not candidatos:
        raise ErroArvore("o Manim terminou sem produzir um GIF")
    produzido = max(candidatos, key=lambda p: p.stat().st_mtime)
    if produzido != gif:
        produzido.replace(gif)

    # Uma segunda cena, com save_last_frame, produz o PNG do estado final.
    m.config.format = "png"
    m.config.save_last_frame = True
    m.config.output_file = "arvore"
    cena_png = construir_cena(raiz, qualidade)()
    cena_png.render()
    candidatos_png = list(destino.rglob("*.png"))
    if not candidatos_png:
        raise ErroArvore("o Manim terminou sem produzir um PNG")
    produzido_png = max(candidatos_png, key=lambda p: p.stat().st_mtime)
    if produzido_png != png:
        produzido_png.replace(png)
    return gif, png


def ler_arvore_arquivo(caminho: Path) -> NoArvore:
    """Lê UTF-8 ou o UTF-16 produzido por ``Out-File`` no Windows PowerShell."""
    dados = caminho.read_bytes()
    if dados.startswith((b"\xff\xfe", b"\xfe\xff")):
        return ler_arvore(dados.decode("utf-16"))
    return ler_arvore(dados.decode("utf-8-sig"))


def resumo_artefatos(destino: Path, gif: Path, png: Path, arvore: Path) -> str:
    """Formata os caminhos absolutos dos arquivos produzidos pela animação."""
    return "\n".join(
        (
            f"Diretório da animação: {destino.expanduser().resolve()}",
            f"GIF: {gif.expanduser().resolve()}",
            f"PNG: {png.expanduser().resolve()}",
            f"Árvore textual: {arvore.expanduser().resolve()}",
        )
    )


def _arvore_de_arquivo() -> NoArvore:
    """Entrada para ``manim animacao/arvore_manim.py ArvoreDerivacao``."""
    caminho = os.environ.get("ARVORE_TXT")
    if not caminho:
        raise ErroArvore(
            "defina ARVORE_TXT com o caminho de uma saída --tree; "
            "ou use a CLI com --parser e --entrada"
        )
    return ler_arvore_arquivo(Path(caminho))


# O Manim procura uma classe Scene no módulo. A classe só é criada quando o
# módulo é carregado pelo comando manim, não durante os testes sem Manim.
if os.environ.get("ARVORE_TXT"):
    ArvoreDerivacao = construir_cena(_arvore_de_arquivo())


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--parser", type=Path, required=True, help="analisador já compilado")
    parser.add_argument("--entrada", type=Path, required=True, help="programa aceito pelo analisador")
    parser.add_argument("--saida", type=Path, required=True, help="diretório dos artefatos")
    parser.add_argument("--qualidade", choices=("baixa", "media", "alta"), default="media")
    args = parser.parse_args(argv)
    try:
        destino = args.saida.expanduser().resolve()
        texto = executar_parser(args.parser, args.entrada)
        raiz = ler_arvore(texto)
        destino.mkdir(parents=True, exist_ok=True)
        arvore = destino / "arvore.txt"
        arvore.write_text(texto, encoding="utf-8")
        gif, png = renderizar(raiz, destino, args.qualidade)
    except ErroArvore as exc:
        print(f"erro: {exc}", file=sys.stderr)
        return 2
    print(resumo_artefatos(destino, gif, png, arvore))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
