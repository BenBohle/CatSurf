#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <vector>

// parse request - router - response

class Router
{
public:
    Router(const std::vector<ServerConfig>& servers);
    ~Router();
    std::pair<const ServerConfig*, const LocationConfig*> route(const HttpRequest& request) const;

private:
    std::vector<ServerConfig> servers;
    const ServerConfig* findServer(const std::string& host) const;
    const LocationConfig* findLocation(const ServerConfig& server, const std::string& uri) const;
};

#endif

Router::Router(const std::vector<ServerConfig>& servers): servers(servers) {}

Router::~Router() {}


const ServerConfig* findServer(const ConfigParser& conf, uint32_t ip, uint16_t port, const std::string& host_header)
{
  const auto& servers = conf.getServers();
  
  //exact match on IP:port and host header
  for (const auto& server : servers)
  {
    for (const auto& lp : server.listen_port)
    {
      if (lp.ip == ip && lp.port == port)
      {
        //server_name match
        if (!host_header.empty())
        {
          for (const auto& name : server.server_name)
          {
            if (name == host_header || name == "_")
              return &server;
          }
        }
        else
          return &server;
      }
    }
  }
  // fallback
  for (const auto& server : servers)
  {
    for (const auto& lp : server.listen_port)
    {
      if (lp.port == port)
        return &server;
    }
  }
  return nullptr;
}

// Find best matching location for URI
const LocationConfig* findLocation(const ServerConfig* server, const std::string& uri)
{
  if (!server || server->locations.empty())
    return nullptr;
  
  const LocationConfig* best_match = nullptr;
  size_t best_match_len = 0;
  
  for (const auto& loc : server->locations)
  {
    // Check if URI starts with location path
    if (uri.find(loc.path) == 0)
    {
      size_t match_len = loc.path.length();
      if (match_len > best_match_len)
      {
        best_match = &loc;
        best_match_len = match_len;
      }
    }
  }
  
  return best_match;
}