#include "../include/interpreter.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

/**
 * @brief Construtor da classe Interpreter.
 *
 * Inicializa as flags de controle (break, continue e return) e empilha o escopo inicial.
 */
Interpreter::Interpreter() : break_flag(false), continue_flag(false), return_flag(false) {
    push_scope();
}

/**
 * @brief Cria um novo escopo.
 *
 * Empilha um novo objeto Scope no vetor de escopos para isolar variáveis.
 */
void Interpreter::push_scope() {
    scopes.push_back(Scope{});
}

/**
 * @brief Remove o escopo atual.
 *
 * Desempilha o último escopo do vetor, se houver algum.
 */
void Interpreter::pop_scope() {
    if (!scopes.empty()) {
        scopes.pop_back();
    }
}

/**
 * @brief Verifica se um valor é considerado verdadeiro.
 *
 * Determina a veracidade de um Value, levando em conta seu tipo (bool, double ou string).
 *
 * @param value Valor a ser avaliado.
 * @return true se o valor for verdadeiro; false caso contrário.
 */
bool Interpreter::is_true(const Value& value) {
    if (std::holds_alternative<bool>(value)) {
        return std::get<bool>(value);
    } else if (std::holds_alternative<double>(value)) {
        return std::get<double>(value) != 0.0;
    } else if (std::holds_alternative<std::string>(value)) {
        return !std::get<std::string>(value).empty();
    }
    return false;
}

/**
 * @brief Avalia uma expressão da AST e retorna seu valor.
 *
 * A função trata diversos tipos de expressões, como constantes, identificadores,
 * acessos, chamadas de funções, operações relacionais, aritméticas, unárias e lógicas.
 *
 * @param expr Ponteiro para a expressão a ser avaliada.
 * @return Valor resultante da avaliação.
 * @throws RunTimeError se a expressão for nula ou se ocorrer algum erro na avaliação.
 */
