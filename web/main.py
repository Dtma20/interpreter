import os
import subprocess
import time

import streamlit as st

MINIPAR_PATH = "../minipar/minipar"

def format_process_output(stdout):
    for line in stdout:
        yield "\n" + line
        time.sleep(0.5)

def exec_minipar(code: str):
    # Definindo o caminho do diretório e arquivo
    dir_path = os.path.join("../minipar/tmp")
    code_file = os.path.join(dir_path, "prog.minipar")

    # Criando o diretório se ele não existir
    os.makedirs(dir_path, exist_ok=True)

    # Escrevendo o código no arquivo
    with open(code_file, "w") as f:
        f.write(code)

    # Definindo o caminho para execução
    code_path = os.path.join("tmp", "prog.minipar")

    # Definindo o comando a ser executado
    cmd = ["./minipar", code_path]

    # Executando o processo
    process = subprocess.Popen(
        cmd,
        cwd=os.path.dirname(MINIPAR_PATH),  # Altere MINIPAR_PATH para o diretório correto
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )

    return process.stdout

def main():
    st.set_page_config(layout="wide")

    # Layout com duas colunas
    col1, col2 = st.columns(2)

    with col1:
        st.title("Editor de Código Minipar")
        code_input = st.text_area(label="Minipar Editor", label_visibility="collapsed", height=600, key="code_input")

        run_button = st.button("Executar")

    with col2:
        st.title("Saída")

        with st.container(border=True, height=600):
            st.empty()

            if run_button:
                st.write_stream(format_process_output(exec_minipar(code=code_input)))

main()


