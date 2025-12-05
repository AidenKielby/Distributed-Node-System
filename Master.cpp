#include <iostream>
#include <cstring>
#include <string>

#include <fstream>
#include <vector>

#include <thread>
#include <WinSock2.h>
#include <ws2tcpip.h>

#include <mutex>
#include <future>
#include <chrono>
#include <atomic>
#include <algorithm>
#pragma comment(lib, "ws2_32.lib")

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

bool running = true;
std::vector<SOCKET> clients;
std::mutex clientMutex;

std::vector<std::string> clientNames;
std::mutex clientNameMutex;

std::mutex ioMutex;

#ifdef _WIN32
#include <windows.h>

// enable virtual terminal processing if possible (safe at function scope)
inline void enableVirtualTerminalProcessing() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode)) return;
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
}
#else
inline void enableVirtualTerminalProcessing() {}
#endif

void removeClient(SOCKET sock){
    std::lock(clientMutex, clientNameMutex);
    std::lock_guard<std::mutex> lock1(clientMutex, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(clientNameMutex, std::adopt_lock);

    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i] == sock) {
            closesocket(clients[i]);
            clients.erase(clients.begin() + i);
            if (i < clientNames.size()) {
                clientNames.erase(clientNames.begin() + i);
            }
            break; // stop after removing
        }
    }
}

bool recv_all (SOCKET socket, char* buf, size_t len){
    size_t recvd = 0;
    while (recvd<len){
        int n = recv(socket, buf+recvd, (int)std::min<size_t>(INT32_MAX, len-recvd), 0);
        if (n <= 0) {
            removeClient(socket);
            return false;
        } 
        if (n == SOCKET_ERROR){ // error occurred
            removeClient(socket);
            return false;
        }
        recvd += n;
    }

    return true;
}

bool is_positive_integer(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c){ return std::isdigit(c); });
}

int sendFile(const std::string& filePath, SOCKET socket, const std::string& funcName){
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Cannot open file!\n";
        return 1;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // send file size
    uint32_t filesize_net = htonl(fileSize);
    int val = send(socket, (char*)&filesize_net, sizeof(filesize_net), 0);

    if (val<=0){
        removeClient(socket);
        return 1;
    }
    
    uint32_t fnameLen = htonl(funcName.size());
    send(socket, (char*)&fnameLen, 4, 0);
    send(socket, funcName.c_str(), funcName.size(), 0);
    
    // now the file
    char buffer[4096]; // 4KB buffer 
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));
        std::streamsize bytesRead = file.gcount();
        val = send(socket, buffer, bytesRead, 0);
        if (val<=0){
            removeClient(socket);
            return 1;
        }
    }

    return 0;
}

