/**
 * @file interpreter_channel.cpp
 * @brief Implementação dos métodos de canal do Interpreter.
 *
 * Contém: CChannelValue, builtin_send(), builtin_close(),
 *         run_server(), run_client(), helpers de framing.
 *
 * Protocolo: 4 bytes big-endian uint32_t de tamanho, seguidos do payload.
 * Modelo: conexão por request (cada request() conecta/envia/recebe/fecha).
 * SIGPIPE: evitado via MSG_NOSIGNAL no send() (0 no Windows).
 */

#include "../../include/interpreter/interpreter_core.hpp"
#include <iostream>
#include <thread>
#include <cstring>
#include <cstdint>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include "../../include/debug.hpp"

// ─── platform helpers ───────────────────────────────────────────
#ifdef _WIN32
constexpr int SEND_FLAGS = 0;
#define CLOSE_SOCKET closesocket
#else
constexpr int SEND_FLAGS = MSG_NOSIGNAL;
#define CLOSE_SOCKET ::close
#endif
static void send_frame(int fd, const std::string &payload)
{
    uint32_t net_len = htonl(static_cast<uint32_t>(payload.size()));
    const char *header = reinterpret_cast<const char *>(&net_len);
    size_t sent = 0;
    while (sent < 4)
    {
        ssize_t n = ::send(fd, header + sent, 4 - sent, SEND_FLAGS);
        if (n <= 0)
            throw RunTimeError("CChannel: erro ao enviar frame header");
        sent += static_cast<size_t>(n);
    }
    const char *data = payload.data();
    size_t total = 0, remaining = payload.size();
    while (total < remaining)
    {
        ssize_t n = ::send(fd, data + total, remaining - total, SEND_FLAGS);
        if (n <= 0)
            throw RunTimeError("CChannel: erro ao enviar frame body");
        total += static_cast<size_t>(n);
    }
}
static std::string recv_frame(int fd)
{
    uint32_t net_len = 0;
    char *p = reinterpret_cast<char *>(&net_len);
    size_t received = 0;
    while (received < 4)
    {
        ssize_t n = ::recv(fd, p + received, 4 - received, 0);
        if (n <= 0)
        {
            if (n == 0)
                throw RunTimeError("CChannel: conexao fechada pelo peer (header)");
            throw RunTimeError("CChannel: erro ao receber frame header");
        }
        received += static_cast<size_t>(n);
    }

    uint32_t payload_len = ntohl(net_len);
    std::string payload(static_cast<size_t>(payload_len), '\0');
    received = 0;
    while (received < payload_len)
    {
        ssize_t n = ::recv(fd, &payload[received], payload_len - received, 0);
        if (n <= 0)
        {
            if (n == 0)
                throw RunTimeError("CChannel: conexao fechada pelo peer (body)");
            throw RunTimeError("CChannel: erro ao receber frame body");
        }
        received += static_cast<size_t>(n);
    }
    return payload;
}
static int connect_to_host(const std::string &host, uint16_t port)
{
    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_ADDRCONFIG;

    int err = ::getaddrinfo(host.c_str(),
                            std::to_string(port).c_str(),
                            &hints, &res);
    if (err != 0)
    {
        throw RunTimeError("CChannel: getaddrinfo falhou para '" +
                           host + "': " + gai_strerror(err));
    }

    int sock_fd = -1;
    for (auto ptr = res; ptr != nullptr; ptr = ptr->ai_next)
    {
        sock_fd = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sock_fd < 0)
            continue;

        if (::connect(sock_fd, ptr->ai_addr, ptr->ai_addrlen) == 0)
            break;

        CLOSE_SOCKET(sock_fd);
        sock_fd = -1;
    }
    ::freeaddrinfo(res);

    if (sock_fd < 0)
    {
        throw RunTimeError("CChannel: falha ao conectar em " +
                           host + ":" + std::to_string(port));
    }
    return sock_fd;
}
struct FdGuard
{
    int fd;
    ~FdGuard()
    {
        if (fd >= 0)
            CLOSE_SOCKET(fd);
    }
};

CChannelValue::CChannelValue(const std::string &host, uint16_t port)
    : host_(host), port_(port)
{
}

