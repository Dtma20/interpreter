import os
import subprocess
import time
import streamlit as st

MINIPAR_PATH = "../minipar/minipar"


def format_process_output(stdout):
    for line in stdout:
        yield "\n" + line


def exec_minipar_with_input(code: str, user_input: str):
    dir_path = os.path.join("../minipar/tmp")
    code_file = os.path.join(dir_path, "prog.minipar")

    os.makedirs(dir_path, exist_ok=True)

    with open(code_file, "w") as f:
        f.write(code)

    code_path = os.path.join("tmp", "prog.minipar")
    cmd = ["./minipar", code_path]

    process = subprocess.Popen(
        cmd,
        cwd=os.path.dirname(MINIPAR_PATH),
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.PIPE,
        text=True
    )

    # Agora o processo continuará rodando e podemos enviar entradas a ele
    # Enquanto o processo está em execução, vamos enviar o input do usuário

    # Envia o input do usuário
    process.stdin.write(user_input + "\n")
    process.stdin.flush()

    # Lê a saída do processo em tempo real
    stdout = process.stdout

    return stdout


def main():
    st.set_page_config(layout="wide")

    col1, col2 = st.columns(2)

    with col1:
        st.title("Editor de Código Minipar")
        code_input = st.text_area(
            "Minipar Editor", height=400, key="code_input")
        user_input = st.text_input("Input para o programa", key="user_input")
        run_button = st.button("Executar")

    with col2:
        st.title("Saída")
        with st.container(border=True, height=600):
            if run_button:
                # Pega o stream de saída em tempo real
                process_output = exec_minipar_with_input(
                    code_input, user_input)

                # Exibe a saída em tempo real
                for line in format_process_output(process_output):
                    st.write(line)


main()