void handleClient(SOCKET masterSocket){
    while(running){
        SOCKET client = accept(masterSocket, nullptr, nullptr);
        if (client == INVALID_SOCKET){
            {
                std::lock_guard<std::mutex> lock(ioMutex);
                std::cerr << "Accept failed: " << WSAGetLastError();
            }
        }
        else{
            {

                std::string clientName(64, '\0');
                recv_all(client, &clientName[0], clientName.size());

                std::lock(clientMutex, clientNameMutex);
                std::lock_guard<std::mutex> lock1(clientMutex, std::adopt_lock);
                std::lock_guard<std::mutex> lock2(clientNameMutex, std::adopt_lock);

                clients.push_back(client);
                clientNames.push_back(clientName);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void sendToClient(SOCKET client, std::string filePath, std::string funcName){
    const char type = 's';
    send(client, &type, 1, 0);

    if (sendFile(filePath, client, funcName) == 0){
        uint32_t size;
        if (!recv_all(client, (char*)&size, 4)) return;
        size = ntohl(size);

        const uint32_t size1 = size;

        std::vector<char> buf(size + 1);
        recv_all(client, buf.data(), size);

        {
            std::lock_guard<std::mutex> lock(ioMutex);
            std::cout << "\033[31;36mReturned value: " << buf.data() << "\033[0m" << std::endl;
        }
        
    }
    else{
        {
            std::lock_guard<std::mutex> lock(ioMutex);
            std::cout <<"An error occured while sending the file" << std::endl;
        }
    }
            
}

void pingClient(SOCKET client){
    const char type = 'p';
    if (send(client, &type, 1, 0) <= 0) {
        removeClient(client);
        return;
    }
    char resp;
    recv_all(client, &resp, 1);
}

void handleInputs(){
    std::string command;
    while (running){
        std::getline(std::cin, command);

        if (command == "quit") running = false;
        else if (command == "client_list" || command == "clientl"){
            std::cout << "Connected clients (indexes start at 0):\n";
            {
                std::lock(clientMutex, clientNameMutex);
                std::lock_guard<std::mutex> lock1(clientMutex, std::adopt_lock);
                std::lock_guard<std::mutex> lock2(clientNameMutex, std::adopt_lock);

                {
                    std::lock_guard<std::mutex> lock(ioMutex);
                    for (int i = 0; i<clients.size(); i++) {
                        std::cout << " * Client socket: " << clients[i] << ", " << clientNames[i] << "\n";
                    }
                }
            }
                
        }
        else if (is_positive_integer(command)){

            SOCKET client;
            {
                std::lock_guard<std::mutex> lock(clientMutex);

                int index = std::stoi(command);
                if (index < 0 || index >= clients.size()) {
                    std::cout << "Invalid client index\n";
                    return;
                }
                client = clients[index];
            }

            // ask what file
            std::string filePath;
            {
                std::lock_guard<std::mutex> lock(ioMutex);
                std::cout << "what file would you like to send? ";
            }

            std::getline(std::cin, filePath); // std::ws eats leftover newline

            // function name
            std::string funcName;
            {
                std::lock_guard<std::mutex> lock(ioMutex);
                std::cout << "what function will the client execute? (only include the parenthesis if you have an input)  " << std::endl;
            }
            std::getline(std::cin, funcName); // read whole line (allows spaces, parentheses)

            std::thread SendingThread(sendToClient, client, filePath, funcName);
            SendingThread.detach();
            {
                std::lock_guard<std::mutex> lock(ioMutex);
                std::cout << "sent... feel free to continue working here. Just dont use the same client please, that would be stupid. " << std::endl;
            }
        }
        else if (command == "ping_all" || command == "pinga"){
            {
                for (int clientIndex = 0; clientIndex < clients.size(); clientIndex++){
                    pingClient(clients[clientIndex]);
                }
            }
        }
        else{
            {
                std::lock_guard<std::mutex> lock(ioMutex);
                std::cout << "unknown command: " << command << std::endl;
                std::cout << "Available Commands: client_list/clientl, any integer value coresponding to a client eg. '1', ping_all/pinga" << std::endl;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}



int main(){
    enableVirtualTerminalProcessing();
    std::cout << "\033[1;36m";
    std::cout << "\033[1;5H";  // Position cursor
    std::cout << "\033[2J";    // Clear screen
    std::cout << "\033[3m";    // Italic
    std::cout << "Welcome to PCTree\n";
    std::cout << "Distributed Node System Master\033[0m\n" << std::endl;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError();
        return 1;
    }

    SOCKET masterSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (masterSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError();
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(masterSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError();
        closesocket(masterSocket);
        WSACleanup();

        return 1;
    }

    listen(masterSocket, SOMAXCONN);

    std::cout << "Available Commands: client_list/clientl, any integer value coresponding to a client eg. '1', ping_all/pinga" << std::endl;

    std::thread ClientsThread(handleClient, masterSocket);
    std::thread InputsThread(handleInputs);

    ClientsThread.detach();
    InputsThread.join();

    for (size_t i = 0; i < clients.size(); ++i)
        closesocket(clients[i]);
    
    closesocket(masterSocket);
    WSACleanup();

    return 0;
}

