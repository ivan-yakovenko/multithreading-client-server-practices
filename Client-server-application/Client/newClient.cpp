#include "newClient.h"

Client::Client() : port(8080), serverIP("127.0.0.1"), serverAddress({}), clientSocket(0), successfulHandshake(false),
                   version("v2.0") {}

std::unique_ptr<Client> Client::init() {

    std::unique_ptr<Client> client = std::make_unique<Client>();

    std::this_thread::sleep_for(std::chrono::seconds(2));

    client->clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (client->clientSocket == -1) {
        std::cerr << "Error while creating a socket\n";
        return nullptr;
    }

    client->serverAddress.sin_family = AF_INET;
    client->serverAddress.sin_port = htons(client->port);
    inet_pton(AF_INET, client->serverIP.c_str(), &(client->serverAddress.sin_addr));

    if (connect(client->clientSocket, reinterpret_cast<struct sockaddr *>(&client->serverAddress),
                sizeof(serverAddress)) == -1) {
        std::cerr << "Error while connecting...\n";
        return nullptr;
    }

    return client;
}

void Client::acceptServer() {
    uint8_t messageLength = version.length();
    send(clientSocket, &messageLength, sizeof(messageLength), 0);
    send(clientSocket, version.c_str(), version.length(), 0);

    uint16_t bufferLength;
    recv(clientSocket, &bufferLength, sizeof(bufferLength), 0);
    bufferLength = ntohs(bufferLength);

    char *buffer = new char[bufferLength + 1];
    recv(clientSocket, buffer, bufferLength, 0);
    buffer[bufferLength] = '\0';

    std::cout << buffer;

    delete[] buffer;

}

Client::~Client() {
    close(clientSocket);
}

void Client::createFolder(std::string &clientName) {
    std::filesystem::create_directory(clientName);
}

void Client::performHandshake(std::string &message) {

    if (successfulHandshake) {
        return;
    }

    uint32_t messageLength = htonl(message.length());
    send(clientSocket, &messageLength, sizeof(messageLength), 0);
    send(clientSocket, message.c_str(), message.length(), 0);

    uint16_t bufferLength;
    recv(clientSocket, &bufferLength, sizeof(bufferLength), 0);
    bufferLength = ntohs(bufferLength);

    char *buffer = new char[bufferLength + 1];
    recv(clientSocket, buffer, bufferLength, 0);
    buffer[bufferLength] = '\0';

    std::string properResponse = "Verification passed, could you type your name please!";
    std::string nameExists = "Such name already exists, try something else";

    std::cout << "Received message from server: " << buffer << "\n";
    if (strcmp(buffer, properResponse.c_str()) == 0) {
        delete[] buffer;
        std::getline(std::cin, name);
        uint32_t nameLength = htonl(name.length());
        send(clientSocket, &nameLength, sizeof(nameLength), 0);
        send(clientSocket, name.c_str(), name.length(), 0);

        uint32_t newBufferLength;
        recv(clientSocket, &newBufferLength, sizeof(newBufferLength), 0);
        newBufferLength = ntohl(newBufferLength);

        char *newBuffer = new char[newBufferLength + 1];
        recv(clientSocket, newBuffer, newBufferLength, 0);
        newBuffer[newBufferLength] = '\0';



        while(strcmp(newBuffer, nameExists.c_str()) == 0) {
            std::cout << newBuffer << "\n";
            delete[] newBuffer;

            std::getline(std::cin, name);
            nameLength = htonl(name.length());
            send(clientSocket, &nameLength, sizeof(nameLength), 0);
            send(clientSocket, name.c_str(), name.length(), 0);

            recv(clientSocket, &newBufferLength, sizeof(newBufferLength), 0);
            newBufferLength = ntohl(newBufferLength);

            newBuffer = new char[newBufferLength + 1];
            recv(clientSocket, newBuffer, newBufferLength, 0);
            newBuffer[newBufferLength] = '\0';

        }

        std::cout << newBuffer << "\n";

        delete[] newBuffer;
        createFolder(name);
        successfulHandshake = true;
    }
}


