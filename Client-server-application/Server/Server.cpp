#include "Server.h"

int Server::GETcounter = 0;
int Server::LISTcounter = 0;
int Server::PUTcounter = 0;
int Server::INFOcounter = 0;
int Server::DELETEcounter = 0;

Server::Server() : port(8080), serverAddress({}), serverSocket(0) {}

std::unique_ptr<Server> Server::init() {

    std::unique_ptr<Server> server = std::make_unique<Server>();

    server->serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (server->serverSocket == -1) {
        std::cerr << "Error while creating a socket\n";
        return nullptr;
    }

    server->serverAddress.sin_family = AF_INET;
    server->serverAddress.sin_port = htons(server->port);
    server->serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(server->serverSocket, reinterpret_cast<struct sockaddr *>(&server->serverAddress),
             sizeof(serverAddress)) == -1) {
        std::cerr << "Binding failed\n";
        return nullptr;
    }

    if (listen(server->serverSocket, SOMAXCONN) == -1) {
        std::cerr << "Listening failed\n";
        return nullptr;
    }

    std::cout << "Server is listening on the port: " << server->port << "\n";

    return server;
}

void Server::acceptClient() {
    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);

        int clientSocket = accept(serverSocket, reinterpret_cast<struct sockaddr *>(&clientAddress),
                                  &clientAddressLength);
        if (clientSocket == -1) {
            std::cerr << "Acception failed\n";
            return;
        }

        std::cout << "Accepted connection from " << inet_ntoa(clientAddress.sin_addr) << ":"
                  << ntohs(clientAddress.sin_port) << "\n";

        uint8_t bufferLength;
        recv(clientSocket, &bufferLength, sizeof(bufferLength), 0);

        char *clientVersion = new char[bufferLength + 1];
        recv(clientSocket, clientVersion, bufferLength, 0);
        clientVersion[bufferLength] = '\0';

        std::cout << clientVersion << "\n";

        clients.emplace_back(&Server::handleClient, this, clientSocket, clientAddress, clientVersion);

        std::string serverGreeting = "Hi client, this is server, input the message for verification\n";

        uint16_t messageLength = htons(serverGreeting.length());
        send(clientSocket, &messageLength, sizeof(messageLength), 0);
        send(clientSocket, serverGreeting.c_str(), serverGreeting.length(), 0);

    }
}

Server::~Server() {
    close(serverSocket);
    for (auto &client: clients) {
        if (client.joinable()) {
            client.join();
        }
    }
}

void Server::handleClient(int clientSocket, sockaddr_in clientAddress, char *cVersion) {
    handleCommands(clientSocket, clientAddress, cVersion);
    delete[] cVersion;
    close(clientSocket);
    commandsStatistics();
}

void Server::createFolder(std::string &clientName) {
    std::filesystem::create_directory(clientName);

    for (const auto &file: std::filesystem::directory_iterator("data/")) {
        const auto &filePath = file.path();
        std::filesystem::copy(filePath, clientName + "/" + filePath.filename().string(),
                              std::filesystem::copy_options::overwrite_existing);
    }
}

