int main()
{
    int x = x+1;


}

/* CASOS DE TESTE: Inteiros negativos vs. Operador de Subtracao (MINUS) */

/* 1. Numeros negativos literais (apos atribuicao, inicio ou separadores) */
x = -42;
y = -1;
func(-10, -20);
int vet[3] = {-5, -15, -25};

/* 2. Subtracao com e sem espacos (deve gerar MINUS seguido de INTEGERCONST positivo) */
a = b - 5;
a = b-5;
total = 100 - 50;
total = 100-50;

/* 3. Subtracao apos fechar parenteses ou colchetes */
res = (x + y) - 10;
res = (x + y)-10;
item = vet[0] - 2;
item = vet[0]-2;

/* 4. Subtracao de numero negativo (deve gerar MINUS e depois INTEGERCONST negativo) */
diff = x - -5;
diff = x - - 5;

/* 5. Subtracao sequencial */
calc = 10 - 5 - 2;
calc = 10-5-2;


/* CASOS DE TESTE: Constantes de Caractere (CHARCONST) e Erros (UNDEF) */

char c1 = 'a';
char c2 = 'Z';
char c3 = '9';
char c4 = '+';
char c5 = ';';
char c6 = ' ';
char nl = '\n';
char tab = '\t';
char barra = '\\';
char aspa = '\'';
char aspa_dupla = '\"';
char zero = '\0';
char err1 = 'a;
char err2 = '\n;
char err3 = ';

/* Casos em expressoes e vetores */
char v[2] = {'x', 'y'};
if (c1 == 'a') {
    return '1';
}


int main()
{
    // Strings validas
    print "String normal";
    print "Escapes: \n \t \\ \"";
    print "";

    // Erro: escape de caractere nulo
    print "String contem nulo: \0";

"blablabla\0blabla"

print "String nao terminada

print "EOF em string

    @
#
$
/* Comentario de bloco
      em multiplas linhas
      com tokens falsos dentro: int y = 20; */


int x@y;


*/

/* EOF em comentario */


/* Comentario que nunca fecha