Value Interpreter::evaluate(Expression* expr) {
    if (!expr) {
        throw RunTimeError("Tentativa de avaliar expressão nula");
    }
    if (auto* constant = dynamic_cast<Constant*>(expr)) {
        std::string value = constant->getToken().getValue();
        std::string type = constant->getType();
        if (type == "NUMBER") return std::stod(value);
        else if (type == "STRING") return value;
        else if (type == "BOOL") return value == "true";
        throw RunTimeError("Tipo de constante não suportado: " + type);
    } else if (auto* id = dynamic_cast<ID*>(expr)) {
        std::string var_name = id->getToken().getValue();
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto var_it = it->variables.find(var_name);
            if (var_it != it->variables.end()) {
                return var_it->second;
            }
        }
        throw RunTimeError("Variável não definida: " + var_name);
    } else if (auto* access = dynamic_cast<Access*>(expr)) {
        Value base_val = evaluate(access->getId());
        Value index_val = evaluate(access->getExpr());
        if (std::holds_alternative<std::string>(base_val) && std::holds_alternative<double>(index_val)) {
            std::string str = std::get<std::string>(base_val);
            int index = static_cast<int>(std::get<double>(index_val));
            if (index >= 0 && index < str.length()) {
                return std::string(1, str[index]);
            }
            throw RunTimeError("Índice fora do intervalo: " + std::to_string(index));
        }
        throw RunTimeError("Acesso a elemento requer string e índice numérico");
    } else if (auto* call = dynamic_cast<Call*>(expr)) {
        std::string func_name = call->getId()->getToken().getValue();
        if (func_name == "print") {
            for (const auto& arg : call->getArgs()) {
                Value value = evaluate(arg.get());
                std::visit([](auto&& val) { std::cout << val << std::endl; }, value);
            }
            return std::string("");
        } else if (func_name == "len") {
            Value arg = evaluate(call->getArgs()[0].get());
            if (std::holds_alternative<std::string>(arg)) {
                return static_cast<double>(std::get<std::string>(arg).length());
            }
            throw RunTimeError("len requer uma string como argumento");
        } else if (func_name == "to_number") {
            Value arg = evaluate(call->getArgs()[0].get());
            if (std::holds_alternative<std::string>(arg)) {
                return std::stod(std::get<std::string>(arg));
            }
            throw RunTimeError("to_number requer uma string como argumento");
        } else if (func_name == "to_string") {
            Value arg = evaluate(call->getArgs()[0].get());
            return std::visit([](auto&& val) -> std::string {
                if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::string>) {
                    return val;
                } else {
                    return std::to_string(val);
                }
            }, arg);
        } else if (func_name == "isnum") {
            Value arg = evaluate(call->getArgs()[0].get());
            if (std::holds_alternative<std::string>(arg)) {
                std::string str = std::get<std::string>(arg);
                return std::all_of(str.begin(), str.end(), ::isdigit);
            }
            return false;
        } else if (func_name == "isalpha") {
            Value arg = evaluate(call->getArgs()[0].get());
            if (std::holds_alternative<std::string>(arg)) {
                std::string str = std::get<std::string>(arg);
                return std::all_of(str.begin(), str.end(), ::isalpha);
            }
            return false;
        } else if (functions.find(func_name) != functions.end()) {
            return execute_function(functions[func_name], call->getArgs());
        }
        throw RunTimeError("Função não suportada: " + func_name);
    } else if (auto* relational = dynamic_cast<Relational*>(expr)) {
        Value left_value = evaluate(relational->getLeft());
        Value right_value = evaluate(relational->getRight());
        std::string op = relational->getToken().getTag();

        if (std::holds_alternative<double>(left_value) && std::holds_alternative<double>(right_value)) {
            double left_num = std::get<double>(left_value);
            double right_num = std::get<double>(right_value);
            if (op == "<") return left_num < right_num;
            if (op == ">") return left_num > right_num;
            if (op == "LTE") return left_num <= right_num;
            if (op == "GTE") return left_num >= right_num;
            if (op == "EQ") return left_num == right_num;
            if (op == "NEQ") return left_num != right_num;
        }
        else if (std::holds_alternative<std::string>(left_value) && std::holds_alternative<std::string>(right_value)) {
            std::string left_str = std::get<std::string>(left_value);
            std::string right_str = std::get<std::string>(right_value);
            if (op == "EQ") return left_str == right_str;
            if (op == "NEQ") return left_str != right_str;
        }
        throw RunTimeError("Operador relacional '" + op + "' requer operandos numéricos ou strings compatíveis");
    } else if (auto* arithmetic = dynamic_cast<Arithmetic*>(expr)) {
        Value left_value = evaluate(arithmetic->getLeft());
        Value right_value = evaluate(arithmetic->getRight());
        std::string op = arithmetic->getToken().getTag();

        if (std::holds_alternative<double>(left_value) && std::holds_alternative<double>(right_value)) {
            double left_num = std::get<double>(left_value);
            double right_num = std::get<double>(right_value);
            if (op == "+") return left_num + right_num;
            if (op == "-") return left_num - right_num;
            if (op == "*") return left_num * right_num;
            if (op == "/") {
                if (right_num == 0) throw RunTimeError("Divisão por zero");
                return left_num / right_num;
            }
        }
        throw RunTimeError("Operador aritmético '" + op + "' requer operandos numéricos");
    } else if (auto* unary = dynamic_cast<Unary*>(expr)) {
        Value value = evaluate(unary->getExpr());
        std::string op = unary->getToken().getTag();
        if (op == "-") {
            if (std::holds_alternative<double>(value)) {
                return -std::get<double>(value);
            }
            throw RunTimeError("Operador unário '-' aplicado a tipo não numérico");
        } else if (op == "!") {
            return !is_true(value);
        }
        throw RunTimeError("Operador unário não suportado: " + op);
    } else if (auto* logical = dynamic_cast<Logical*>(expr)) {
        Value left_value = evaluate(logical->getLeft());
        std::string op = logical->getToken().getTag();

        if (op == "AND") {
            if (!is_true(left_value)) return false;
            Value right_value = evaluate(logical->getRight());
            return is_true(right_value);
        } else if (op == "OR") {
            if (is_true(left_value)) return true;
            Value right_value = evaluate(logical->getRight());
            return is_true(right_value);
        }
        throw RunTimeError("Operador lógico não suportado: " + op);
    }
    throw RunTimeError("Expressão não suportada");
}