void Server::performHandshake(int clientSocket, sockaddr_in clientAddress, bool &successfulHandshake,
                              std::string &clientName, char *cVersion) {

    if (successfulHandshake) return;

    std::string properMessage = "Hi server!";

    std::string properMessage2 = "Hello server!";

    std::string responseWrong = "Incorrect message, try again";

    std::string responseProper = "Verification passed, could you type your name please!";

    std::string responseProper2 = "Verification passed, now you can perform some commands!";

    std::string approvedClient = "Good! Now you can perform some commands, ";

    std::string nameExists = "Such name already exists, try something else";

    while (true) {

        uint32_t bufferLength;
        ssize_t bytesReceived = recv(clientSocket, &bufferLength, sizeof(bufferLength), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Client " << inet_ntoa(clientAddress.sin_addr) << ":"
                                   << ntohs(clientAddress.sin_port) << " disconnected\n";
            break;
        }
        bufferLength = ntohl(bufferLength);

        char *buffer = new char[bufferLength + 1];
        recv(clientSocket, buffer, bufferLength, 0);
        buffer[bufferLength] = '\0';

        std::cout << "Received message from: " << inet_ntoa(clientAddress.sin_addr) << ":"
                  << ntohs(clientAddress.sin_port) << " " << buffer << "\n";
        if (strcmp(cVersion, "v1.0") == 0) {
            if (strcmp(buffer, properMessage.c_str()) == 0) {
                successfulHandshake = true;
                uint16_t messageLength = htons(responseProper2.length());
                send(clientSocket, &messageLength, sizeof(messageLength), 0);
                send(clientSocket, responseProper2.c_str(), responseProper2.length(), 0);
                delete[] buffer;
                break;
            } else {
                uint16_t messageLength = htons(responseWrong.length());
                send(clientSocket, &messageLength, sizeof(messageLength), 0);
                send(clientSocket, responseWrong.c_str(), responseWrong.length(), 0);
                delete[] buffer;
            }
        }
        if (strcmp(cVersion, "v2.0") == 0) {
            if (strcmp(buffer, properMessage2.c_str()) == 0) {
                uint16_t messageLength = htons(responseProper.length());
                send(clientSocket, &messageLength, sizeof(messageLength), 0);
                send(clientSocket, responseProper.c_str(), responseProper.length(), 0);
                delete[] buffer;
                while (true) {
                    uint32_t newBufferLength;
                    ssize_t newBytesReceived = recv(clientSocket, &newBufferLength, sizeof(newBufferLength), 0);
                    if (newBytesReceived <= 0) {
                        std::cerr << "Client " << clientName << " disconnected\n";
                        break;
                    }
                    newBufferLength = ntohl(newBufferLength);

                    char *newBuffer = new char[newBufferLength + 1];
                    recv(clientSocket, newBuffer, newBufferLength, 0);
                    newBuffer[newBufferLength] = '\0';

                    clientName = newBuffer;

                    if (std::find(clientNames.begin(), clientNames.end(), clientName) !=
                        clientNames.end()) {
                        uint32_t nameExistsLength = htonl(nameExists.length());
                        send(clientSocket, &nameExistsLength, sizeof(nameExistsLength), 0);
                        send(clientSocket, nameExists.c_str(), nameExists.length(), 0);
                        delete[] newBuffer;
                    }
                    else {
                        clientNames.push_back(clientName);
                        successfulHandshake = true;
                        std::string nameReceived = approvedClient + newBuffer + "!";
                        uint32_t nameReceivedLength = htonl(nameReceived.length());
                        send(clientSocket, &nameReceivedLength, sizeof(nameReceivedLength), 0);
                        send(clientSocket, nameReceived.c_str(), nameReceived.length(), 0);
                        createFolder(clientName);
                        delete[] newBuffer;
                        break;
                    }
                }
                break;
            } else {
                uint16_t messageLength = htons(responseWrong.length());
                send(clientSocket, &messageLength, sizeof(messageLength), 0);
                send(clientSocket, responseWrong.c_str(), responseWrong.length(), 0);
                delete[] buffer;
            }
        }
    }
}


