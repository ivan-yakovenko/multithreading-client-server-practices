#include "Server.h"

int main() {
    auto server = Server::init();
    if(server) {
        server->acceptClient();
    }
    else {
        return -1;
    }
}