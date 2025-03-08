#pragma once

#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <chrono>
#include <thread>
#include <mutex>

class Server {
private:
    int port, serverSocket;
    sockaddr_in serverAddress;

    std::vector<std::thread> clients;

    std::vector<std::string> clientNames;

    static int GETcounter, LISTcounter, PUTcounter, INFOcounter, DELETEcounter;

    std::mutex GETcounterMutex, LISTcounterMutex, PUTcounterMutex, INFOcounterMutex, DELETEcounterMutex;

public:
    Server();

    static std::unique_ptr<Server> init();

    void acceptClient();

    ~Server();

    void handleClient(int clientSocket, sockaddr_in clientAddress, char *cVersion);

    static void createFolder(std::string &clientName);

    void performHandshake(int clientSocket, sockaddr_in clientAddress, bool &successfulHandshake, std::string &clientName, char *cVersion);

    void handleCommands(int clientSocket, sockaddr_in clientAddress, char *cVersion);

    static void processGET(int clientSocket, std::string &clientName, std::string &filename);

    static void processLIST(int clientSocket, std::string &clientName);

    static void processPUT(int clientSocket, std::string &clientName, std::string &filename);

    static void processDELETE(int clientSocket, std::string &clientName, std::string &filename);

    static void processINFO(int clientSocket, std::string &clientName, std::string &filename);

    static void commandsStatistics();

};