std::string CChannelValue::request(const std::string &msg)
{
    if (closed_)
        throw RunTimeError("CChannel: canal fechado (host=" + host_ +
                           ", port=" + std::to_string(port_) + ")");

    int sock = connect_to_host(host_, port_);
    FdGuard guard{sock};

    send_frame(sock, msg);
    return recv_frame(sock);
}

ValueWrapper Interpreter::builtin_send(Call *call)
{
    if (call->getArgs().size() != 2)
        throw RunTimeError("Funcao 'send' espera 2 argumentos");

    ValueWrapper ch_val = evaluate(call->getArgs()[0].get());
    ValueWrapper msg_val = evaluate(call->getArgs()[1].get());

    if (!std::holds_alternative<std::shared_ptr<CChannelValue>>(ch_val.data))
        throw RunTimeError("Primeiro argumento de 'send' deve ser um canal cliente");
    if (!std::holds_alternative<std::string>(msg_val.data))
        throw RunTimeError("Segundo argumento de 'send' deve ser uma string");

    auto channel = std::get<std::shared_ptr<CChannelValue>>(ch_val.data);
    const auto &msg = std::get<std::string>(msg_val.data);

    std::string response = channel->request(msg);
    return ValueWrapper(response);
}

ValueWrapper Interpreter::builtin_close(Call *call)
{
    if (call->getArgs().size() != 1)
        throw RunTimeError("Funcao 'close' espera 1 argumento");

    ValueWrapper ch_val = evaluate(call->getArgs()[0].get());
    if (!std::holds_alternative<std::shared_ptr<CChannelValue>>(ch_val.data))
        throw RunTimeError("Argumento de 'close' deve ser um canal cliente");

    auto channel = std::get<std::shared_ptr<CChannelValue>>(ch_val.data);
    channel->close();

    return ValueWrapper();
}

/**
 * @brief Executa um servidor SChannel.
 *
 * - Bind respeita @p localhost (usa getaddrinfo com AI_PASSIVE).
 *   Wildcard ("*", "0.0.0.0", "") mapeia para INADDR_ANY.
 *   "localhost" → 127.0.0.1.
 * - Protocolo: lê frame (4-byte len + payload), chama handler,
 *   envia resposta emoldurada, fecha client_fd.
 * - Loop infinito; cada iteração é um request completo.
 */
void Interpreter::run_server(SChannel *schannel)
{
    std::string name = schannel->getName();
    ValueWrapper lh_val = evaluate(schannel->getLocalhostNode());
    ValueWrapper port_val = evaluate(schannel->getPortNode());
    std::string func_name = schannel->getFuncName();
    ValueWrapper desc_val = evaluate(schannel->getDescription());

    auto *lh_str = std::get_if<std::string>(&lh_val.data);
    if (!lh_str)
        throw RunTimeError("SChannel '" + name + "': localhost deve ser string");
    auto *desc_str = std::get_if<std::string>(&desc_val.data);
    if (!desc_str)
        throw RunTimeError("SChannel '" + name + "': description deve ser string");
    auto *p_num = std::get_if<long double>(&port_val.data);
    if (!p_num)
        throw RunTimeError("SChannel '" + name + "': porta deve ser número");

    std::string localhost = *lh_str;
    uint16_t port = to_port(*p_num, ("Porta de SChannel '" + name + "'").c_str());
    std::string description = *desc_str;
    const char *node = nullptr;
    if (localhost == "*" || localhost == "0.0.0.0" || localhost.empty())
        node = nullptr;
    else
        node = localhost.c_str();

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int err = ::getaddrinfo(node,
                            std::to_string(port).c_str(),
                            &hints, &res);
    if (err != 0)
    {
        throw RunTimeError("SChannel '" + name +
                           "': getaddrinfo falhou para bind: " + gai_strerror(err));
    }

    int server_fd = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_fd < 0)
    {
        ::freeaddrinfo(res);
        throw RunTimeError("Erro ao criar socket para SChannel '" + name + "'");
    }

    int opt = 1;
    if (::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        ::freeaddrinfo(res);
        CLOSE_SOCKET(server_fd);
        throw RunTimeError("Erro ao configurar SO_REUSEADDR para SChannel '" + name + "'");
    }
