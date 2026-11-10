import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
GRAMATICAS = ROOT / "testes" / "gramaticas"
COMPILADOR = os.environ.get("CC") or shutil.which("cc") or shutil.which("gcc")


@unittest.skipUnless(COMPILADOR, "um compilador C e necessario para os testes")
class TestExtras(unittest.TestCase):
    def compilar_gerador(self, diretorio):
        executavel = diretorio / (
            "gerador.exe" if sys.platform == "win32" else "gerador"
        )
        fontes = sorted((ROOT / "src").glob("*.c"))
        subprocess.run(
            [
                COMPILADOR,
                "-ansi",
                "-pedantic",
                "-Wall",
                "-Wextra",
                "-o",
                str(executavel),
                *(str(fonte) for fonte in fontes),
            ],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
        return executavel

    def gerar_e_compilar(self, gerador, gramatica, prefixo, *opcoes):
        subprocess.run(
            [str(gerador), str(gramatica), "-o", str(prefixo), *opcoes],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
        executavel = prefixo.with_suffix(
            ".exe" if sys.platform == "win32" else ""
        )
        subprocess.run(
            [
                COMPILADOR,
                "-ansi",
                "-pedantic",
                "-Wall",
                "-Wextra",
                "-o",
                str(executavel),
                str(prefixo.with_suffix(".c")),
            ],
            cwd=ROOT,
            check=True,
            capture_output=True,
            text=True,
        )
        return executavel

    def test_ast_eh_emitida_sem_pontuacao_literal(self):
        with tempfile.TemporaryDirectory() as nome:
            diretorio = Path(nome)
            gerador = self.compilar_gerador(diretorio)
            parser = self.gerar_e_compilar(
                gerador,
                GRAMATICAS / "expr.ebnf",
                diretorio / "expr",
                "--ast",
            )
            resultado = subprocess.run(
                [
                    str(parser),
                    "--ast",
                    str(ROOT / "testes" / "programas" / "expr" / "aceito_1.txt"),
                ],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            self.assertEqual(resultado.returncode, 0)
            self.assertIn("AST:\nexpr\n", resultado.stdout)
            self.assertIn("NUM '2'", resultado.stdout)
            self.assertNotIn("'+'", resultado.stdout)

    def test_recover_retorna_varios_erros(self):
        with tempfile.TemporaryDirectory() as nome:
            diretorio = Path(nome)
            gerador = self.compilar_gerador(diretorio)
            parser = self.gerar_e_compilar(
                gerador,
                GRAMATICAS / "expr.ebnf",
                diretorio / "expr_recover",
                "--recover",
            )
            entrada = diretorio / "erros.txt"
            entrada.write_text("2 + * ) +\n", encoding="ascii")
            resultado = subprocess.run(
                [str(parser), "--recover", str(entrada)],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            saida = resultado.stdout + resultado.stderr
            self.assertNotEqual(resultado.returncode, 0)
            self.assertIn("analise concluida com", saida)
            self.assertGreaterEqual(saida.count("esperado"), 2)

    def test_auto_conflicts_fatora_prefixo_comum(self):
        with tempfile.TemporaryDirectory() as nome:
            diretorio = Path(nome)
            gerador = self.compilar_gerador(diretorio)
            conflito = GRAMATICAS / "conflito_ll1.ebnf"
            sem_correcao = subprocess.run(
                [str(gerador), str(conflito), "-o", str(diretorio / "sem")],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            self.assertNotEqual(sem_correcao.returncode, 0)
            parser = self.gerar_e_compilar(
                gerador,
                conflito,
                diretorio / "com",
                "--auto-conflicts",
            )
            entrada = diretorio / "stmt.txt"
            entrada.write_text("a=2;\n", encoding="ascii")
            resultado = subprocess.run(
                [str(parser), str(entrada)],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            self.assertEqual(resultado.returncode, 0)
            self.assertIn("entrada aceita", resultado.stdout)

    def test_ebnf_descreve_uma_gramatica_valida(self):
        with tempfile.TemporaryDirectory() as nome:
            diretorio = Path(nome)
            gerador = self.compilar_gerador(diretorio)
            parser = self.gerar_e_compilar(
                gerador,
                GRAMATICAS / "ebnf.ebnf",
                diretorio / "ebnf",
                "--ast",
            )
            entrada = diretorio / "amostra.ebnf"
            entrada.write_text(
                "TOKENS\n"
                "  NUM = /[0-9]+/ ;\n"
                "  SKIP = /[ \\t\\r\\n]+/ ;\n"
                "RULES\n"
                "  expr = NUM { \"+\" NUM } ;\n",
                encoding="ascii",
            )
            resultado = subprocess.run(
                [str(parser), str(entrada)],
                cwd=ROOT,
                capture_output=True,
                text=True,
            )
            self.assertEqual(resultado.returncode, 0)
            self.assertIn("entrada aceita", resultado.stdout)


if __name__ == "__main__":
    unittest.main()
