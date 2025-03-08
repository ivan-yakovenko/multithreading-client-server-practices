#pragma once

#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <fstream>

class Client {
private:
    int port, clientSocket;
    std::string serverIP, version, name;
    sockaddr_in serverAddress;
    bool successfulHandshake;

public:
    Client();

    ~Client();
    
    static std::unique_ptr<Client> init();

    void acceptServer();

    void performHandshake(std::string &message);

    void handleCommands();

    void static createFolder(std::string &clientName);

    void requestGET(std::string &filename, std::string &clientName) const;

    void requestLIST(std::string &clientName) const;

    void requestPUT(std::string &filename, std::string &clientName) const;

    void requestDELETE(std::string &filename, std::string &clientName) const;

    void requestINFO(std::string &filename, std::string &clientName) const;

    bool static existingCommand(std::string &command);

};


