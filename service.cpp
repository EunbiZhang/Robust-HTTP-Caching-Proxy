#include "proxy.h"
#include "cache.cpp"
#include "parse.cpp"

using namespace std;

mutex cache_mtx;

class Service{
public:

    /*---------------------------------------------------------------------------------------------------------
    * Connect with the server:
    * take in the server's hostname and return the client_fd
    */
    static int goServer(const char* hostname, const char* port){
        int status;
        int socket_fd;
        struct addrinfo host_info;
        struct addrinfo *host_info_list;

        memset(&host_info, 0, sizeof(host_info));
        host_info.ai_family   = AF_UNSPEC;
        host_info.ai_socktype = SOCK_STREAM;

        status = getaddrinfo(hostname, port, &host_info, &host_info_list);
        if (status != 0) {
            cerr << "Error: cannot get address info for host: " << hostname << ", " << port << endl;
            string error_message = "no-id: ERROR cannot get address info for host";
            Writter::write(error_message);
            return -1;
        } 
        socket_fd = socket(host_info_list->ai_family, 
                    host_info_list->ai_socktype, 
                    host_info_list->ai_protocol);
        if (socket_fd == -1) {
            cerr << "Error: cannot create socket for: " << hostname << ", " << port << endl;
            string error_message = "no-id: ERROR cannot create socket";
            Writter::write(error_message);
            return -1;
        }  
        status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
        if (status == -1) {
            cerr << "Error: cannot connect to socket" << endl;
            cerr << "  (" << hostname << "," << port << ")" << endl;
            string error_message = "no-id: ERROR cannot connect to socket";
            Writter::write(error_message);
            return -1;
        } //connect
        freeaddrinfo(host_info_list);
        return socket_fd;
    }

    /*----------------------------------------------------------------------------------------------------
    * POST
    */
    static void service_POST(int& client_fd, Request& request, size_t& rev_total_sz, vector<char> post){
        //write requesting form server
        Writter::writeRequesting(request);

        string message(post.begin(), post.end());

        //if not available in cache: construct connection with server
        int to_server_fd = goServer(request.getHost().c_str(), request.getPort().c_str());   
          
        //send the request to server
        size_t send_sz = 0;
        while(send_sz < rev_total_sz){
            string sub_message = message.substr(send_sz);
            int tmp_sz = send(to_server_fd, sub_message.c_str(), sub_message.length(), 0);
            send_sz += tmp_sz;
        }
             
        //receive response from the server
        vector<char> buffer(65535, 0);
        vector<char> response;
        rev_total_sz = 0;
        size_t rev_sz = recv(to_server_fd, &(buffer[0]), 65534, 0);
        vector<char>::iterator it = buffer.begin() + rev_sz;
        response.insert(response.end(), buffer.begin(), it);
        rev_total_sz += rev_sz;

        pair<string, size_t> response_header = Parse::parseContentLength(response);
        string Etag = Parse::parseETag(response);
        string response_type = Parse::parseResponseNumber(response);

        //write responding
        string dummy(response.begin(), response.end());
        Writter::writeResponding(request.getID(), dummy.substr(0, dummy.find("\r\n")));
            
        if(response_header.first == "CHUNKED"){//if chunked
            send(client_fd, response.data(), rev_total_sz, 0);
            while(rev_sz > 6){
                rev_sz = recv(to_server_fd, &(buffer[0]), 65534, 0);
                vector<char>::iterator it = buffer.begin() + rev_sz;
                response.insert(response.end(), buffer.begin(), it);
                rev_total_sz += rev_sz;
                //send the chunk to client
                send(client_fd, buffer.data(), rev_sz, 0);
            }
        }
        else{//if no chunked but have content-length
            size_t target_sz = response_header.second;
            while(rev_total_sz < target_sz) {
                rev_sz = recv(to_server_fd, &(buffer[0]), 65534, 0);
                vector<char>::iterator it = buffer.begin() + rev_sz;
                response.insert(response.end(), buffer.begin(), it);
                rev_total_sz += rev_sz;
            }
            //send received response to the client
            send_sz = 0;
            while(send_sz < rev_total_sz){
                size_t tmp_sz = send(client_fd, response.data(), response.size(), 0);
                send_sz += tmp_sz;
            }
        }   
        close(to_server_fd);
    }


