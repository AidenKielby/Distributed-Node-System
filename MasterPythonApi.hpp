#pragma once
#include <string>
#include <vector>
#include <future>

// Start the master server
void initMaster();

// kill master server when done
void killMaster();

// Get connected clients
std::string listActiveClients();

// Send file to a client
std::future<std::string> sendToClientAsync(int clientIndex, const std::string& filePath, const std::string& funcName);

// ping clients
void pingClient(int clientIndex);