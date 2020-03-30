//
//  Writter.cpp
//  
//
//  Created by Li Yating on 2020/2/27.
//

#include "request.cpp"

std::mutex write_mtx;

class Writter {
    static std::string fileName;
    
public:
    /*
     Write the String into the file.
     */
    static void write(std::string str) {
        std::cout << str;
        
        std::ofstream file;
        file.open(fileName, std::ofstream::out | std::ofstream::app);
        if (!file.is_open()) {
            std::cout << "Not open" << std::endl;
        } else {
            std::cout << "Write: " << str << std::endl;
            file << str;
        }
        file.close();
    }
    
    static void writeRequest(Request* request) {
        std::string log;
        log.append(std::to_string(request->getID()));
        log.append(": \"");
        log.append(request->toString());
        log.append("\" form ");
        log.append(request->getIP());
        log.append(" @ ");
        log.append(request->getTime());
        
        std::cout << log;
        //write_mtx.lock();
        //std::ofstream file("/home/yl657/hw2/proxy/proxy.log");
        std::ofstream file;
        file.open(fileName, std::ofstream::out | std::ofstream::app);
        if (!file.is_open()) {
            std::cout << "Not open" << std::endl;
        } else {
            std::cout << "Write: " << log << std::endl;
            file << log;
        }
        file.close();
        //write_mtx.unlock();
        //myfile << message;
    }
    
    /*
     Write the requesting into file.
     */
    static void writeRequesting(Request request) {
        std::string log = std::to_string(request.getID());
        log.append(": Requesting \"");
        log.append(request.toString());
        log.append("\" from ");
        log.append(request.getHost());
        log.append("\n");
        
        //write_mtx.lock();
        std::ofstream file;
        file.open(fileName, std::ofstream::out | std::ofstream::app);
        if (!file.is_open()) {
            std::cout << "Not open" << std::endl;
        } else {
            std::cout << "Write: " << log << std::endl;
            file << log;
        }
        file.close();
        //write_mtx.unlock();
    }
    
    /*
     Write the responding into file
     */
    static void writeResponding(int id, std::string respond) {
        std::string log = std::to_string(id);
        log.append(": Responding \"");
        log.append(respond);
        log.append("\"\n");
        
        //write_mtx.lock();
        std::cout << log;
//        std::ofstream file("/home/yl657/hw2/proxy/proxy.log");
        std::ofstream file;
        file.open(fileName, std::ofstream::out | std::ofstream::app);
        if (!file.is_open()) {
            std::cout << "Not open" << std::endl;
        } else {
            std::cout << "Write: " << log << std::endl;
            file << log;
        }
        file.close();
        //myfile << log;
        //write_mtx.unlock();
    }
    
    /*
     Write the received into file
     */
    static void writeReceived(Request request, std::string received) {
        std::string log = std::to_string(request.getID());
        log.append(": Received \"");
        log.append(received);
        log.append("\" from ");
        log.append(request.getHost());
        log.append("\n");
        
        //write_mtx.lock();
        std::cout << log;
        std::ofstream file;
        file.open(fileName, std::ofstream::out | std::ofstream::app);
        if (!file.is_open()) {
            std::cout << "Not open" << std::endl;
        } else {
            std::cout << "Write: " << log << std::endl;
            file << log;
        }
        file.close();
        //myfile << log;
        //write_mtx.unlock();
    }
    
};

//std::ofstream Writter::file = std::ofstream("/var/log/erss/proxy.log");
std::string Writter::fileName = "/var/log/erss/proxy.log";