    /*----------------------------------------------------------------------------------------------------
    * GET
    */
    static void service_GET(int& client_fd, Request& request, size_t& rev_total_sz, string& message){
        //check cache
        cache_mtx.lock();
        string cached_response = request.getResponse();
        cache_mtx.unlock();

        if(cached_response == "EXPIREDTIME" || cached_response == "REVALIDATION" || cached_response == "NEWREQUEST"){
            //if not available in cache: construct connection with server
            int to_server_fd = goServer(request.getHost().c_str(), request.getPort().c_str());
            
            //write requesting form server
            Writter::writeRequesting(request);

            if(cached_response == "REVALIDATION"){
                vector<char> m = request.getRevalidMessage(message);
                int total = m.size();
                int send_sz = 0;
                while(send_sz < total){
                    send_sz += send(to_server_fd, m.data(), total - send_sz, 0);
                }
            }
            else{
                //send the request to server
                size_t send_sz = 0;
                while(send_sz < rev_total_sz){
                    string sub_message = message.substr(send_sz);
                    int tmp_sz = send(to_server_fd, sub_message.c_str(), sub_message.length(), 0);
                    send_sz += tmp_sz;
                }
            }
            
            //receive response from the server
            vector<char> buffer(65535, 0);
            vector<char> response;
            rev_total_sz = 0;
            size_t rev_sz = recv(to_server_fd, &(buffer[0]), 65534, 0);
            vector<char>::iterator it = buffer.begin() + rev_sz;
            response.insert(response.end(), buffer.begin(), it);
            rev_total_sz += rev_sz;
            
            pair<string, size_t> response_header = Parse::parseContentLength(response);
            long lifespan = Parse::parseCacheControl(response);
            string Etag = Parse::parseETag(response);
            string response_type = Parse::parseResponseNumber(response);

            //write received response from server
            string dummy(response.begin(), response.end());
            Writter::writeReceived(request, dummy.substr(0, dummy.find("\r\n")));

            size_t send_sz;

            if(cached_response == "REVALIDATION"){
                if(response_type == "304"){
                    //send cached response to the client
                    send_sz = 0;
                    while(send_sz < cached_response.length()){
                        string sub_response = cached_response.substr(send_sz);
                        send_sz += send(client_fd, sub_response.c_str(), sub_response.length(), 0);
                    }
                    request.renewResponseBirth();
                    //write responding
                    string dummy(response.begin(), response.end());
                    Writter::writeResponding(request.getID(), dummy.substr(0, dummy.find("\r\n")));
                    return;
                }
            }
            
            if(response_header.first == "CHUNKED"){//if chunked
                send(client_fd, response.data(), rev_total_sz, 0);
                while(rev_sz > 6){
                    rev_sz = recv(to_server_fd, &(buffer[0]), 65534, 0);
                    vector<char>::iterator it = buffer.begin() + rev_sz;
                    response.insert(response.end(), buffer.begin(), it);
                    rev_total_sz += rev_sz;
                    //send the chunk to client
                    send(client_fd, buffer.data(), rev_sz, 0);
                }
            }
            else{//if no chunked but have content-length
                size_t target_sz = response_header.second;
                while(rev_total_sz < target_sz) {
                    rev_sz = recv(to_server_fd, &(buffer[0]), 65534, 0);
                    vector<char>::iterator it = buffer.begin() + rev_sz;
                    response.insert(response.end(), buffer.begin(), it);
                    rev_total_sz += rev_sz;
                }

                //send received response to the client
                send_sz = 0;
                while(send_sz < rev_total_sz){
                    size_t tmp_sz = send(client_fd, response.data(), response.size(), 0);
                    send_sz += tmp_sz;
                }
            }

            if(response_type == "200 OK"){
                //store in cache
                cache_mtx.lock();
                Response *stored_response = new Response(response, request.getID());
                stored_response->setBirth(time(NULL));
                stored_response->setLifeSpan(lifespan);
                stored_response->setETag(Etag);
                request.setResponse(stored_response);
                cache_mtx.unlock();
            }

            close(to_server_fd);
            //write responding
            string dummy1(response.begin(), response.end());
            Writter::writeResponding(request.getID(), dummy1.substr(0, dummy1.find("\r\n")));
        }
        else{
            //send cached response to the client
            size_t send_sz = 0;
            while(send_sz < cached_response.length()){
                string sub_response = cached_response.substr(send_sz);
                send_sz += send(client_fd, sub_response.c_str(), sub_response.length(), 0);
            }

            //write responding
            string dummy(cached_response.begin(), cached_response.end());
            Writter::writeResponding(request.getID(), dummy.substr(0, dummy.find("\r\n")));
        }     
    }