void Server::handleCommands(int clientSocket, sockaddr_in clientAddress, char *cVersion) {
    bool successfulHandshake = false;
    std::string clientName;
    performHandshake(clientSocket, clientAddress, successfulHandshake, clientName, cVersion);
    if (successfulHandshake) {
        while (true) {
            uint32_t bufferLength;
            ssize_t bytesReceived = recv(clientSocket, &bufferLength, sizeof(bufferLength), 0);
            if (bytesReceived <= 0) {
                std::cerr << "Client " << inet_ntoa(clientAddress.sin_addr) << ":"
                                       << ntohs(clientAddress.sin_port) << " disconnected\n";
                break;
            }
            bufferLength = ntohl(bufferLength);

            char *buffer = new char[bufferLength + 1];
            recv(clientSocket, buffer, bufferLength, 0);
            buffer[bufferLength] = '\0';
            std::istringstream commands(buffer);
            std::string command, folderName, filename;
            if (strcmp(cVersion, "v1.0") == 0) {
                std::string directory = "data";
                commands >> command;
                std::cout << "Command processed from " << inet_ntoa(clientAddress.sin_addr) << ":"
                          << ntohs(clientAddress.sin_port) << " : " << buffer << "\n";
                if (command == "GET") {
                    commands >> filename;
                    GETcounterMutex.lock();
                    GETcounter++;
                    GETcounterMutex.unlock();
                    processGET(clientSocket, directory, filename);
                }
                if (command == "LIST") {
                    LISTcounterMutex.lock();
                    LISTcounter++;
                    LISTcounterMutex.unlock();
                    processLIST(clientSocket, directory);
                }
                if (command == "PUT") {
                    commands >> filename;
                    PUTcounterMutex.lock();
                    PUTcounter++;
                    PUTcounterMutex.unlock();
                    processPUT(clientSocket, directory, filename);
                }
                if (command == "DELETE") {
                    commands >> filename;
                    DELETEcounterMutex.lock();
                    DELETEcounter++;
                    DELETEcounterMutex.unlock();
                    processDELETE(clientSocket, directory, filename);
                }
                if (command == "INFO") {
                    commands >> filename;
                    INFOcounterMutex.lock();
                    INFOcounter++;
                    INFOcounterMutex.unlock();
                    processINFO(clientSocket, directory, filename);
                }
                if (command == "STOP") {
                    break;
                }
            } else {
                commands >> command >> folderName;
                std::cout << "Command processed from " << clientName << ": " << buffer << "\n";
                if (folderName != clientName) {
                    std::string accessDenied = "Access denied to the folder " + folderName + "\n";
                    uint32_t accessDeniedLength = htonl(accessDenied.length());
                    send(clientSocket, &accessDeniedLength, sizeof(accessDeniedLength), 0);
                    send(clientSocket, accessDenied.c_str(), accessDenied.length(), 0);
                    continue;
                }
                if (command == "GET") {
                    commands >> filename;
                    GETcounterMutex.lock();
                    GETcounter++;
                    GETcounterMutex.unlock();
                    processGET(clientSocket, folderName, filename);
                }
                if (command == "LIST") {
                    LISTcounterMutex.lock();
                    LISTcounter++;
                    LISTcounterMutex.unlock();
                    processLIST(clientSocket, folderName);
                }
                if (command == "PUT") {
                    commands >> filename;
                    PUTcounterMutex.lock();
                    PUTcounter++;
                    PUTcounterMutex.unlock();
                    processPUT(clientSocket, folderName, filename);
                }
                if (command == "DELETE") {
                    commands >> filename;
                    DELETEcounterMutex.lock();
                    DELETEcounter++;
                    DELETEcounterMutex.unlock();
                    processDELETE(clientSocket, folderName, filename);
                }
                if (command == "INFO") {
                    commands >> filename;
                    INFOcounterMutex.lock();
                    INFOcounter++;
                    INFOcounterMutex.unlock();
                    processINFO(clientSocket, folderName, filename);
                }
                if (command == "STOP") {
                    delete[] buffer;
                    break;
                }
            }
            delete[] buffer;
        }
    }
}


void Server::processGET(int clientSocket, std::string &clientName, std::string &filename) {
    std::string filePath = clientName + "/" + filename;
    std::ifstream fin(filePath);

    if (!std::filesystem::exists(filePath)) {
        std::string notAvailable = "The file " + filename + " is not available in this directory\n";
        uint32_t notAvailableLength = htonl(notAvailable.length());
        send(clientSocket, &notAvailableLength, sizeof(notAvailableLength), 0);
        send(clientSocket, notAvailable.c_str(), notAvailable.length(), 0);
        return;
    }

    char buffer[1024];

    while (fin.read(buffer, sizeof(buffer)) || fin.gcount() > 0) {
        uint32_t bufferLength = htonl(uint32_t(fin.gcount()));
        send(clientSocket, &bufferLength, sizeof(bufferLength), 0);
        send(clientSocket, buffer, size_t(fin.gcount()), 0);
    }

    uint32_t bufferLength = 0;
    send(clientSocket, &bufferLength, sizeof(bufferLength), 0);

    fin.close();

}

