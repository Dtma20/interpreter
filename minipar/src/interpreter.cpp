#include "../include/interpreter.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>
#include "../include/debug.hpp"

Interpreter::Interpreter() : break_flag(false), continue_flag(false), return_flag(false) {
    LOG_DEBUG("Interpreter: Construtor chamado, inicializando flags e escopo");
    push_scope();
}

void Interpreter::push_scope() {
    LOG_DEBUG("Interpreter: Empilhando novo escopo, tamanho atual: " << scopes.size());
    scopes.push_back(Scope{});
}

void Interpreter::pop_scope() {
    if (!scopes.empty()) {
        LOG_DEBUG("Interpreter: Desempilhando escopo, tamanho antes: " << scopes.size());
        scopes.pop_back();
        LOG_DEBUG("Interpreter: Escopo desempilhado, tamanho agora: " << scopes.size());
    }
}

bool Interpreter::is_true(const Value& value) {
    LOG_DEBUG("Interpreter: Verificando verdade em valor");
    if (std::holds_alternative<bool>(value)) {
        bool result = std::get<bool>(value);
        LOG_DEBUG("Interpreter: Valor é bool, resultado: " << (result ? "true" : "false"));
        return result;
    } else if (std::holds_alternative<double>(value)) {
        double num = std::get<double>(value);
        bool result = num != 0.0;
        LOG_DEBUG("Interpreter: Valor é double (" << num << "), resultado: " << (result ? "true" : "false"));
        return result;
    } else if (std::holds_alternative<std::string>(value)) {
        bool result = !std::get<std::string>(value).empty();
        LOG_DEBUG("Interpreter: Valor é string, vazia: " << (result ? "não" : "sim") << ", resultado: " << (result ? "true" : "false"));
        return result;
    }
    LOG_DEBUG("Interpreter: Valor desconhecido, retornando false por padrão");
    return false;
}

