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
  - `fib.minipar` (Fibonacci Iterativo)
  - `fib2.minipar` (Fibonacci Recursivo)
  - `mochila.minipar` (Problema da Mochila Binária)
  - `nn.minipar` (Treinamento de Neurônio)
  - `server.minipar` (Calculadora via Canal)
  
- **main.cpp**  
  Arquivo principal que integra os componentes e inicia o interpretador.

---

## Instalação e Compilação

Para compilar e executar programas escritos em Minipar, siga os passos abaixo:

1. **Pré-requisitos:**  
   - Compilador C++ com suporte a C++17 (por exemplo, `g++`).
   - Biblioteca padrão do C++ instalada.

2. **Compilação:**  
   No diretório `build/`, utilize o *Makefile*:
   - **Compilação padrão:**  
     ```bash
     make run
     ```  
     Esse comando gera o executável `programa` usando:
     ```bash
     g++ -g -o programa ../main.cpp ../src/lexer.cpp ../src/parser.cpp ../src/semantic.cpp ../src/token.cpp ../src/error.cpp ../src/ast.cpp ../src/interpreter.cpp -std=c++17 -fPIC
     ```
   - **Modo de Depuração:**  
     ```bash
     make debug
     ```  
     Adiciona a flag `-DDEBUG_MODE` para logs detalhados:
     ```bash
     g++ -DDEBUG_MODE -g -o programa ../main.cpp ../src/lexer.cpp ../src/parser.cpp ../src/semantic.cpp ../src/token.cpp ../src/error.cpp ../src/ast.cpp ../src/interpreter.cpp -std=c++17 -fPIC
     ```

3. **Execução:**  
   Execute o interpretador passando um arquivo fonte como argumento:
   ```bash
   ./build/programa exemplos/fib.minipar
   ```
   O interpretador processará o código, exibindo a saída ou mensagens de erro no console.

---

## Exemplos de Programas

Os exemplos incluídos demonstram diversas funcionalidades do Minipar:

1. **Problema da Mochila Binária (`exemplos/mochila.minipar`):**  
   Utiliza arrays, laços `for` e funções para resolver o problema via programação dinâmica.  
   - **Saída:** `15`

2. **Fibonacci Iterativo (`exemplos/fib.minipar`):**  
   Calcula a sequência de Fibonacci de forma iterativa.  
   - **Saída:** `8` (Sequência: 0, 1, 1, 2, 3, 5, 8)

3. **Treinamento de Neurônio (`exemplos/nn.minipar`):**  
   Implementa um perceptron simples com ajuste de pesos.  
   - **Saída parcial:** Exibe iterações com ajustes até o treinamento ser concluído.

4. **Calculadora via Canal (`exemplos/server.minipar`):**  
   Cria um servidor que processa expressões enviadas por um cliente.  
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
  `func`, `if`, `while`, `return`, `break`, `continue`, `par`, `seq`, `c_channel`, `s_channel`, `for`, entre outras.

- **Comentários:**  
  - Linha única: `# comentário`
  - Multi-linha: `/* comentário */`

- **Espaços e Quebras de Linha:**  
  Geralmente ignorados, exceto para delimitar tokens.

---

### 2. Tipos de Dados

Minipar suporta os seguintes tipos:
- **num:** Números reais (ex.: `5`, `3.14`).
- **string:** Cadeias de caracteres delimitadas por aspas (ex.: `"hello"`).
- **bool:** Valores booleanos (`true`, `false`).
- **void:** Tipo de retorno para funções sem valor retornado.
- **array:** Coleções ordenadas de elementos do mesmo tipo (ex.: `x: array[10]`).

---

### 3. Estruturas e Comandos

#### Declaração de Variáveis e Funções

```go
x : num = 42
str : string = "texto"
arr : array[5] = [1, 2, 3, 4, 5]
```

#### Operações com Arrays

Arrays permitem acesso e manipulação de elementos via índices:
```go
arr : array[5] = [1, 2, 3, 4, 5]
print(arr[2])  # Exibe 3
arr[2] = 10    # Altera o elemento no índice 2 para 10
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

- **Laços:**
  - **While:** Para repetições com condição.
  - **For:** Para iterações com contadores.

- **Comandos de Controle de Fluxo:**  
  `break` e `continue` manipulam iterações.

#### Estruturas de Paralelismo e Sequenciamento

- **Paralelismo (`par`):** Executa instruções em threads separadas.
  ```go
  par {
      print("tarefa 1")
      print("tarefa 2")
  }
  ```

- **Sequenciamento Explícito (`seq`):** Executa instruções em ordem.
  ```go
  seq {
      print("passo 1")
      print("passo 2")
  }
  ```

#### Comunicação via Canais

- **CChannel (Cliente):**
  ```go
  c_channel meu_canal { "localhost", 8080 }
  ```
- **SChannel (Servidor):**
  Veja exemplo em `exemplos/server.minipar`.

- **Exemplo de Uso:**
  - **Servidor:**
    ```go
    s_channel servidor { 8080 }
    print("Aguardando conexão...")
    # Processa mensagens recebidas
    ```
  - **Cliente:**
    ```go
    c_channel cliente { "localhost", 8080 }
    cliente.send("2 + 3")
    ```
  *Nota:* Canais utilizam sockets TCP para comunicação.

---

### 4. Expressões e Operadores

#### Operadores Aritméticos
`+`, `-`, `*`, `/`

#### Operadores Relacionais
`<`, `>`, `<=`, `>=`, `==`, `!=`

#### Operadores Lógicos
`&&` (AND), `||` (OR), `!` (NOT)

#### Operadores Unários
- `++`: Incrementa em 1 (ex.: `x++`).
- `--`: Decrementa em 1 (ex.: `x--`).
- `!`: Nega valor booleano (ex.: `!true` → `false`).
- `-`: Nega valor numérico (ex.: `-5`).

#### Ordem de Precedência
1. Unários: `!`, `-`, `++`, `--`
2. `*`, `/`
3. `+`, `-`
4. `<`, `>`, `<=`, `>=`
5. `==`, `!=`
6. `&&`, `||`
7. Parênteses alteram precedência.

---

### 5. Biblioteca Padrão

- **Entrada e Saída:**  
  - `print(valor)`: Exibe o valor no console.

- **Conversão de Tipos:**  
  - `to_num(str)`: Converte string em número (ex.: `"42" → 42`).
  - `to_string(valor)`: Converte valor em string.

- **Manipulação de Strings e Arrays:**  
  - `len(valor)`: Retorna tamanho de string ou array.
  - `isalpha(str)`: Verifica se contém apenas letras.
  - `isnum(str)`: Verifica se é um número válido.
