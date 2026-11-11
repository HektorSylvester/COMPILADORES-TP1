#!/usr/bin/env python3
"""Renderiza a arvore textual produzida por um analisador gerado."""

import argparse
import os
from pathlib import Path
import subprocess
import sys


def ler_arvore(parser: Path, entrada: Path) -> list[tuple[int, str]]:
    try:
        resultado = subprocess.run(
            [str(parser), "--tree", str(entrada)],
            capture_output=True,
            text=True,
            check=False,
        )
    except OSError as erro:
        raise RuntimeError(f"nao foi possivel executar o parser: {erro}") from erro

    if resultado.returncode != 0:
        mensagem = resultado.stderr.strip() or "o parser terminou com erro"
        raise RuntimeError(f"parser retornou {resultado.returncode}: {mensagem}")

    arvore = []
    for linha in resultado.stdout.splitlines():
        if not linha.strip() or linha.strip() == "entrada aceita":
            continue
        espacos = len(linha) - len(linha.lstrip(" "))
        arvore.append((espacos // 2, linha.strip()))
    if not arvore:
        raise RuntimeError("o parser nao produziu uma arvore com --tree")
    return arvore


def criar_cena(arvore: list[tuple[int, str]], saida: Path) -> None:
    try:
        from manim import DOWN, LEFT, Scene, Text, config
    except ImportError as erro:
        raise RuntimeError(
            "Manim nao esta instalado; execute "
            "'python3 -m pip install manim --break-system-packages'"
        ) from erro

    class ArvoreScene(Scene):
        def construct(self):
            for indice, (profundidade, texto) in enumerate(arvore):
                item = Text(texto, font_size=28)
                item.shift(
                    LEFT * (6 - profundidade * 0.45)
                    + DOWN * (3 - indice * 0.38)
                )
                item.set_opacity(0)
                self.play(item.animate.set_opacity(1), run_time=0.15)
            self.wait(1)

    saida.parent.mkdir(parents=True, exist_ok=True)
    config.media_dir = str(saida.parent.resolve())
    config.video_dir = str(saida.parent.resolve())
    config.output_file = saida.name
    ArvoreScene().render()


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Gera uma animacao da arvore produzida por um parser."
    )
    parser.add_argument("--parser", required=True, type=Path)
    parser.add_argument("--entrada", required=True, type=Path)
    parser.add_argument("--saida", required=True, type=Path)
    argumentos = parser.parse_args()

    if not argumentos.parser.is_file() or not os.access(argumentos.parser, os.X_OK):
        parser.error(f"parser nao executavel: {argumentos.parser}")
    if not argumentos.entrada.is_file():
        parser.error(f"arquivo de entrada nao encontrado: {argumentos.entrada}")

    try:
        arvore = ler_arvore(argumentos.parser, argumentos.entrada)
        criar_cena(arvore, argumentos.saida)
    except RuntimeError as erro:
        print(f"arvore_manim: {erro}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
