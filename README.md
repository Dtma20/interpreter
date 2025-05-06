# Minipar: Linguagem de Programação Minimalista

Minipar é uma linguagem de programação minimalista desenvolvida para oferecer suporte a estruturas básicas de controle, paralelismo e comunicação por canais. Com uma sintaxe simples e expressiva, o projeto inclui um analisador léxico, sintático, semântico e um interpretador escrito em C++.

---

## Estrutura do Projeto

A organização dos arquivos do Minipar segue uma estrutura clara e modular, facilitando a manutenção e a expansão:

```
├── exemplos
│   ├── bubble_sort.minipar
│   ├── client.minipar
│   ├── fat.minipar
│   ├── fat_rec.minipar
│   ├── fib.minipar
│   ├── fib2.minipar
│   ├── mochila.minipar
│   ├── par.minipar
│   ├── quick_sort.minipar
│   ├── recomendacao.minipar
│   ├── rede_tres_neuronios.minipar
│   ├── rede_um_neuronio.minipar
│   └── server.minipar
├── gramatica
│   └── bnf.txt
├── include
│   ├── ast.hpp
│   ├── debug.hpp
│   ├── error.hpp
│   ├── interpreter.hpp
│   ├── lexer.hpp
│   ├── parser.hpp
│   ├── semantic.hpp
│   ├── symtable.hpp
│   └── token.hpp
├── main.cpp
├── makefile
├── minipar
├── minipar-extension
│   ├── language-configuration.json
│   ├── minipar-extension-0.0.1.vsix
│   ├── package.json
│   ├── syntaxes
│   │   └── minipar.tmLanguage.json
│   └── themes
│       └── minipar-theme.json
└── src
    ├── ast.cpp
    ├── error.cpp
    ├── interpreter.cpp
    ├── lexer.cpp
    ├── parser.cpp
    ├── semantic.cpp
    └── token.cpp
```

---

## Instalação e Compilação

Para compilar e executar programas escritos em Minipar, siga os passos abaixo:

1. **Pré-requisitos:**

   * Compilador C++ com suporte a C++17 (por exemplo, `g++`).
   * Biblioteca padrão do C++ instalada.

2. **Compilação:**
   No diretório raiz do projeto, utilize o `makefile`:

   * **Compilação padrão:**

     ```bash
     make run
     ```

     Esse comando gera o executável `minipar` usando:

     ```bash
     g++ -g -o minipar main.cpp src/lexer.cpp src/parser.cpp src/semantic.cpp src/token.cpp src/error.cpp src/ast.cpp src/interpreter.cpp -std=c++17 -fPIC
     ```
   * **Modo de Depuração:**

     ```bash
     make debug
     ```

     Adiciona a flag `-DDEBUG_MODE` para logs detalhados:

     ```bash
     g++ -DDEBUG_MODE -g -o minipar main.cpp src/lexer.cpp src/parser.cpp src/semantic.cpp src/token.cpp src/error.cpp src/ast.cpp src/interpreter.cpp -std=c++17 -fPIC
     ```

3. **Execução:**
   Após compilar, execute o interpretador apontando para um exemplo na pasta `exemplos/`:

   ```bash
   ./minipar exemplos/fib.minipar
   ```

   O interpretador processará o código, exibindo a saída ou mensagens de erro no console.

---

## Funcionalidades Principais

Minipar oferece um conjunto rico de recursos para facilitar o desenvolvimento de algoritmos e provas de conceito em diferentes paradigmas:

* **Tipos de Dados Básicos**

  * `num`: números reais com operações aritméticas e comparações.
  * `string`: manipulação, concatenação e funções auxiliares (`to_string`, `len`).
  * `bool`: operações lógicas (`&&`, `||`, `!`).

* **Arrays**

  * Declaração estática e inicialização: `arr : array[5] = [1, 2, 3, 4, 5]`.
  * Acesso e atribuição por índice, função `len` e suporte a arrays multidimensionais (`array[N][M]`).

* **Controle de Fluxo**

  * Condicionais: `if … else`.
  * Laços: `while` e `for`, com suporte a `break` e `continue`.

* **Funções e Escopo**

  * Declaração de funções com `func nome(args) { … }` e `return`.
  * Escopo léxico e passagem de parâmetros por valor ou referência.

* **Paralelismo e Sequenciamento**

  * `par` para executar blocos em threads paralelas.
  * `seq` para forçar execução sequencial.

* **Comunicação via Canais TCP**

  * Cliente (`c_channel`) e servidor (`s_channel`) para troca de mensagens bidirecionais.
  * Métodos nativos: `send`, `close`.

* **Funções Matemáticas e Probabilísticas**

  * `exp(x)`, `randf(a, b)`, `randi(a, b)`.

* **Algoritmos Clássicos e Exemplos Avançados**

  * Ordenação: `bubble_sort.minipar`, `quick_sort.minipar`.
  * Programação Dinâmica: `mochila.minipar`.
  * Recursão: `fib.minipar`, `fib2.minipar`, `fat.minipar`, `fat_rec.minipar`.
  * Redes Neurais: `rede_um_neuronio.minipar`, `rede_tres_neuronios.minipar`.

---

## Exemplos de Programas Disponíveis

Veja na pasta `exemplos/` diversos casos de uso:

1. **Ordenação e Estruturas de Dados:** `bubble_sort`, `quick_sort`, `mochila`
2. **Sequências Numéricas:** `fib`, `fib2`, `fat`, `fat_rec`
3. **Paralelismo:** `par`
4. **Comunicação Cliente-Servidor:** `client`, `server`
5. **Sistema de Recomendação Simples:** `recomendacao`
6. **Redes Neurais:** `rede_um_neuronio`, `rede_tres_neuronios`

---

## Manual de Referência (Resumo)

**Lexical:** identificadores, palavras-chave, comentários.
**Tipos:** num, string, bool, void, array.
**Operadores:** aritméticos, relacionais, lógicos, unários e precedência.
**Estruturas:** condicionais, laços, funções, `par`, `seq`, canais.