void Client::handleCommands() {
    bool firstIteration = false;
    while (true) {
        std::string input;
        std::getline(std::cin, input);
        performHandshake(input);
        if (successfulHandshake) {
            if (!firstIteration) {
                firstIteration = true;
                continue;
            }
            std::istringstream commands(input);
            std::string command, clientName, filename;
            commands >> command >> clientName;
            if (command == "GET") {
                commands >> filename;
                if (clientName.empty()) {
                    std::cerr << "Specify client name!\n";
                    continue;
                }
                if (filename.empty()) {
                    std::cerr << "Specify filename!\n";
                    continue;
                }
                requestGET(filename, clientName);
            }
            if (command == "LIST") {
                if (clientName.empty()) {
                    std::cerr << "Specify client name!\n";
                    continue;
                }
                requestLIST(clientName);
            }
            if (command == "PUT") {
                if (clientName != name) {
                    std::string accessDenied = "Access denied to the folder " + clientName + "\n";
                    std::cerr << accessDenied;
                    continue;
                }
                commands >> filename;
                if (clientName.empty()) {
                    std::cerr << "Specify client name!\n";
                    continue;
                }
                if (filename.empty()) {
                    std::cerr << "Specify filename!\n";
                    continue;
                }
                requestPUT(filename, clientName);
            }
            if (command == "DELETE") {
                commands >> filename;
                if (clientName.empty()) {
                    std::cerr << "Specify client name!\n";
                    continue;
                }
                if (filename.empty()) {
                    std::cerr << "Specify filename!\n";
                    continue;
                }
                requestDELETE(filename, clientName);
            }
            if (command == "INFO") {
                commands >> filename;
                if (clientName.empty()) {
                    std::cerr << "Specify client name!\n";
                    continue;
                }
                if (filename.empty()) {
                    std::cerr << "Specify filename!\n";
                    continue;
                }
                requestINFO(filename, clientName);
            }
            if (command == "STOP") {
                break;
            }
            if (!existingCommand(command)) {
                std::cout << "No such command, try again\n";
            }
        }
    }
}

void Client::requestGET(std::string &filename, std::string &clientName) const {
    std::string request = "GET " + clientName + " " + filename;

    uint32_t requestLength = htonl(request.length());
    send(clientSocket, &requestLength, sizeof(requestLength), 0);
    send(clientSocket, request.c_str(), request.length(), 0);

    uint32_t bufferLength;
    recv(clientSocket, &bufferLength, sizeof(bufferLength), 0);
    bufferLength = ntohl(bufferLength);

    char *buffer = new char[bufferLength + 1];
    recv(clientSocket, buffer, bufferLength, 0);
    buffer[bufferLength] = '\0';

    std::string response(buffer);
    std::string notAvailable = "The file " + filename + " is not available in this directory";
    std::string wrongFolder = "Access denied to the folder " + filename + "\n";
    if (response == notAvailable || response == wrongFolder) {
        std::cout << buffer;
        delete[] buffer;
        return;
    }

    std::string filePath = clientName + "/" + filename;

    std::ofstream fout(filePath);

    do {
        fout.write(buffer, bufferLength);
        delete[] buffer;
        if (recv(clientSocket, &bufferLength, sizeof(bufferLength), 0) <= 0) break;
        bufferLength = ntohl(bufferLength);
        if (bufferLength == 0) break;
        buffer = new char[bufferLength];
        recv(clientSocket, buffer, bufferLength, 0);
    } while (true);

    std::cout << "File has been received successfully\n";

    fout.close();
}

