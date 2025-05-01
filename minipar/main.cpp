#include <iostream>
#include <fstream>
#include "./include/lexer.hpp"
#include "./include/parser.hpp"
#include "./include/semantic.hpp"
#include "./include/interpreter.hpp"
#include "./include/error.hpp"

int main(int argc, char* argv[]) {
    std::string source_code;
    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file.is_open()) {
            std::cerr << "Erro: Não foi possível abrir o arquivo " << argv[1] << std::endl;
            return 1;
        }
        source_code.assign(
            std::istreambuf_iterator<char>(file),
            std::istreambuf_iterator<char>()
        );
        file.close();
    } else {
        source_code.assign(
            std::istreambuf_iterator<char>(std::cin),
            std::istreambuf_iterator<char>()
        );
    }

    try {
        // 1) Lexer → tokens
        Lexer lexer(source_code);
        auto tokens = lexer.scan();

        // 2) Parser → AST
        Parser parser(std::move(tokens));
        auto ast = parser.start();

        // 3) Semantic → checa tipos e declarações
        SemanticAnalyzer semantic;
        semantic.visit(ast.get());

        // 4) Interpreter → executa o programa
        Interpreter interpreter;
        interpreter.execute(ast.get());

    } catch (const SyntaxError& e) {
        std::cerr << "Erro de sintaxe: " << e.what() << std::endl;
        return 1;
    } catch (const SemanticError& e) {
        std::cerr << "Erro semântico: " << e.what() << std::endl;
        return 1;
    } catch (const RunTimeError& e) {
        std::cerr << "Erro em tempo de execução: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Erro inesperado: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
