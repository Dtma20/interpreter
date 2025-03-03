# Minipar

**Minipar** é um interpretador para uma linguagem de programação simples escrita em C++17. Ele suporta atribuições de variáveis, funções definidas pelo usuário, controle de fluxo (`if`, `while`, `break`, `continue`), chamadas de função embutidas (como `print`), e estruturas paralelas/sequenciais (`par`, `seq`). O projeto foi desenvolvido como parte de um estudo de compiladores na UFAL e pode ser executado com entrada de arquivos ou diretamente no terminal.

## Funcionalidades

- **Declaração de Variáveis**: Suporta variáveis tipadas (ex.: `x: number = 42`).
- **Funções**: Permite definir funções recursivas ou iterativas (ex.: `func fib(n: number) -> number { ... }`).
- **Controle de Fluxo**: Inclui `if`, `while`, `break`, e `continue`.
- **Expressões**: Suporta operações aritméticas (`+`, `-`, `*`, `/`), relacionais (`<`, `>`, `<=`, `>=`, `==`, `!=`), e unárias (`-`, `!`).
- **Paralelismo**: Estrutura `par` para execução paralela básica usando threads.
- **Entrada/Saída**: Função embutida `print` para saída no console.

## Estrutura do Projeto

```
minipar/
├── include/         # Cabeçalhos (.hpp)
│   ├── ast.hpp
│   ├── error.hpp
│   ├── interpreter.hpp
│   ├── lexer.hpp
│   ├── parser.hpp
│   ├── semantic.hpp
│   ├── symtable.hpp
│   └── token.hpp
├── src/            # Implementações (.cpp)
│   ├── ast.cpp
│   ├── error.cpp
│   ├── interpreter.cpp
│   ├── lexer.cpp
│   ├── parser.cpp
│   ├── semantic.cpp
│   └── token.cpp
├── main.cpp        # Ponto de entrada
├── tests/          # Arquivos de teste
│   ├── fib.minipar
│   └── fib2.minipar
└── README.md       # Este arquivo
```

## Requisitos

- **Compilador**: GCC ou outro compatível com C++17.
- **Bibliotecas**: STL (Standard Template Library), incluindo suporte a `pthread` para threads.
- **Sistema Operacional**: Testado em Linux (Ubuntu), mas deve ser compatível com outros sistemas POSIX.

## Instalação

1. **Clone o Repositório** (se aplicável):
   ```bash
   git clone <URL_DO_REPOSITORIO>
   cd minipar
   ```
   Ou copie os arquivos para um diretório local.

2. **Compile o Projeto**:
   ```bash
   g++ -g -o programa main.cpp src/lexer.cpp src/parser.cpp src/semantic.cpp src/token.cpp src/error.cpp src/ast.cpp src/interpreter.cpp -std=c++17 -fPIC -pthread
   ```
   - `-g`: Inclui informações de depuração.
   - `-std=c++17`: Usa o padrão C++17.
   - `-pthread`: Habilita suporte a threads.
   - `-fPIC`: Gera código independente de posição.

3. **Verifique o Executável**:
   Após a compilação, você terá um executável chamado `programa` no diretório raiz.

## Uso

### **Execução com Arquivo**
Crie um arquivo de entrada (ex.: `tests/meu_codigo.minipar`) com o código na linguagem Minipar. Por exemplo:
```
x: number = 42
print(x)
i: number = 0
while (i < 3) {
    print(i)
    i = i + 1
}
```

Execute:
```bash
./programa tests/meu_codigo.minipar
```

**Saída Esperada**:
```
42
0
1
2
Execução concluída sem erros.
```

### **Execução Interativa**
Execute sem argumentos para digitar o código manualmente:
```bash
./programa
Digite o código fonte (termine com Ctrl+D ou Ctrl+Z):
x: number = 10
print(x)
```
Pressione `Ctrl+D` (Linux) ou `Ctrl+Z` (Windows).

**Saída Esperada**:
```
10
Execução concluída sem erros.
```

### **Depuração**
Use `gdb` para depurar:
```bash
gdb ./programa
run tests/meu_codigo.minipar
```

## Exemplos

### **Fibonacci Iterativo**
Arquivo: `tests/fib.minipar`
```python
n: number = 10
a: number = 0
b: number = 1
i: number = 0
print(a)
print(b)
while (i < n - 2) {
    c: number = a + b
    print(c)
    a = b
    b = c
    i = i + 1
}
```

### **Fibonacci Recursivo**
Arquivo: `tests/fib2.minipar`
```python
func fib(n: number) -> number {
    if (n <= 0) {
        return 0
    }
    if (n == 1) {
        return 1
    }
    return fib(n - 1) + fib(n - 2)
}
x: number = fib(6)
print(x)
```