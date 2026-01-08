#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <vector>
#include "../../httpRequest.hpp"

// + update my uri thingy and host and stuff ya
// 

enum Resource
{
    REDIRECT,
    CGI,
    FILE,
    DIRECTORY_LISTING,
    ERROR
};

struct Route
{
    Resource type;
    
    int status;
    std::string file_path;
    /* std::sring error_info; */
    std::string redirect_url;
    std::string cgi_path;
    std::string cgi_ext;
    const LocationConfig* location;
    
    Route() : type(ERROR), status(0), location(nullptr) {}
};


class Router
{
	private:
    ServerConfig &server;
	parsedRequest &req;
    Route result;

	const LocationConfig* findLocation(const std::string& uri) const;
    bool isCGI(const LocationConfig* loc, const std::string& file_path) const
	std::string mapURI(const LocationConfig &loc, std::string &uri);
	
	public:
    Router(const ServerConfig& server, const parsedRequest& req);
    ~Router();
	void route();
};

#endif

Router::Router(const ServerConfig& server, const parsedRequest &req): server(server), req(req) {}

Router::~Router() {}

bool Router::isCGI(const LocationConfig* loc, const std::string& file_path) const
{
    if (!loc || loc->cgi_extension.empty())
        return false;
    size_t dot = file_path.find_last_of('.');
    if (dot == std::string::npos)
        return false;
    
    std::string extension = file_path.substr(dot);
    return extension == loc->cgi_extension;
}

// match location, check redirect, check allow methods, resolve path -> static | directory listing | cgi
void Router::route()
{
	const LocationConfig *loc = findLocation(serv, req.uri);
    if (loc && !loc.find(return_).empty())
	{
		result.type = REDIRECT
		result.status = stoi(location.return_.substr(0, 3));
        result.redirect_url = location.return_.substr(3, return_.back());
	}
	else if (loc && !loc.allow_methods.find(req.method))
	{
		result.type = ERROR;
		result.status = 405;
	}
	else
	{
		result.file_path = mapURI(loc, req.uri);
		struct stat st;
        if (stat(path.c_str(), &st) != 0)
        {
            result.type = error;
            result.status = NotFound; //404
        }
        else if (S_ISDIR(st.st_mode))
        {
            // Try to find index file
            std::string index_path = resolveIndexFile(path, loc); //index path needs index in config
        
            if (!index_path.empty())
            {
                result.file_path = index_path; // any issues with index prio?
            
                if (isCGI(loc, index_path))
                {
                    if (loc.cgi_path.empty())
                    {
                        result.type = ERROR;
                        result.status = InternalServerError; //500
                    }
                    else
                    {
                        result.type = CGI;
                        /* result.cgi_path = getCGIInterpreter(loc, index_path); */
                    }
                }
                else
                    result.type = FILE;
            }
            else if (loc && loc->autoindex)
                result.type = DIRECTORY_LISTING;
            else
            {
                result.type = ERROR;
                result.status = 403; // Forbidden
            }
        }
        else if (S_ISREG(st.st_mode))
        {
            if (isCGI(loc, path))
            {
                if (loc.cgi_path.empty())
                {
                    result.type = ERROR;
                    result.status = InternalServerError; //500
                }
                else
                {
                    result.type = CGI;
                    /* result.cgi_path = getCGIInterpreter(loc, index_path); */
                }
            }
            else
                result.type = FILE;
        }
        else
        {
            result.type = ERROR;
            result.status = 403; // forbidden
        }
    }
}

std::string Router::mapURI(const LocationConfig &loc, std::string &uri)
{
    std::string root;
    if (loc && !loc->root.empty())
        root = loc->root;
    else
        root = server.root;
    
    std::string relative_path = uri;
    if (loc && uri.find(loc->path) == 0)
    {
        relative_path = uri.substr(loc->path.length());
        if (relative_path.empty() || relative_path[0] != '/')
            relative_path = "/" + relative_path;
    }

    std::string full_path = root;
    if (!full_path.empty() && full_path.back() == '/' && !relative_path.empty() && relative_path[0] == '/')
        full_path.pop_back();  // Avoid double slash
    full_path += relative_path;
    
    return full_path;
}

const LocationConfig* Router::findLocation(const std::string& uri)
{
	if (!server || server->locations.empty())
    	return nullptr;
  
  	const LocationConfig* best_match = nullptr;
  	size_t best_match_len = 0;
  
  	for (const auto& loc : server->locations)
  	{
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