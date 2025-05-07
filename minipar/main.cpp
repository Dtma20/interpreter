#include <iostream>
#include <fstream>
#include "./include/lexer.hpp"
#include "./include/error.hpp"
#include "./include/parser.hpp"
#include "./include/semantic.hpp"
#include "./include/interpreter.hpp"

int main(int argc, char* argv[]) {
    
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(NULL);
    std::cout.tie(NULL);

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
        Lexer lexer(source_code);
        auto tokens = lexer.scan();

        Parser parser(std::move(tokens));
        auto ast = parser.start();

        SemanticAnalyzer semantic;
        semantic.visit(ast.get());

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
