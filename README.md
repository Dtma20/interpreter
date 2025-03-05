# Minipar: Linguagem de Programação Minimalista

Minipar é uma linguagem de programação minimalista desenvolvida para oferecer suporte a estruturas básicas de controle, paralelismo e comunicação por canais. Com uma sintaxe simples e expressiva, o projeto inclui um analisador léxico, sintático, semântico e um interpretador escrito em C++.

---

## Estrutura do Projeto

A organização dos arquivos do Minipar segue uma estrutura clara e modular, facilitando a manutenção e a expansão:

- **build/**  
  Contém o *Makefile*, que automatiza o processo de compilação.
  
- **include/**  
  Arquivos de cabeçalho (`.hpp`) com as definições de classes e interfaces, incluindo:
  - `ast.hpp`
  - `debug.hpp`
  - `error.hpp`
  - `interpreter.hpp`
  - `lexer.hpp`
  - `parser.hpp`
  - `semantic.hpp`
  - `symtable.hpp`
  - `token.hpp`
  
- **src/**  
  Implementações em C++ correspondentes aos arquivos de cabeçalho:
  - `ast.cpp`
  - `error.cpp`
  - `interpreter.cpp`
  - `lexer.cpp`
  - `parser.cpp`
  - `semantic.cpp`
  - `token.cpp`
  
- **exemplos/**  
  Exemplos de programas escritos em Minipar, que demonstram diversas funcionalidades da linguagem:
  - `fib.minipar` (Fibonacci Recursivo)
  - `fib2.minipar` (Fibonacci Iterativo)
  - `mochila.minipar` (Problema da Mochila Binária)
  - `nn.minipar` (Treinamento de Neurônio)
  - `server.minipar` (Calculadora via Canal)
  
- **main.cpp**  
  Arquivo principal que integra os componentes e inicia o interpretador.
  
- **README.md**  
  Documentação e instruções gerais do projeto.

---

## Instalação e Compilação

Para compilar e executar programas escritos em Minipar, siga os passos abaixo:

1. **Pré-requisitos:**  
   - Compilador C++ com suporte a C++17 (por exemplo, `g++`).
   - Biblioteca padrão do C++ instalada.

2. **Compilação:**  
   No diretório `build/`, utilize o *Makefile*:
   - **Compilação padrão:**  
     ```go
     make run
     ```  
     Esse comando gera o executável `programa` usando o seguinte comando de compilação:
     ```go
     g++ -g -o programa ../main.cpp ../src/lexer.cpp ../src/parser.cpp ../src/semantic.cpp ../src/token.cpp ../src/error.cpp ../src/ast.cpp ../src/interpreter.cpp -std=c++17 -fPIC
     ```
   - **Modo de Depuração:**  
     ```go
     make debug
     ```  
     Essa opção adiciona a flag `-DDEBUG_MODE` para ativar logs de depuração:
     ```go
     g++ -DDEBUG_MODE -g -o programa ../main.cpp ../src/lexer.cpp ../src/parser.cpp ../src/semantic.cpp ../src/token.cpp ../src/error.cpp ../src/ast.cpp ../src/interpreter.cpp -std=c++17 -fPIC
     ```

3. **Execução:**  
   Após a compilação, execute o interpretador passando um arquivo fonte como argumento, por exemplo:
   ```go
   ./build/programa exemplos/fib.minipar
   ```
   O interpretador processará o código, exibindo a saída ou mensagens de erro no console.

---

## Exemplos de Programas

Os exemplos incluídos no projeto demonstram diversas funcionalidades do Minipar:

1. **Problema da Mochila Binária (`exemplos/mochila.minipar`):**  
   Utiliza array, laços `for` e funções para resolver o problema da mochila via programação dinâmica.
   - **Saída:** `15`

2. **Fibonacci Iterativo (`exemplos/fib.minipar`):**  
   Calcula a sequência de Fibonacci de forma iterativa.
   - **Saída:** `8` (Sequência: 0, 1, 1, 2, 3, 5, 8)

3. **Treinamento de Neurônio (`exemplos/nn.minipar`):**  
   Implementa um perceptron simples com ajuste de pesos, iterando até atingir o valor desejado.
   - **Saída parcial:** Exibe as iterações com ajustes de peso até o treinamento ser concluído.

4. **Calculadora via Canal (`exemplos/server.minipar`):**  
   Cria um servidor que processa expressões matemáticas enviadas por um cliente, evitando estruturas `else if` aninhadas.
   - **Saída:** Inicializa um servidor que responde a expressões como `"2 + 3"`.

5. **Fibonacci Recursivo (`exemplos/fib2.minipar`):**  
   Utiliza recursão para calcular a sequência de Fibonacci.
   - **Saída:** `8`

---

## Manual de Referência da Linguagem

### 1. Convenções Léxicas

- **Identificadores:**  
  Iniciam com letras ou `_`, seguidos por letras, números ou `_` (ex.: `x`, `minha_variavel`).

- **Palavras-chave:**  
  Incluem `func`, `if`, `while`, `return`, `break`, `continue`, `par`, `seq`, `c_channel`, `s_channel`, `for`, entre outras.

- **Comentários:**  
  - Linha única: `# comentário`
  - Multi-linha: `/* comentário */`

- **Espaços e Quebras de Linha:**  
  São geralmente ignorados, exceto para delimitar tokens.

---

### 2. Tipos de Dados

Minipar suporta os seguintes tipos:
- **number:** Números reais (ex.: `5`, `3.14`).
- **string:** Cadeias de caracteres delimitadas por aspas (ex.: `"hello"`).
- **bool:** Valores booleanos (`true`, `false`).
- **void:** Tipo de retorno para funções que não retornam valor.
- **mochila:** Coleções ordenadas de elementos do mesmo tipo (ex.: `x: array[10]`).

---

### 3. Estruturas e Comandos

#### Declaração de Variáveis e Funções

```go
x : number = 42
str : string = "texto"
arr : array[5] = [1, 2, 3, 4, 5]
```

#### Estruturas de Controle

- **Condicionais (`if/else`):**
  ```go
  if (x > 0) {
      print("positivo")
  } else {
      print("negativo ou zero")
  }
  ```
  *Nota:* Para condições múltiplas, utilize blocos aninhados:
  ```go
  if (operator == "+") {
      result = result + valor_num
  } else {
      if (operator == "-") {
          result = result - valor_num
      } else {
          print("outro caso")
      }
  }
  ```

- **Laços:**
  - **While:**  
    Utilizado para repetições com condição.
  - **For:**  
    Comumente usado em iterações com contadores.

- **Comandos de Controle de Fluxo:**  
  `break` e `continue` para manipulação de iterações.

#### Estruturas de Paralelismo e Sequenciamento

- **Paralelismo (`par`):**
  ```go
  par {
      print("tarefa 1")
      print("tarefa 2")
  }
  ```
  Executa as instruções em threads separadas.

- **Sequenciamento Explícito (`seq`):**
  ```go
  seq {
      print("passo 1")
      print("passo 2")
  }
  ```

#### Comunicação via Canais

- **CChannel (Canal Cliente):**
  ```go
  c_channel meu_canal { "localhost", 8080 }
  ```
- **SChannel (Canal Servidor):**
  Exemplo presente em `exemplos/server.minipar`.

---

### 4. Expressões e Operadores

#### Operadores Aritméticos
`+`, `-`, `*`, `/`

#### Operadores Relacionais
`<`, `>`, `<=`, `>=`, `==`, `!=`

#### Operadores Lógicos
`&&` (AND), `||` (OR), `!` (NOT)

#### Ordem de Precedência
1. Operadores unários: `!`, `-`
2. Multiplicação e divisão: `*`, `/`
3. Adição e subtração: `+`, `-`
4. Operadores relacionais: `<`, `>`, `<=`, `>=`
5. Igualdade: `==`, `!=`
6. Lógicos: `&&` e `||`
7. Uso de parênteses para alterar a precedência.

#### Chamadas de Função
Exemplos em `exemplos/fib.minipar`, `exemplos/nn.minipar` e outros.

---

### 5. Biblioteca Padrão

- **Entrada e Saída:**  
  `print(valor)` para exibição de mensagens.

- **Conversão de Tipos:**  
  `to_number(str)` e `to_string(valor)`

- **Manipulação de Strings:**  
  `len(valor)`, `isalpha(str)` e `isnum(str)`

---