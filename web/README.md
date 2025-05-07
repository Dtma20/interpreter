# Minipar Code Editor

Editor web interativo para escrever e executar código na linguagem **Minipar** em tempo real, com interface desenvolvida em [Streamlit](https://streamlit.io/).

## 🖥️ Funcionalidades

* Editor de código com suporte à linguagem Minipar
* Execução do código em tempo real
* Exibição progressiva da saída do interpretador
* Interface gráfica responsiva dividida em editor e terminal de saída

## 🚀 Como executar

### Pré-requisitos

* Python 3.7+
* [Streamlit](https://docs.streamlit.io/)
* Interpretador **Minipar** compilado em `../minipar/minipar`

### Instalação

1. Instale as dependências:

```bash
pip install streamlit
```

2. Execute a aplicação:

```bash
streamlit run app.py
```

> Certifique-se de que o binário `minipar` esteja compilado e localizado em `../minipar/minipar` a partir da raiz do projeto.

## 📁 Estrutura

```
project/
│
├── app.py               # Código principal da interface
├── ../minipar/          # Diretório contendo o interpretador compilado
│   └── minipar
└── ../minipar/tmp/      # Arquivos temporários de código Minipar
```

## 🛠️ Personalização

* O interpretador espera receber arquivos `.minipar` na pasta `tmp`, sendo criado dinamicamente pelo sistema.
* A saída é exibida linha a linha com pequeno atraso para simular execução em tempo real.

