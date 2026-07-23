import subprocess
import uuid
from pathlib import Path

import streamlit as st

# ── paths robustos (não dependem de CWD) ──────────────────────────────────────
MINIPAR_DIR = Path(__file__).resolve().parent.parent / "minipar"
MINIPAR_BIN = MINIPAR_DIR / "minipar"
TMP_DIR = MINIPAR_DIR / "tmp"

# ── padrões suspeitos de path traversal ──────────────────────────────────────
_FORBIDDEN = ["../", "..\\", "~/", "/etc/", "\\system32", "cmd.exe"]


def validate_code(code: str) -> tuple[bool, str]:
    """Retorna (ok, mensagem_de_erro)."""
    if not code or not code.strip():
        return False, "Código vazio"
    for pattern in _FORBIDDEN:
        if pattern.lower() in code.lower():
            return False, f"Path traversal detectado: '{pattern}'"
    return True, ""


def exec_minipar(code: str, user_input: str, timeout: int = 10) -> tuple[str, str]:
    """
    Executa código Minipar em subprocesso isolado.
    Retorna (stdout, stderr). Ambos strings.
    """
    TMP_DIR.mkdir(parents=True, exist_ok=True)

    filename = f"{uuid.uuid4().hex}.minipar"
    code_file = TMP_DIR / filename

    try:
        code_file.write_text(code, encoding="utf-8")

        result = subprocess.run(
            [str(MINIPAR_BIN), f"tmp/{filename}"],
            cwd=str(MINIPAR_DIR),
            input=(user_input + "\n") if user_input else "\n",
            capture_output=True,
            text=True,
            timeout=timeout,
        )

        stdout = result.stdout
        stderr = result.stderr
        if result.returncode != 0 and not stderr:
            stderr = f"Processo terminou com código {result.returncode}"
        return stdout, stderr

    except subprocess.TimeoutExpired:
        return "", "Tempo limite excedido (10s)"

    except FileNotFoundError:
        return "", f"Binário não encontrado: {MINIPAR_BIN}"

    except Exception as e:
        return "", f"Erro inesperado: {e}"

    finally:
        # Limpeza garantida do arquivo temporário
        try:
            if code_file.exists():
                code_file.unlink()
        except OSError:
            pass


def main() -> None:
    st.set_page_config(layout="wide")

    col1, col2 = st.columns(2)

    with col1:
        st.title("Editor de Código Minipar")
        code_input = st.text_area("Minipar Editor", height=400, key="code_input")
        user_input = st.text_input("Input para o programa", key="user_input")
        run_button = st.button("Executar")

    with col2:
        st.title("Saída")
        with st.container(border=True, height=600):
            if run_button:
                ok, err = validate_code(code_input)
                if not ok:
                    st.error(err)
                else:
                    with st.spinner("Executando..."):
                        stdout, stderr = exec_minipar(code_input, user_input)
                    if stdout:
                        st.text(stdout)
                    if stderr:
                        st.error(stderr)


if __name__ == "__main__":
    main()
