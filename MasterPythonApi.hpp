#pragma once
#include <string>
#include <vector>

// Start the master server
void initMaster();

// kill master server when done
void killMaster();

// Get connected clients
std::string listActiveClients();

// Send file to a client
std::string sendToClient(int clientIndex, const std::string& filePath, const std::string& funcName);

// ping clients
void pingClient(int clientIndex);