
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "../../include/server.hpp"
#include "../../include/router.hpp"
#include "../../include/requestHandler.hpp"
#include "../../include/httpResponse.hpp"
#include "../../include/utils.hpp"

Server::Server(ConfigParser &config): config(config), poller(event::make_poller()) {}

Server::~Server() {}

void Server::init()
{
    const std::vector<ServerConfig>& servers = config.getServers();

    for (const auto& server : servers)
    {
        for (const auto& lp : server.listen_port)
        {
            int fd = -1;
            for (const auto& ls : listen_sockets)
            {
                if (ls.ip == lp.ip && ls.port == lp.port)
                {
                    fd = ls.fd;
                    break;
                }
            }
            if (fd < 0)
            {
                fd = create_and_listen(lp);
                if (fd < 0)
                    throw std::runtime_error("Failed to bind socket");
                listen_sockets.emplace_back(fd, lp.ip, lp.port);
                #ifdef DEBUG
                std::cout << "\nListening on http://"
                        << inet_ntoa(*(struct in_addr*)&lp.ip)
                        << ":" << lp.port << "\n";
                #endif
            }
            listen_fd_set.insert(fd);
        }
    }
}

void Server::run()
{

    while (true)
    {
        auto events = poller->wait(1000);
    
        for (const auto& event : events)
        {
            if (listen_fd_set.count(event.fd) > 0)
            {
                new_connection(event.fd);
                continue;
            }
            if (event.readable)
                read_client(event.fd);
            if (event.writable)
                client_write(event.fd);
        } 
        check_timeouts(); 
    }
}

//update(fd, read, write) -> event::update(fd, true, true);
void Server::client_write(int client_fd)
{
    auto it = clients.find(client_fd);
    if (it == clients.end())
        return;

    ClientCon& conn = it->second;

    if (!conn.res_ready || conn.response_out.empty())
        return;

    int remaining = conn.response_out.size() - conn.sent;
    if (remaining <= 0)
        return;

    int written = event::send_data(client_fd, conn.response_out.data() + conn.sent, remaining);

    if (written > 0)
    {
        conn.sent += written;

        if (conn.sent == conn.response_out.size())
        {
            conn.response_out.clear();
            conn.sent = 0;
            conn.res_ready = false;

            if (!conn.keep_alive)
            {
                close_client(conn.fd);
                return;
            }
            conn.req.clear();
            conn.last_act = std::time(nullptr);
            poller->update(conn.fd, true, false);
        }
    }
    else if (written == 0)
        return;
    else
        close_client(conn.fd);
}

void Server::new_connection(int listen_fd)
{
    while (true)
    {
        int client_fd = event::accept_connection(listen_fd);
        if (client_fd == -1)
            break;  // No more connections
        
        // Find which IP/port this listen socket is on
        const ListenSocket* ls = get_listen_socket(listen_fd);
        if (!ls)
        {
            event::close_socket(client_fd);
            continue;
        }
        
        event::set_non_blocking(client_fd);
        poller->add(client_fd, true, false);
        
        clients.emplace(client_fd, ClientCon(client_fd, ls->ip, ls->port));
        #ifdef DEBUG
        std::cout << "\nNew client " << client_fd << " connected to " << ls->port << "\n";
        #endif
    }
}

void Server::read_client(int client_fd)
{
    auto it = clients.find(client_fd);
    if (it == clients.end())
        return;
    
    ClientCon& conn = it->second;
    conn.last_act = std::time(nullptr);
    
    // Read from client
    std::array<char, 4096> buffer{};
    int bytes = event::receive_data(client_fd, buffer.data(), buffer.size());
    
    if (bytes <= 0)
    {
        close_client(client_fd);
        return;
    }

    while (true)
    {
        ParseState state = conn.req.parseRequest(buffer.data(), bytes);
        bytes = 0;
        
        if (state == COMPLETE)
        {
            #ifdef DEBUG
            conn.req.printRequest();
            #endif

            std::string host = conn.req.getHeaderVal("host");
            conn.servConf = findServer(conn.ip, conn.port, host);
            if (!conn.servConf)
            {
                fallback_error(conn, 500);
                return;
            }
            process_request(conn);
            conn.req.clear();
            continue;
        }
        if (state == ERROR)
        {
            int status = conn.req.getRequest().error_code;
            if (status <= 0)
                status = 400;
            fallback_error(conn, status);
            return;
        }
        break;
    }
}

