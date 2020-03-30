#include "proxy.h"
#include "service.cpp"

using namespace std;

//--------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[]){
    if (argc != 2) {
        cout << "Syntax: proxy <port_num>\n" << endl;
        return -1;
    }
    //create the server socket
    int status;
    int socket_fd;
    struct addrinfo host_info;
    struct addrinfo *host_info_list;
    const char *hostname = NULL;
    const char *port     = argv[1];
    memset(&host_info, 0, sizeof(host_info));
    host_info.ai_family   = AF_UNSPEC;
    host_info.ai_socktype = SOCK_STREAM;
    host_info.ai_flags    = AI_PASSIVE;
    status = getaddrinfo(hostname, port, &host_info, &host_info_list);
    if (status != 0) {
        string error_message = "no-id: ERROR cannot get address info for host";
        Writter::write(error_message);
        return -1;
    }
    socket_fd = socket(host_info_list->ai_family, 
                host_info_list->ai_socktype, 
                host_info_list->ai_protocol);
    if (socket_fd == -1) {
        string error_message = "no-id: ERROR cannot create socket";
        Writter::write(error_message);
        return -1;
    } 

    //bind
    int yes = 1;
    status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    status = ::bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
    if (status == -1) {
        string error_message = "no-id: ERROR cannot bind socket";
        Writter::write(error_message);
        return -1;
    } 

    //listen
    status = listen(socket_fd, 1000);
    if (status == -1) {
        string error_message = "no-id: ERROR cannot listen on socket";
        Writter::write(error_message);
        return -1;
    } 
   
    //receive a request from the firefox---------------------------------------------------------------
    int cnt = 0;
    while(1){
        cout << " -------------------------- " << cnt << " -------------------------- " << endl;
        struct sockaddr_in socket_addr;
        socklen_t socket_addr_len = sizeof(socket_addr);
        int client_connection_fd = 0;
        while(client_connection_fd == 0){
            client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
            if (client_connection_fd == -1) {
                string error_message = "no-id: ERROR cannot accept connection on socket";
                Writter::write(error_message);
                continue;
            } //accept the connections from firefox
        }
        cout << "connected with the firefox successfully, I am listening ..." << endl;
        string firefox_IP = inet_ntoa(socket_addr.sin_addr);
        std::unique_ptr<int> x = std::make_unique<int>(cnt);
        thread obj(Service::service, client_connection_fd, ref(firefox_IP), std::move(x));
        obj.detach();
        cnt++;
    }

    freeaddrinfo(host_info_list);
    close(socket_fd);
    return 0;
}