void Client::requestLIST(std::string &clientName) const {
    std::string request = "LIST " + clientName;

    uint32_t requestLength = htonl(request.length());
    send(clientSocket, &requestLength, sizeof(requestLength), 0);
    send(clientSocket, request.c_str(), request.length(), 0);

    uint32_t bufferLength;
    recv(clientSocket, &bufferLength, sizeof(bufferLength), 0);
    bufferLength = ntohl(bufferLength);

    char *buffer = new char[bufferLength + 1];
    ssize_t bytesReceived = recv(clientSocket, buffer, bufferLength, 0);
    buffer[bufferLength] = '\0';
    std::string info;
    info.append(buffer, bytesReceived);

    delete[] buffer;

    std::istringstream filesStream(info);
    std::string line;

    std::cout << "Received info from the server:\n";
    while (std::getline(filesStream, line)) {
        std::cout << line << "\n";
    }
}

void Client::requestPUT(std::string &filename, std::string &clientName) const {
    std::string request = "PUT " + clientName + " " + filename;
    std::string filePath = "./" + clientName + "/" + filename;

    if (!std::filesystem::exists(filePath)) {
        std::cout << "The file " + filename + " is not available in this directory\n";
        return;
    }

    uint32_t requestLength = htonl(request.length());
    send(clientSocket, &requestLength, sizeof(requestLength), 0);
    send(clientSocket, request.c_str(), request.length(), 0);
    std::ifstream fin(filePath);

    char buffer[1024];

    while (fin.read(buffer, sizeof(buffer)) || fin.gcount() > 0 && fin.gcount() < 1024) {
        uint32_t bufferLength = htonl(uint32_t(fin.gcount()));
        send(clientSocket, &bufferLength, sizeof(bufferLength), 0);
        send(clientSocket, buffer, size_t(fin.gcount()), 0);
    }

    uint32_t bufferLength = 0;
    send(clientSocket, &bufferLength, sizeof(bufferLength), 0);

    uint16_t responseBufferLength;
    recv(clientSocket, &responseBufferLength, sizeof(responseBufferLength), 0);
    responseBufferLength = ntohs(responseBufferLength);

    char *responseBuffer = new char[responseBufferLength + 1];
    recv(clientSocket, responseBuffer, responseBufferLength, 0);
    responseBuffer[responseBufferLength] = '\0';

    std::cout << responseBuffer;

    delete[] responseBuffer;

    fin.close();
}

void Client::requestDELETE(std::string &filename, std::string &clientName) const {
    std::string request = "DELETE " + clientName + " " + filename;
    uint32_t requestLength = htonl(request.length());
    send(clientSocket, &requestLength, sizeof(requestLength), 0);
    send(clientSocket, request.c_str(), request.length(), 0);

    uint32_t bufferLength;
    recv(clientSocket, &bufferLength, sizeof(bufferLength), 0);
    bufferLength = ntohl(bufferLength);

    char *buffer = new char[bufferLength + 1];
    recv(clientSocket, buffer, bufferLength, 0);
    buffer[bufferLength] = '\0';

    std::cout << buffer;

    delete[] buffer;
}

void Client::requestINFO(std::string &filename, std::string &clientName) const {
    std::string request = "INFO " + clientName + " " + filename;
    uint32_t requestLength = htonl(request.length());
    send(clientSocket, &requestLength, sizeof(requestLength), 0);
    send(clientSocket, request.c_str(), request.length(), 0);

    uint32_t bufferLength;
    recv(clientSocket, &bufferLength, sizeof(bufferLength), 0);
    bufferLength = ntohl(bufferLength);

    char *buffer = new char[bufferLength + 1];
    ssize_t bytesReceived = recv(clientSocket, buffer, bufferLength, 0);
    buffer[bufferLength] = '\0';
    std::string info;

    info.append(buffer, bytesReceived);

    delete[] buffer;

    std::istringstream infoStream(info);
    std::string line;

    while (std::getline(infoStream, line)) {
        std::cout << line << "\n";
    }

}


bool Client::existingCommand(std::string &command) {
    return command == "GET" || command == "LIST" || command == "PUT" || command == "DELETE" || command == "INFO" ||
           command == "STOP";
}