Value Interpreter::evaluate(Expression* expr) {
    if (!expr) {
        LOG_DEBUG("Interpreter: Tentativa de avaliar expressão nula");
        throw RunTimeError("Tentativa de avaliar expressão nula");
    }
    LOG_DEBUG("Interpreter: Avaliando expressão, tipo: " << typeid(*expr).name());

    if (auto* constant = dynamic_cast<Constant*>(expr)) {
        std::string value = constant->getToken().getValue();
        std::string type = constant->getType();
        LOG_DEBUG("Interpreter: Constante detectada, tipo: " << type << ", valor: " << value);
        if (type == "NUMBER") {
            double num = std::stod(value);
            LOG_DEBUG("Interpreter: Convertendo NUMBER para double: " << num);
            return num;
        } else if (type == "STRING") {
            LOG_DEBUG("Interpreter: Retornando STRING: " << value);
            return value;
        } else if (type == "BOOL") {
            bool result = value == "true";
            LOG_DEBUG("Interpreter: Convertendo BOOL: " << result);
            return result;
        }
        LOG_DEBUG("Interpreter: Erro, tipo de constante não suportado: " << type);
        throw RunTimeError("Tipo de constante não suportado: " + type);
    } else if (auto* id = dynamic_cast<ID*>(expr)) {
        std::string var_name = id->getToken().getValue();
        LOG_DEBUG("Interpreter: Avaliando ID: " << var_name);
        for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
            auto var_it = it->variables.find(var_name);
            if (var_it != it->variables.end()) {
                LOG_DEBUG("Interpreter: Variável " << var_name << " encontrada no escopo atual");
                return var_it->second;
            }
        }
        LOG_DEBUG("Interpreter: Erro, variável não definida: " << var_name);
        throw RunTimeError("Variável não definida: " + var_name);
    } else if (auto* access = dynamic_cast<Access*>(expr)) {
        LOG_DEBUG("Interpreter: Avaliando acesso a elemento");
        Value base_val = evaluate(access->getId());
        Value index_val = evaluate(access->getExpr());
        if (std::holds_alternative<std::string>(base_val) && std::holds_alternative<double>(index_val)) {
            std::string str = std::get<std::string>(base_val);
            int index = static_cast<int>(std::get<double>(index_val));
            LOG_DEBUG("Interpreter: Acesso a string '" << str << "' no índice: " << index);
            if (index >= 0 && index < str.length()) {
                std::string result(1, str[index]);
                LOG_DEBUG("Interpreter: Resultado do acesso: " << result);
                return result;
            }
            LOG_DEBUG("Interpreter: Erro, índice fora do intervalo: " << index);
            throw RunTimeError("Índice fora do intervalo: " + std::to_string(index));
        }
        LOG_DEBUG("Interpreter: Erro, acesso requer string e índice numérico");
        throw RunTimeError("Acesso a elemento requer string e índice numérico");
    } else if (auto* call = dynamic_cast<Call*>(expr)) {
        std::string func_name = call->getId()->getToken().getValue();
        LOG_DEBUG("Interpreter: Avaliando chamada de função: " << func_name);
        if (func_name == "print") {
            for (const auto& arg : call->getArgs()) {
                Value value = evaluate(arg.get());
                LOG_DEBUG("Interpreter: Argumento para print: " << convert_value_to_string(value));
                std::visit([](auto&& val) { std::cout << val << std::endl; }, value);
            }
            LOG_DEBUG("Interpreter: print concluído, retornando string vazia");
            return std::string("");
        } else if (func_name == "len") {
            Value arg = evaluate(call->getArgs()[0].get());
            if (std::holds_alternative<std::string>(arg)) {
                double length = static_cast<double>(std::get<std::string>(arg).length());
                LOG_DEBUG("Interpreter: len retornando: " << length);
                return length;
            }
            LOG_DEBUG("Interpreter: Erro, len requer string");
            throw RunTimeError("len requer uma string como argumento");
        } else if (func_name == "to_number") {
            Value arg = evaluate(call->getArgs()[0].get());
            if (std::holds_alternative<std::string>(arg)) {
                double num = std::stod(std::get<std::string>(arg));
                LOG_DEBUG("Interpreter: to_number retornando: " << num);
                return num;
            }
            LOG_DEBUG("Interpreter: Erro, to_number requer string");
            throw RunTimeError("to_number requer uma string como argumento");
        } else if (func_name == "to_string") {
            Value arg = evaluate(call->getArgs()[0].get());
            std::string result = std::visit([](auto&& val) -> std::string {
                if constexpr (std::is_same_v<std::decay_t<decltype(val)>, std::string>) {
                    return val;
                } else {
                    return std::to_string(val);
                }
            }, arg);
            LOG_DEBUG("Interpreter: to_string retornando: " << result);
            return result;
        } else if (func_name == "isnum") {
            Value arg = evaluate(call->getArgs()[0].get());
            if (std::holds_alternative<std::string>(arg)) {
                std::string str = std::get<std::string>(arg);
                bool result = std::all_of(str.begin(), str.end(), ::isdigit);
                LOG_DEBUG("Interpreter: isnum retornando: " << (result ? "true" : "false"));
                return result;
            }
            LOG_DEBUG("Interpreter: isnum retornando false (não é string)");
            return false;
        } else if (func_name == "isalpha") {
            Value arg = evaluate(call->getArgs()[0].get());
            if (std::holds_alternative<std::string>(arg)) {
                std::string str = std::get<std::string>(arg);
                bool result = std::all_of(str.begin(), str.end(), ::isalpha);
                LOG_DEBUG("Interpreter: isalpha retornando: " << (result ? "true" : "false"));
                return result;
            }
            LOG_DEBUG("Interpreter: isalpha retornando false (não é string)");
            return false;
        } else if (functions.find(func_name) != functions.end()) {
            Value result = execute_function(functions[func_name], call->getArgs());
            LOG_DEBUG("Interpreter: Função definida pelo usuário " << func_name << " retornou valor");
            return result;
        }
        LOG_DEBUG("Interpreter: Erro, função não suportada: " << func_name);
        throw RunTimeError("Função não suportada: " + func_name);
    } else if (auto* relational = dynamic_cast<Relational*>(expr)) {
        LOG_DEBUG("Interpreter: Avaliando operação relacional");
        Value left_value = evaluate(relational->getLeft());
        Value right_value = evaluate(relational->getRight());
        std::string op = relational->getToken().getTag();
        LOG_DEBUG("Interpreter: Operação relacional: " << convert_value_to_string(left_value) << " " << op << " " << convert_value_to_string(right_value));

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
        LOG_DEBUG("Interpreter: Avaliando operação aritmética");
        Value left_value = evaluate(arithmetic->getLeft());
        Value right_value = evaluate(arithmetic->getRight());
        std::string op = arithmetic->getToken().getTag();
        LOG_DEBUG("Interpreter: Operação aritmética: " << convert_value_to_string(left_value) << " " << op << " " << convert_value_to_string(right_value));

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
        LOG_DEBUG("Interpreter: Avaliando operação unária");
        Value value = evaluate(unary->getExpr());
        std::string op = unary->getToken().getTag();
        LOG_DEBUG("Interpreter: Operação unária: " << op << " " << convert_value_to_string(value));
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
        LOG_DEBUG("Interpreter: Avaliando operação lógica");
        Value left_value = evaluate(logical->getLeft());
        std::string op = logical->getToken().getTag();
        LOG_DEBUG("Interpreter: Operação lógica: " << convert_value_to_string(left_value) << " " << op);

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
    LOG_DEBUG("Interpreter: Erro, expressão não suportada: " << typeid(*expr).name());
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
        LOG_DEBUG("Interpreter: Tentativa de executar statement nulo");
        throw RunTimeError("Tentativa de executar statement nulo");
    }
    LOG_DEBUG("Interpreter: Executando stmt, tipo: " << typeid(*stmt).name());
    
    if (auto* assign = dynamic_cast<Assign*>(stmt)) {
        std::string var_name = assign->getLeft()->getToken().getValue();
        Value value = evaluate(assign->getRight());
        scopes.back().variables[var_name] = value;
        LOG_DEBUG("Interpreter: Atribuindo " << var_name << " = " << convert_value_to_string(value));
    } else if (auto* call = dynamic_cast<Call*>(stmt)) {
        LOG_DEBUG("Interpreter: Executando chamada de função como statement");
        evaluate(call);
    } else if (auto* func_def = dynamic_cast<FuncDef*>(stmt)) {
        functions[func_def->getName()] = func_def;
        LOG_DEBUG("Interpreter: Definindo função: " << func_def->getName());
    } else if (auto* ret = dynamic_cast<Return*>(stmt)) {
        return_value = evaluate(ret->getExpr());
        return_flag = true;
        LOG_DEBUG("Interpreter: Retorno definido: " << convert_value_to_string(return_value));
    } else if (auto* if_stmt = dynamic_cast<If*>(stmt)) {
        Value cond_value = evaluate(if_stmt->getCondition());
        LOG_DEBUG("Interpreter: Avaliando IF, condição: " << convert_value_to_string(cond_value));
        if (is_true(cond_value)) {
            LOG_DEBUG("Interpreter: Executando corpo do IF");
            for (const auto& stmt_ptr : if_stmt->getBody()) {
                execute_stmt(stmt_ptr.get());
                if (return_flag || break_flag || continue_flag) break;
            }
        } else if (if_stmt->getElseStmt()) {
            LOG_DEBUG("Interpreter: Executando corpo do ELSE");
            for (const auto& stmt_ptr : *if_stmt->getElseStmt()) {
                execute_stmt(stmt_ptr.get());
                if (return_flag || break_flag || continue_flag) break;
            }
        }
    } else if (auto* while_stmt = dynamic_cast<While*>(stmt)) {
        LOG_DEBUG("Interpreter: Iniciando WHILE");
        while (true) {
            Value cond_value = evaluate(while_stmt->getCondition());
            LOG_DEBUG("Interpreter: Condição do WHILE: " << convert_value_to_string(cond_value));
            if (!is_true(cond_value)) {
                LOG_DEBUG("Interpreter: Saindo do WHILE");
                break;
            }
            for (const auto& stmt_ptr : while_stmt->getBody()) {
                execute_stmt(stmt_ptr.get());
                if (return_flag) {
                    LOG_DEBUG("Interpreter: Retorno detectado no WHILE");
                    return;
                }
                if (break_flag) {
                    LOG_DEBUG("Interpreter: Break detectado no WHILE");
                    break_flag = false;
                    return;
                }
                if (continue_flag) {
                    LOG_DEBUG("Interpreter: Continue detectado no WHILE");
                    continue_flag = false;
                    break;
                }
            }
        }
    } else if (auto* brk = dynamic_cast<Break*>(stmt)) {
        LOG_DEBUG("Interpreter: Executando BREAK");
        break_flag = true;
    } else if (auto* cont = dynamic_cast<Continue*>(stmt)) {
        LOG_DEBUG("Interpreter: Executando CONTINUE");
        continue_flag = true;
    } else if (auto* par = dynamic_cast<Par*>(stmt)) {
        LOG_DEBUG("Interpreter: Executando PAR (execução paralela)");
        std::vector<std::thread> threads;
        for (const auto& stmt_ptr : par->getBody()) {
            threads.emplace_back([this, &stmt_ptr]() {
                execute_stmt(stmt_ptr.get());
            });
            LOG_DEBUG("Interpreter: Thread criada para statement em PAR");
        }
        for (auto& thread : threads) {
            thread.join();
            LOG_DEBUG("Interpreter: Thread concluída em PAR");
        }
    } else if (auto* seq = dynamic_cast<Seq*>(stmt)) {
        LOG_DEBUG("Interpreter: Executando SEQ (sequência)");
        for (const auto& stmt_ptr : seq->getBody()) {
            execute_stmt(stmt_ptr.get());
            if (return_flag || break_flag || continue_flag) {
                LOG_DEBUG("Interpreter: Interrupção detectada na SEQ");
                break;
            }
        }
    } else if (auto* cchannel = dynamic_cast<CChannel*>(stmt)) {
        LOG_DEBUG("Interpreter: Executando C_CHANNEL");
        std::string name = cchannel->getName();
        Value localhost = evaluate(cchannel->getLocalhostNode());
        Value port = evaluate(cchannel->getPortNode());
        std::cout << "CChannel '" << name << "' criado com localhost: " 
                  << std::get<std::string>(localhost) << ", port: " 
                  << std::get<std::string>(port) << std::endl;
        LOG_DEBUG("Interpreter: CChannel criado: " << name);
    } else if (auto* schannel = dynamic_cast<SChannel*>(stmt)) {
        LOG_DEBUG("Interpreter: Iniciando servidor S_CHANNEL");
        run_server(schannel);
    } else {
        LOG_DEBUG("Interpreter: Erro, statement não suportado: " << typeid(*stmt).name());
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
        LOG_DEBUG("Interpreter: Tentativa de executar módulo nulo");
        throw RunTimeError("Módulo nulo fornecido para execução");
    }
    LOG_DEBUG("Interpreter: Iniciando execução do módulo");
    for (const auto& stmt : module->getStmts()) {
        execute_stmt(stmt.get());
        if (return_flag) {
            LOG_DEBUG("Interpreter: Retorno detectado no módulo");
            break;
        }
    }
    LOG_DEBUG("Interpreter: Execução do módulo concluída");
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
