#include "newClient.h"

int main() {
    std::unique_ptr<Client> client = Client::init();
    if(client) {
        client->acceptServer();
        client->handleCommands();
    }
    else {
        return -1;
    }
}
