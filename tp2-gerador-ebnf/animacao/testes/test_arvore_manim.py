import subprocess
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from arvore_manim import (
    ErroArvore,
    _dimensoes_cena,
    executar_parser,
    ler_arvore,
    ler_arvore_arquivo,
    nos_em_ordem,
)


EXEMPLO = """expr
  term
    factor
      NUM '2'
    '*'
    factor
      ID 'x'
entrada aceita
"""


class TestArvore(unittest.TestCase):
    def test_reconstroi_hierarquia_e_ordem(self):
        raiz = ler_arvore(EXEMPLO)
        self.assertEqual(raiz.rotulo, "expr")
        self.assertEqual(raiz.filhos[0].rotulo, "term")
        self.assertEqual(raiz.filhos[0].filhos[0].rotulo, "factor")
        self.assertEqual(raiz.filhos[0].filhos[1].rotulo, "'*'")
        self.assertEqual([n.rotulo for n in nos_em_ordem(raiz)], [
            "expr", "term", "factor", "NUM '2'", "'*'", "factor", "ID 'x'"
        ])

    def test_aceita_espacos_em_lexema(self):
        raiz = ler_arvore("expr\n  ID 'valor com espacos'\nentrada aceita\n")
        self.assertEqual(raiz.filhos[0].rotulo, "ID 'valor com espacos'")

    def test_le_arquivo_utf16_do_powershell(self):
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            caminho = Path(tmp) / "arvore.txt"
            caminho.write_bytes(EXEMPLO.encode("utf-16"))
            self.assertEqual(ler_arvore_arquivo(caminho).rotulo, "expr")

    def test_rejeita_saida_sem_arvore(self):
        with self.assertRaises(ErroArvore):
            ler_arvore("entrada aceita\n")

    def test_rejeita_recuo_invalido(self):
        with self.assertRaises(ErroArvore):
            ler_arvore("expr\n   term\n")

    def test_rejeita_duas_raizes(self):
        with self.assertRaises(ErroArvore):
            ler_arvore("expr\nterm\n")

    def test_dimensoes_da_cena_sao_16_por_9(self):
        largura, altura = _dimensoes_cena(ler_arvore(EXEMPLO))
        self.assertAlmostEqual(largura / altura, 16 / 9)

    def test_executa_parser_sem_shell(self):
        with patch("arvore_manim.subprocess.run") as run:
            run.return_value = subprocess.CompletedProcess(
                ["parser"], 0, stdout=EXEMPLO, stderr=""
            )
            with patch("arvore_manim.Path.is_file", return_value=True):
                saida = executar_parser(Path("parser"), Path("entrada.txt"))
            self.assertEqual(saida, EXEMPLO)
            comando = run.call_args.args[0]
            self.assertEqual(comando[1:], ["--tree", str(Path("entrada.txt").resolve())])
            self.assertNotIn("shell", run.call_args.kwargs)

    def test_propaga_rejeicao_do_parser(self):
        with patch("arvore_manim.subprocess.run") as run:
            run.return_value = subprocess.CompletedProcess(
                ["parser"], 1, stdout="", stderr="erro sintatico"
            )
            with patch("arvore_manim.Path.is_file", return_value=True):
                with self.assertRaisesRegex(ErroArvore, "status 1"):
                    executar_parser(Path("parser"), Path("entrada.txt"))


if __name__ == "__main__":
    unittest.main()