/**
 * @brief Executa uma função definida pelo usuário.
 *
 * Cria um novo escopo, associa os argumentos aos parâmetros, executa os statements
 * da função e retorna o valor de retorno, se houver.
 *
 * @param func Ponteiro para a definição da função.
 * @param args Lista de argumentos passados para a função.
 * @return Valor retornado pela função ou uma string vazia se não houver retorno.
 * @throws RunTimeError se o número de argumentos não coincidir com os parâmetros.
 */
Value Interpreter::execute_function(FuncDef* func, const Arguments& args) {
    push_scope();
    const Parameters& params = func->getParams();
    if (args.size() != params.size()) {
        throw RunTimeError("Número incorreto de argumentos para função " + func->getName());
    }

    size_t i = 0;
    for (const auto& [name, param_data] : params) {
        Value value = evaluate(args[i].get());
        scopes.back().variables[name] = value;
        i++;
    }

    for (const auto& stmt : func->getBody()) {
        execute_stmt(stmt.get());
        if (return_flag) {
            Value result = return_value;
            return_flag = false;
            pop_scope();
            return result;
        }
    }
    pop_scope();
    return std::string("");
}

/**
 * @brief Inicia um servidor para um canal de serviço (SChannel).
 *
 * Configura um socket TCP, vincula-o à porta definida, escuta conexões e,
 * ao receber uma mensagem, executa a função associada ao canal e envia a resposta.
 *
 * @param schannel Ponteiro para o SChannel a ser executado.
 * @throws RunTimeError em caso de erro na criação ou configuração do socket.
 */
void Interpreter::run_server(SChannel* schannel) {
    std::string name = schannel->getName();
    Value localhost_val = evaluate(schannel->getLocalhostNode());
    Value port_val = evaluate(schannel->getPortNode());
    std::string func_name = schannel->getFuncName();
    Value desc_val = evaluate(schannel->getDescription());

    std::string localhost = std::get<std::string>(localhost_val);
    int port = static_cast<int>(std::get<double>(port_val));
    std::string description = std::get<std::string>(desc_val);

    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        throw RunTimeError("Erro ao criar socket para SChannel '" + name + "'");
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        throw RunTimeError("Erro ao configurar opções do socket para SChannel '" + name + "'");
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw RunTimeError("Erro ao vincular socket para SChannel '" + name + "' na porta " + std::to_string(port));
    }

    if (listen(server_fd, 3) < 0) {
        throw RunTimeError("Erro ao escutar no socket para SChannel '" + name + "'");
    }

    std::cout << "SChannel '" << name << "' escutando em " << localhost << ":" << port << " (" << description << ")\n";

    while (true) {
        if ((client_fd = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            std::cerr << "Erro ao aceitar conexão em SChannel '" << name << "'\n";
            continue;
        }

        read(client_fd, buffer, 1024);
        std::string message(buffer);
        std::cout << "Mensagem recebida: " << message << "\n";

        Arguments args;
        args.push_back(std::make_unique<Constant>("STRING", Token("STRING", message)));
        Value result = execute_function(functions[func_name], args);

        std::string response = convert_value_to_string(result);
        send(client_fd, response.c_str(), response.length(), 0);
        std::cout << "Resposta enviada: " << std::stoi(response) << "\n";

        close(client_fd);
    }
    close(server_fd);
}

/**
 * @brief Converte um Value para sua representação em string.
 *
 * Utiliza std::visit para tratar os diferentes tipos armazenados no Value.
 *
 * @param value Valor a ser convertido.
 * @return String representando o valor.
 */
std::string Interpreter::convert_value_to_string(const Value& value) {
    return std::visit([](auto&& val) -> std::string {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, double>) {
            if (std::fabs(val - std::round(val)) < 1e-9)
                return std::to_string(static_cast<int>(std::round(val)));
            else {
                std::ostringstream oss;
                oss << val;
                return oss.str();
            }
        } else if constexpr (std::is_same_v<T, std::string>) {
            return val;
        } else if constexpr (std::is_same_v<T, bool>) {
            return val ? "true" : "false";
        } else {
            return "";
        }
    }, value);
}

/**
 * @brief Executa um statement (comando) da AST.
 *
 * Identifica o tipo do statement e executa a operação correspondente, como
 * atribuição, chamada de função, definição de função, controle de fluxo, etc.
 *
 * @param stmt Ponteiro para o statement a ser executado.
 * @throws RunTimeError se o statement for nulo ou de um tipo não suportado.
 */
