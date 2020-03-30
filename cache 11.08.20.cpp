#include "proxy.h"
#include "writter.cpp"

class Cache {
    
private:
    static size_t m_capacity;
    //m_map_iter->first: key, m_map_iter->second: list iterator;
    static std::unordered_map<std::string,  std::list<std::pair<std::string, Request*>>::iterator> m_map;
    //m_list_iter->first: key, m_list_iter->second: value;
    static std::list<std::pair<std::string, Request*>> m_list;
    static int ID;

public:
    static Request* makeRequest(std::unordered_map<std::string, std::string> information) {
        std::string action = information.at("ACTION");
        
        if (action == "POST" || action == "CONNECT") {
            Request* request = new Request(information, ID);
            ID++;
            Writter::writeRequest(request);
            return request;
        }
        Request* request = inCache(information);
        return request;
    }
    
    /*
     Check whether the request is in the cache.
     If yes, return it, otherwise return a new request object.
     */
    static Request* inCache(std::unordered_map<std::string, std::string> information) {
        std::string key;
        key.append(information.at("ACTION"));
        key.append(" ");
        key.append(information.at("WEB"));
        key.append(" ");
        key.append(information.at("HTTP"));
        
        auto found_iter = m_map.find(key);
        if (found_iter == m_map.end()) {
            //not in the cache
            Request* request = new Request(information, ID);
            ID++;
            
            //write log
            Writter::writeRequest(request);
            std::string log = std::to_string(request->getID()) + ": not in cache\n";
            Writter::write(log);
            
            //put into cache
            //reached capacity
            if (m_map.size() == m_capacity) {
                std::cout << "Arrive the MAXIMUM" << std::endl;
               std::string key_to_del = m_list.back().first;
                //remove node in list;
               Request* remove = m_list.back().second;
               remove->deleteResponse();
               delete remove;
                m_list.pop_back();
               m_map.erase(key_to_del);
                
            }
            std::string key = request->toString();
            m_list.emplace_front(key, request);  //create new node in list
            m_map[key] = m_list.begin();       //create correspondence between key and node
            return request;
        } else {
            //write to log
            Request* request = found_iter->second->second;
            std::string response = request->getResponse();
            std::string log;
            Writter::writeRequest(request);
            if (response == "EXPIREDTIME") {
                log = std::to_string(request->getID()) + ": in cache, but expired at " + information.at("TIME");
                Writter::write(log);
            } else if (response == "REVALIDATION") {
                log = std::to_string(request->getID()) + ": in cache, requires re-validation\n";
                Writter::write(log);
            } else {
                log = std::to_string(request->getID()) + ": in cache, valid\n";
                Writter::write(log);
            }
            //move the node corresponding to key to front
            m_list.splice(m_list.begin(), m_list, found_iter->second);
            return request;
        }
    }
    
};

size_t Cache::m_capacity = 20;
std::unordered_map<std::string,  std::list<std::pair<std::string, Request*>>::iterator> Cache::m_map = {};
std::list<std::pair<std::string, Request*>> Cache::m_list = std::list<std::pair<std::string, Request*>>();
int Cache::ID = 0;
