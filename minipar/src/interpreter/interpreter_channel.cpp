/**
 * @file interpreter_channel.cpp
 * @brief Implementação dos métodos de canal do Interpreter.
 *
 * Contém: builtin_send(), builtin_close(), run_server(), run_client().
 */

#include "../../include/interpreter/interpreter_core.hpp"
#include <iostream>
#include <thread>
#include <cstring>
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  #include <arpa/inet.h>
#endif
#include "../../include/debug.hpp"

ValueWrapper Interpreter::builtin_send(Call *call)
{
    if (call->getArgs().size() != 2)
        throw RunTimeError("Função 'send' espera 2 argumentos");

    ValueWrapper ch_val = evaluate(call->getArgs()[0].get());
    ValueWrapper msg_val = evaluate(call->getArgs()[1].get());

    if (!std::holds_alternative<std::shared_ptr<CChannelValue>>(ch_val.data))
        throw RunTimeError("Primeiro argumento de 'send' deve ser um canal cliente");
    if (!std::holds_alternative<std::string>(msg_val.data))
        throw RunTimeError("Segundo argumento de 'send' deve ser uma string");

    auto channel = std::get<std::shared_ptr<CChannelValue>>(ch_val.data);
    const auto &msg = std::get<std::string>(msg_val.data);

    channel->send_raw(msg + "\n");
    std::string response = channel->recv_until('\n');

    return ValueWrapper(response);
}

ValueWrapper Interpreter::builtin_close(Call *call)
{
    if (call->getArgs().size() != 1)
        throw RunTimeError("Função 'close' espera 1 argumento");

    ValueWrapper ch_val = evaluate(call->getArgs()[0].get());
    if (!std::holds_alternative<std::shared_ptr<CChannelValue>>(ch_val.data))
        throw RunTimeError("Argumento de 'close' deve ser um canal cliente");

    auto channel = std::get<std::shared_ptr<CChannelValue>>(ch_val.data);
    channel->close();

    return ValueWrapper();
}

/**
 * @brief Executa um servidor para um canal S_CHANNEL.
 */
void Interpreter::run_server(SChannel *schannel)
{
    std::string name = schannel->getName();
    ValueWrapper localhost_val = evaluate(schannel->getLocalhostNode());
    ValueWrapper port_val = evaluate(schannel->getPortNode());
    std::string func_name = schannel->getFuncName();
    ValueWrapper desc_val = evaluate(schannel->getDescription());

    std::string localhost = std::get<std::string>(localhost_val.data);
    int port = static_cast<int>(std::get<long double>(port_val.data));
    std::string description = std::get<std::string>(desc_val.data);

    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
        throw RunTimeError("Erro ao criar socket para SChannel '" + name + "'");

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
        throw RunTimeError("Erro ao configurar opções do socket para SChannel '" + name + "'");

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
        throw RunTimeError("Erro ao vincular socket para SChannel '" + name + "' na porta " + std::to_string(port));

    if (listen(server_fd, 3) < 0)
        throw RunTimeError("Erro ao escutar no socket para SChannel '" + name + "'");

    std::cout << "SChannel '" << name << "' escutando em " << localhost << ":" << port << " (" << description << ")\n";

    while (true)
    {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            std::cerr << "Erro ao aceitar conexão em SChannel '" << name << "'\n";
            continue;
        }

        read(client_fd, buffer, 1024);
        std::string message(buffer);
        std::cout << "Mensagem recebida: " << message << "\n";

        Arguments args;
        args.push_back(std::make_unique<Constant>("STRING", Token("STRING", message)));
        ValueWrapper result = execute_function(functions[func_name], args);

        std::string response = convert_value_to_string(result);
        send(client_fd, response.c_str(), response.length(), 0);
        std::cout << "Resposta enviada: " << response << "\n";

        close(client_fd);
    }
    close(server_fd);
}

/**
 * @brief Executa um canal de cliente (CChannel).
 */
void Interpreter::run_client(CChannel *cchannel)
{
    std::string name = cchannel->getName();
    ValueWrapper localhost_val = evaluate(cchannel->getLocalhostNode());
    ValueWrapper port_val = evaluate(cchannel->getPortNode());

    std::string host = std::get<std::string>(localhost_val.data);
    int port = static_cast<int>(std::get<long double>(port_val.data));

    struct sockaddr_in serv_addr;
    char buffer[1024];

    std::cout << "CChannel '" << name
              << "' pronto para conectar em " << host << ":" << port << "\n";

    while (true)
    {
        std::string message;
        {
            std::lock_guard<std::mutex> lk(cout_mutex);
            std::getline(std::cin, message);
        }
        if (message.empty())
            continue;

        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
            throw RunTimeError("Erro ao criar socket para CChannel '" + name + "'");

        std::memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, host.c_str(), &serv_addr.sin_addr) <= 0)
        {
            close(sock);
            throw RunTimeError("Endereço inválido em CChannel '" + name + "'");
        }
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        {
            close(sock);
            std::cerr << "CChannel '" << name << "': falha ao conectar, tentando novamente\n";
            continue;
        }
        send(sock, message.c_str(), message.size(), 0);
        int n = read(sock, buffer, sizeof(buffer) - 1);
        if (n < 0)
        {
            std::cerr << "CChannel '" << name << "': erro na leitura da resposta\n";
        }
        else
        {
            buffer[n] = '\0';
            std::lock_guard<std::mutex> lk(cout_mutex);
            std::cout << "Resposta recebida: " << buffer << "\n";
        }
        close(sock);
    }
}