void Server::processLIST(int clientSocket, std::string &clientName) {

    std::string dataDirectory = clientName + "/";
    std::string files;

    for (const auto &file: std::filesystem::directory_iterator(dataDirectory)) {
        std::string filename = file.path().filename().string() + "\n";
        files.append(filename);
    }

    uint32_t filesLength = htonl(files.length());
    send(clientSocket, &filesLength, sizeof(filesLength), 0);
    send(clientSocket, files.c_str(), files.length(), 0);

}

void Server::processPUT(int clientSocket, std::string &clientName, std::string &filename) {
    std::string filePath = clientName + "/" + filename;

    std::ofstream fout(filePath);

    while (true) {
        uint32_t bufferLength;
        recv(clientSocket, &bufferLength, sizeof(bufferLength), 0);
        bufferLength = ntohl(bufferLength);
        if (bufferLength == 0) break;

        char *buffer = new char[bufferLength];
        recv(clientSocket, buffer, bufferLength, 0);

        fout.write(buffer, bufferLength);

        delete[] buffer;

    }

    std::string confirmation = "The file was transferred successfully\n";
    uint16_t confirmationLength = htons(confirmation.length());
    send(clientSocket, &confirmationLength, sizeof(confirmationLength), 0);
    send(clientSocket, confirmation.c_str(), confirmation.length(), 0);

    fout.close();
}


void Server::processDELETE(int clientSocket, std::string &clientName, std::string &filename) {

    std::string filePath = clientName + "/" + filename;

    if (std::filesystem::exists(filePath)) {
        remove(filePath.c_str());
        std::string confirmation = "The file " + filename + " was deleted successfully\n";
        uint32_t confirmationLength = htonl(confirmation.length());
        send(clientSocket, &confirmationLength, sizeof(confirmationLength), 0);
        send(clientSocket, confirmation.c_str(), confirmation.length(), 0);
    } else {
        std::string notAvailable = "The file " + filename + " is not available in this directory\n";
        uint32_t notAvailableLength = htonl(notAvailable.length());
        send(clientSocket, &notAvailableLength, sizeof(notAvailableLength), 0);
        send(clientSocket, notAvailable.c_str(), notAvailable.length(), 0);
    }
}

void Server::processINFO(int clientSocket, std::string &clientName, std::string &filename) {
    std::string filePath = clientName + "/" + filename;
    std::string info;

    if (std::filesystem::exists(filePath)) {
        auto fileSize = std::filesystem::file_size(filePath);
        auto lastModified = std::filesystem::last_write_time(filePath);
        auto convertedTime1 = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                lastModified - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
        );
        auto convertedTime2 = std::chrono::system_clock::to_time_t(convertedTime1);

        std::string filePathStr = "File path: " + filePath + "\n";

        std::string fileSizeStr = "File size: " + std::to_string(fileSize) + " bytes\n";

        std::ostringstream time;
        time << std::put_time(std::localtime(&convertedTime2), "%F, " "%T");

        std::string lastModifiedStr = "Last modified: " + time.str() + "\n";

        info.append(filePathStr);
        info.append(fileSizeStr);
        info.append(lastModifiedStr);
    } else {
        std::string notAvailable = "The file " + filename + " is not available in this directory";
        uint32_t notAvailableLength = htonl(notAvailable.length());
        send(clientSocket, &notAvailableLength, sizeof(notAvailableLength), 0);
        send(clientSocket, notAvailable.c_str(), notAvailable.length(), 0);
    }

    uint32_t infoLength = htonl(info.length());
    send(clientSocket, &infoLength, sizeof(infoLength), 0);
    send(clientSocket, info.c_str(), info.length(), 0);

}

void Server::commandsStatistics() {
    std::string filePath = "statistics/stats.txt";
    std::ofstream fout(filePath);

    fout << "GET: " << GETcounter << "\n";
    fout << "LIST: " << LISTcounter << "\n";
    fout << "PUT: " << PUTcounter << "\n";
    fout << "INFO: " << INFOcounter << "\n";
    fout << "DELETE: " << DELETEcounter << "\n";
    fout.close();
}