void Interpreter::execute_stmt(Node* stmt) {
    if (!stmt) {
        throw RunTimeError("Tentativa de executar statement nulo");
    }
    if (auto* assign = dynamic_cast<Assign*>(stmt)) {
        std::string var_name = assign->getLeft()->getToken().getValue();
        Value value = evaluate(assign->getRight());
        scopes.back().variables[var_name] = value;
    } else if (auto* call = dynamic_cast<Call*>(stmt)) {
        evaluate(call);
    } else if (auto* func_def = dynamic_cast<FuncDef*>(stmt)) {
        functions[func_def->getName()] = func_def;
    } else if (auto* ret = dynamic_cast<Return*>(stmt)) {
        return_value = evaluate(ret->getExpr());
        return_flag = true;
    } else if (auto* if_stmt = dynamic_cast<If*>(stmt)) {
        Value cond_value = evaluate(if_stmt->getCondition());
        if (is_true(cond_value)) {
            for (const auto& stmt_ptr : if_stmt->getBody()) {
                execute_stmt(stmt_ptr.get());
                if (return_flag || break_flag || continue_flag) break;
            }
        } else if (if_stmt->getElseStmt()) {
            for (const auto& stmt_ptr : *if_stmt->getElseStmt()) {
                execute_stmt(stmt_ptr.get());
                if (return_flag || break_flag || continue_flag) break;
            }
        }
    } else if (auto* while_stmt = dynamic_cast<While*>(stmt)) {
        while (true) {
            Value cond_value = evaluate(while_stmt->getCondition());
            if (!is_true(cond_value)) break;
            for (const auto& stmt_ptr : while_stmt->getBody()) {
                execute_stmt(stmt_ptr.get());
                if (return_flag) return;
                if (break_flag) {
                    break_flag = false;
                    return;
                }
                if (continue_flag) {
                    continue_flag = false;
                    break;
                }
            }
        }
    } else if (auto* brk = dynamic_cast<Break*>(stmt)) {
        break_flag = true;
    } else if (auto* cont = dynamic_cast<Continue*>(stmt)) {
        continue_flag = true;
    } else if (auto* par = dynamic_cast<Par*>(stmt)) {
        std::vector<std::thread> threads;
        for (const auto& stmt_ptr : par->getBody()) {
            threads.emplace_back([this, &stmt_ptr]() {
                execute_stmt(stmt_ptr.get());
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
    } else if (auto* seq = dynamic_cast<Seq*>(stmt)) {
        for (const auto& stmt_ptr : seq->getBody()) {
            execute_stmt(stmt_ptr.get());
            if (return_flag || break_flag || continue_flag) break;
        }
    } else if (auto* cchannel = dynamic_cast<CChannel*>(stmt)) {
        std::string name = cchannel->getName();
        Value localhost = evaluate(cchannel->getLocalhostNode());
        Value port = evaluate(cchannel->getPortNode());
        std::cout << "CChannel '" << name << "' criado com localhost: " 
                  << std::get<std::string>(localhost) << ", port: " 
                  << std::get<std::string>(port) << std::endl;
    } else if (auto* schannel = dynamic_cast<SChannel*>(stmt)) {
        run_server(schannel);
    } else {
        throw RunTimeError("Statement não suportado");
    }
}

/**
 * @brief Executa um módulo (conjunto de statements) da AST.
 *
 * Itera sobre os statements do módulo e os executa sequencialmente.
 *
 * @param module Ponteiro para o módulo a ser executado.
 * @throws RunTimeError se o módulo for nulo.
 */
void Interpreter::execute(Module* module) {
    if (!module) {
        throw RunTimeError("Módulo nulo fornecido para execução");
    }
    for (const auto& stmt : module->getStmts()) {
        execute_stmt(stmt.get());
        if (return_flag) break;
    }
}

/**
 * @brief Sobrecarga do operador de saída para o tipo Value.
 *
 * Permite imprimir um Value utilizando std::ostream.
 *
 * @param os Fluxo de saída.
 * @param v Valor a ser impresso.
 * @return Referência para o fluxo de saída.
 */
std::ostream& operator<<(std::ostream& os, const Value& v) {
    std::visit([&os](const auto& val) { os << val; }, v);
    return os;
}