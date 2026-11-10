import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "animacao"))

from arvore_manim import executar_parser, ler_arvore, nos_em_ordem


COMPILADOR = os.environ.get("CC") or shutil.which("cc") or shutil.which("gcc")


@unittest.skipUnless(COMPILADOR, "um compilador C é necessário para o teste de integração")
class TestIntegracaoExpr(unittest.TestCase):
    def test_parser_real_continua_aceitando_e_exportando_arvore(self):
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            gerador = tmp / ("gerador.exe" if sys.platform == "win32" else "gerador")
            fontes = sorted((ROOT / "src").glob("*.c"))
            subprocess.run(
                [COMPILADOR, "-ansi", "-pedantic", "-Wall", "-Wextra",
                 "-o", str(gerador), *(str(fonte) for fonte in fontes)],
                cwd=ROOT, check=True, capture_output=True,
            )
            prefixo = tmp / "expr"
            subprocess.run(
                [str(gerador), str(ROOT / "testes/gramaticas/expr.ebnf"),
                 "-o", str(prefixo), "--tree"],
                cwd=ROOT, check=True, capture_output=True,
            )
            executavel = tmp / ("expr.exe" if sys.platform == "win32" else "expr")
            subprocess.run(
                [COMPILADOR, "-ansi", "-pedantic", "-Wall", "-Wextra",
                 "-o", str(executavel), str(prefixo.with_suffix(".c"))],
                cwd=ROOT, check=True, capture_output=True,
            )
            entrada = ROOT / "testes/programas/expr/aceito_1.txt"
            saida = executar_parser(executavel, entrada)
            self.assertIn("entrada aceita", saida)
            raiz = ler_arvore(saida)
            self.assertEqual(raiz.rotulo, "expr")
            self.assertGreater(len(nos_em_ordem(raiz)), 5)


if __name__ == "__main__":
    unittest.main()