    /*----------------------------------------------------------------------------------------------------
    * CONNECT
    */
    static void service_CONNECT(int& client_fd, Request& request){
        //construct connection with server
        int to_server_fd = goServer(request.getHost().c_str(), request.getPort().c_str());
 
        //write requesting form server
        Writter::writeRequesting(request);

        //send a 200K to the client
        const char* OK = "HTTP/1.1 200 OK\r\n\r\n";
        while(send(client_fd, OK, strlen(OK), 0) < 0){
            cerr << "error: failed to send 200 OK" << endl;
        }

        //transfer massages between client and server until some side close the connection
        fd_set readfds;
        int max_sd = max(client_fd, to_server_fd);
        while(1){
            FD_ZERO(&readfds);
            FD_SET(client_fd, &readfds);
            FD_SET(to_server_fd, &readfds);
            //wait for an activity on one of the sockets
            int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
            if(activity < 0 ){
                cerr << "error: select" << endl;
                break;
            }
            if(FD_ISSET(client_fd, &readfds)){//if get request from client
                vector<char> buffer(65535, 0);
                size_t rev_sz = recv(client_fd, &(buffer[0]), 65534, 0);
                if(rev_sz == 0) {
                    break;
                }
                //send received request to the server
                size_t tmp_sz = send(to_server_fd, buffer.data(), rev_sz, 0);
                if(tmp_sz == 0) break;
            }
            else if(FD_ISSET(to_server_fd, &readfds)){//if get response from server
                vector<char> buffer(65535, 0);
                size_t rev_sz = recv(to_server_fd, &(buffer[0]), 65534, 0);
                if(rev_sz == 0) {
                    break;
                }
                //send received response to the client
                size_t tmp_sz = send(client_fd, buffer.data(), rev_sz, 0);
                if(tmp_sz == 0) break;
            }
        }
        close(to_server_fd);
        string log1 = to_string(request.getID()) + ": Tunnel Closed\n";
        Writter::write(log1);
   }


   /*---------------------------------------------------------------------------------------------------------
    * After connect with one client firefox, a thread is created to provide specific service:
    * take in client's fd, IP and Port number
    */
    static void service(int client_fd, string client_IP, std::unique_ptr<int> thread_id_ptr){
        //cout << thread_id << endl;
        fd_set readfds;
        int max_sd = client_fd;
        FD_ZERO(&readfds);
        FD_SET(client_fd, &readfds);
        //wait for an request on client
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if(activity < 0 ){
            cerr << "error: select(failed to get request from client)" << endl;
            close(client_fd);
            return;
        }
        if(FD_ISSET(client_fd, &readfds)){
            size_t rev_sz;
            size_t rev_total_sz = 0;
            char buffer[65535];
            string message;
            char end[] = "\r\n\r\n";
            //received a header from the client
            while(!strstr(buffer, end)) {
                rev_sz = recv(client_fd, buffer, 65535, 0);
                message.append(buffer, 0, rev_sz);
                rev_total_sz += rev_sz;
            }

            cache_mtx.lock();
            Request* request = Cache::makeRequest(Parse::parseRequest(message, client_IP));
            cache_mtx.unlock();
           
            //GET
            if(request->getAction() == "GET"){
                service_GET(client_fd, *request, rev_total_sz, message);
            }
            //CONNECT
            else if(request->getAction() == "CONNECT"){
                service_CONNECT(client_fd, *request);
            }
            //POST
            else if(request->getAction() == "POST"){
                vector<char> post;
                for(size_t i = 0; i < rev_total_sz; ++i){
                    post.push_back(buffer[i]);
                }
                pair<string, size_t> type_len = Parse::parseContentLength(post);
                size_t len = type_len.second;
                vector<char> buffer2;
                while(rev_total_sz < len) {
                    rev_sz = recv(client_fd, &(buffer2[0]), 65534, 0);
                    vector<char>::iterator it = buffer2.begin() + rev_sz;
                    post.insert(post.end(), buffer2.begin(), it);
                    rev_total_sz += rev_sz;
                }
                service_POST(client_fd, *request, rev_total_sz, post);
            }
        }
        close(client_fd);
        return;    
    }
};