void Server::fallback_error(ClientCon& conn, int status)
{
    std::string body = generateErrorPage(status, mapStatus(status));

    conn.response_out =
        "HTTP/1.1 " + std::to_string(status) + " " + mapStatus(status) + "\r\n"
        "Date: " + httpDate() + "\r\n"
        "Server: CatSurf\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: " + std::to_string(body.size()) + "\r\n"
        "Connection: close\r\n"
        "\r\n" + body;

    conn.keep_alive = false;
    conn.res_ready = true;
    poller->update(conn.fd, false, true);
}


const ServerConfig* Server::findServer(uint32_t ip, uint16_t port, const std::string& host_header)
{
	const std::vector<ServerConfig>& servers = config.getServers();
    const ServerConfig *firstMatch = nullptr;

    for (const auto& server : servers)
  	{
        for (const auto& lp : server.listen_port)
    	{
      		if (lp.ip == ip && lp.port == port)
      		{
        		if (!firstMatch)
                    firstMatch = &server; 
                if (!host_header.empty())
        		{
          			for (const auto& name : server.server_name)
          			{
                        if (name == str_tolower(host_header) || name == "_")
                            return &server;
          			}
        		}
      		}
    	}
  	}
    if (firstMatch)
    {
        return firstMatch;
    }
  	return nullptr;
}

const ListenSocket* Server::get_listen_socket(int fd) const
{
    for (const auto& ls : listen_sockets)
    {
        if (ls.fd == fd)
            return &ls;
    }
    return nullptr;
}

void Server::process_request(ClientCon& conn)
{
    Router r(*conn.servConf, conn.req.getRequest());
    Route routy = r.route();
    parsedRequest req = conn.req.getRequest();

    std::string connection = str_tolower(conn.req.getHeaderVal("connection"));
    if (req.http_v == "HTTP/1.1")
        conn.keep_alive = connection != "close";
    else if (req.http_v == "HTTP/1.0")
        conn.keep_alive = connection == "keep-alive";

    RequestHandler handler(routy, req, *conn.servConf, conn.keep_alive);
    HttpResponse res = handler.handle();
    conn.response_out = res.buildResponse();
    conn.res_ready = true;

    // maybe leave read always open and make sure i can pipeline http requests?
    poller->update(conn.fd, false, true);
    conn.last_act = std::time(nullptr);
}

int Server::create_and_listen(const ListenPort& lp)
{
    int fd = event::create_socket();
    event::set_socket_reuse(fd);
    bind_and_listen(fd, lp.port, lp.ip);
    event::set_non_blocking(fd);
    poller->add(fd, true, false);
    return fd;
}

void Server::bind_and_listen(int fd, uint16_t port, uint32_t ip)
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = htons(port);

    if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) != 0)
    {
      throw std::runtime_error("bind failed");
    }

    if (listen(fd, SOMAXCONN) != 0)
    {
      throw std::runtime_error("listen failed");
    }
}

void Server::check_timeouts()
{
    time_t now = std::time(nullptr);
    std::vector<int> to_close;
    
    for (auto& [fd, conn] : clients)
    {
        int timeout = 60;
        if (conn.servConf && conn.servConf->timeout > 0)
            timeout = conn.servConf->timeout;

        if (now - conn.last_act > timeout)
        {
            #ifdef DEBUG
            std::cout << "\nClient " << fd << " timed out\n";
            #endif
            conn.req = HttpRequest();
            to_close.push_back(fd);
        }
    }
    
    for (int fd : to_close)
        close_client(fd);
}

void Server::close_client(int client_fd)
{
    poller->remove(client_fd);
    clients.erase(client_fd);
    event::close_socket(client_fd);
    #ifdef DEBUG
    std::cout << "\nClient " << client_fd << " disconnected\n";
    #endif
}
