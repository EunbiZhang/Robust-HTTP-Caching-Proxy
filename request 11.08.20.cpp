#include "proxy.h"
#include "response.cpp"

class Request {
    
    std::string action;
    std::string web;
    std::string http;
    std::string host;
    std::string port="80";
    std::string IP;
    Response *response=nullptr;
    
    int ID;

public:
    Request() {}
    
    Request(std::unordered_map<std::string, std::string> information, int id) {
        action = information.at("ACTION");
        web = information.at("WEB");
        http = information.at("HTTP");
        host = information.at("HOST");
        IP = information.at("CLIENTIP");
        ID = id;
        if (action == "CONNECT") {
            unsigned long pos = host.find(":");
            port = host.substr(pos+1);
            host = host.substr(0, pos);
        }
    }

    std::vector<char> getRevalidMessage(std::string inputStr) {
        std::size_t pos = inputStr.find("\r\n\r\n");
        std::vector<char> input(inputStr.begin(), inputStr.end());
        std::string etag = response->getETag();
        std::vector<char>::iterator it = input.begin() + pos;
        input.insert(it, etag.begin(), etag.end());
        return input;
    }
    
    void deleteResponse() {
        delete response;
    }

    /*
     Renew the birth of response. If fails, return false.
     */
    bool renewResponseBirth() {
        if (response == nullptr) {
            return false;
        }
        response->setBirth(time(nullptr));
        return true;
    }
    
    /*
     If the request contains the same information.
     */
    int compareTo(std::unordered_map<std::string, std::string> map) {
        return map.at("ACTION") == action && map.at("WEB") == web && map.at("HTTP") == http && map.at("HOST") == host;
    }
    
    //set response
    void setResponse(Response *r) {
        if (response != nullptr) {
            delete response;
        }
        response = r;
    }
    
    /*
     Check whether the request has a response
     */
    bool isNewRequest() {
        return response==nullptr;
    }

    /*
     If the response is still valid, return the message.
     Else, return "EXPIREDTIME" or "REVALIDATION".
     */
    std::string getResponse() {
        if (isNewRequest()) {
            return "NEWREQUEST";
        }
        std::string result = response->isValid();
        if (result == "VALID") {
            return response->getMessage();
        }
        return result;
    }
    
    std::string toString() {
        std::string message;
        message.append(action);
        message.append(" ");
        message.append(web);
        message.append(" ");
        message.append(http);
        return message;
    }

    //get
    std::string getAction() {
        return action;
    }

    std::string getWeb() {
        return web;
    }

    std::string getHttp() {
        return http;
    }
    
    std::string getHost() {
        return host;
    }
    
    std::string getPort() {
        return port;
    }
    
    std::string getIP() {
        return IP;
    }
    
    std::string getTime() {
        std::time_t now = std::time(nullptr);
        char *dt = std::ctime(&now);
        return dt;
    }
    
    int getID() {
        return ID;
    }
    
};