#ifdef SO_REUSEPORT
    if (::setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
    {
        ::freeaddrinfo(res);
        CLOSE_SOCKET(server_fd);
        throw RunTimeError("Erro ao configurar SO_REUSEPORT para SChannel '" + name + "'");
    }
#endif

    if (::bind(server_fd, res->ai_addr, static_cast<socklen_t>(res->ai_addrlen)) < 0)
    {
        ::freeaddrinfo(res);
        CLOSE_SOCKET(server_fd);
        throw RunTimeError("Erro ao vincular socket para SChannel '" + name +
                           "' na porta " + std::to_string(port));
    }
    ::freeaddrinfo(res);

    if (::listen(server_fd, 3) < 0)
    {
        CLOSE_SOCKET(server_fd);
        throw RunTimeError("Erro ao escutar no socket para SChannel '" + name + "'");
    }

    std::cout << "SChannel '" << name << "' escutando em "
              << localhost << ":" << port << " (" << description << ")\n";

    while (true)
    {
        struct sockaddr_in client_addr{};
        socklen_t addrlen = sizeof(client_addr);

        int client_fd = ::accept(server_fd,
                                 reinterpret_cast<struct sockaddr *>(&client_addr),
                                 &addrlen);
        if (client_fd < 0)
        {
            std::cerr << "Erro ao aceitar conexao em SChannel '" << name << "'\n";
            continue;
        }
        std::string message;
        try
        {
            message = recv_frame(client_fd);
        }
        catch (const RunTimeError &e)
        {
            std::cerr << "SChannel '" << name << "': " << e.what() << "\n";
            CLOSE_SOCKET(client_fd);
            continue;
        }

        std::cout << "Mensagem recebida: " << message << "\n";
        Arguments args;
        args.push_back(std::make_unique<Constant>("STRING",
                                                  Token("STRING", message)));
        auto it = functions.find(func_name);
        if (it == functions.end())
        {
            CLOSE_SOCKET(client_fd);
            throw RunTimeError("Funcao '" + func_name +
                               "' nao encontrada para handler do canal");
        }
        ValueWrapper result = execute_function(it->second, args);
        std::string response = convert_value_to_string(result);
        try
        {
            send_frame(client_fd, response);
        }
        catch (const RunTimeError &e)
        {
            std::cerr << "SChannel '" << name
                      << "': erro ao enviar resposta: " << e.what() << "\n";
        }

        std::cout << "Resposta enviada: " << response << "\n";
        CLOSE_SOCKET(client_fd);
    }

    CLOSE_SOCKET(server_fd);
}
/**
 * @brief Cliente stdin interativo (CChannel).
 *
 * Lê linhas de stdin, conecta a cada linha, envia frame, recebe frame.
 */
void Interpreter::run_client(CChannel *cchannel)
{
    std::string name = cchannel->getName();
    ValueWrapper lh_val = evaluate(cchannel->getLocalhostNode());
    ValueWrapper port_val = evaluate(cchannel->getPortNode());

    auto *h_str = std::get_if<std::string>(&lh_val.data);
    if (!h_str)
        throw RunTimeError("CChannel '" + name + "': localhost deve ser string");
    auto *p_num = std::get_if<long double>(&port_val.data);
    if (!p_num)
        throw RunTimeError("CChannel '" + name + "': porta deve ser número");

    std::string host = *h_str;
    uint16_t port = to_port(*p_num, ("Porta de CChannel '" + name + "'").c_str());

    std::cout << "CChannel '" << name
              << "' pronto para conectar em " << host << ":" << port << "\n";

    while (true)
    {
        std::string message;
        {
            std::lock_guard<std::mutex> lk(cout_mutex);
            if (!std::getline(std::cin, message))
                break;
        }
        if (message.empty())
            continue;

        try
        {
            int sock = connect_to_host(host, port);
            FdGuard guard{sock};

            send_frame(sock, message);
            std::string response = recv_frame(sock);

            {
                std::lock_guard<std::mutex> lk(cout_mutex);
                std::cout << "Resposta recebida: " << response << "\n";
            }
        }
        catch (const RunTimeError &e)
        {
            std::cerr << "CChannel '" << name << "': " << e.what()
                      << " — tentando novamente\n";
        }
    }
}
