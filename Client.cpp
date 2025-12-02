#include <iostream>
#include <cstring>
#include <fstream>
#include <cstdint>
#include <Python.h>

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <crtdbg.h>
#include <direct.h>
#include <string>
#include <Windows.h>
#include <thread>
#include <fstream>
#include "resource.h"
#pragma comment(lib, "ws2_32.lib")

bool running = true;
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

std::string runFile(const std::string& filePath, const std::string& functionName){

    std::string fn = functionName;
    bool hasParamiters = false;
    std::string paramStr;
    size_t start = functionName.find('(');
    size_t end = functionName.find(')');
    if (start != std::string::npos && end != std::string::npos && end > start) {
        fn = functionName.substr(0, start);       // function name without parentheses
        paramStr = functionName.substr(start + 1, end - start - 1);  // the parameters inside
        hasParamiters = true;
    }

    PyObject *name, *load_module, *func, *callFunc, *args;

    PyRun_SimpleString("import sys; sys.path.insert(0, '.')");

    std::string moduleName = filePath;
    if (moduleName.size() > 3 && moduleName.substr(moduleName.size()-3) == ".py") {
        moduleName = moduleName.substr(0, moduleName.size()-3);
    }

    PyObject *sys = PyImport_ImportModule("sys");
    PyObject *modules = PyObject_GetAttrString(sys, "modules");

    PyObject *modNameObj = PyUnicode_FromString(moduleName.c_str());
    if (PyDict_Contains(modules, modNameObj)) {
        PyDict_DelItem(modules, modNameObj);
    }
    Py_XDECREF(modNameObj);

    Py_XDECREF(modules);
    Py_XDECREF(sys);

    name = PyUnicode_FromString(moduleName.c_str());
    if (!name) { PyErr_Print(); return ""; }
    
    load_module = PyImport_Import(name);
    Py_XDECREF(name);
    if (!load_module) { PyErr_Print(); return ""; }
    
    func = PyObject_GetAttrString(load_module, fn.c_str());
    if (!func) {
        PyErr_Print();
        Py_XDECREF(load_module);
        return "";
    }

    if (!PyCallable_Check(func)) {
        std::cerr << "Requested attribute is not callable\n";
        Py_XDECREF(func);
        Py_XDECREF(load_module);
        return "";
    }   
    
    PyObject* args1 = nullptr;
    if (hasParamiters) {
        PyObject* globals = PyDict_New();
        PyDict_SetItemString(globals, "__builtins__", PyEval_GetBuiltins());

        std::string tupleStr = "(" + paramStr + ")";
        args1 = PyRun_String(tupleStr.c_str(), Py_eval_input, globals, globals);

        Py_XDECREF(globals);
        if (!args1) {
            PyErr_Print();
            Py_XDECREF(func);
            Py_XDECREF(load_module);
            return "";
        }
    }

    callFunc = PyObject_CallObject(func, args1);
    Py_XDECREF(args1);

    if (!callFunc) {
        PyErr_Print();
        Py_XDECREF(func);
        Py_XDECREF(load_module);
        return "";
    }

    PyObject* strObj = PyObject_Str(callFunc);

    if (!strObj) {
        PyErr_Print();
        Py_XDECREF(callFunc);
        Py_XDECREF(func);
        Py_XDECREF(load_module);
        return "";
    }
    
    const char* c = PyUnicode_AsUTF8(strObj);
    std::string result = c ? c : "";

    Py_XDECREF(strObj);
    Py_XDECREF(callFunc);
    Py_XDECREF(func);
    Py_XDECREF(load_module);
    
    return result;
}

std::string recvFile(std::string filePath, SOCKET socket){
    uint32_t filesize_net;
    recv(socket, (char*)&filesize_net, sizeof(filesize_net), 0);
    size_t filesize = ntohl(filesize_net);

    std::ofstream outfile(filePath, std::ios::binary | std::ios::trunc);

    // recv func name
    uint32_t fnameLen_net;
    recv(socket, (char*)&fnameLen_net, 4, 0);
    uint32_t fnameLen = ntohl(fnameLen_net);

    std::string functionName(fnameLen, 0);
    recv(socket, &functionName[0], fnameLen, 0);

    // recv file
    size_t bytesReceived = 0;
    char buffer[4096];

    while (bytesReceived < filesize) {
        int chunk = recv(socket, buffer, sizeof(buffer), 0);
        if (chunk <= 0) {
            std::cerr << "Connection closed or error!\n";
            running = false;
            break;
        }
        outfile.write(buffer, chunk);
        bytesReceived += chunk;
    }

    outfile.close(); 

    std::string funcName(functionName);

    std::string returnedValue = runFile(filePath, funcName);

    std::cout << "returned value: " << returnedValue << std::endl;

    return returnedValue;

}


std::string getComputerName() {
    DWORD size = 0;
    // Get required buffer size
    GetComputerNameExA(ComputerNamePhysicalDnsHostname, nullptr, &size);
    
    if (size == 0) {
        std::cerr << "Error determining buffer size: " << GetLastError() << std::endl;
        return "unknown";
    }

    std::string name(size, '\0'); // allocate buffer
    if (GetComputerNameExA(ComputerNamePhysicalDnsHostname, &name[0], &size)) {
        name.resize(size); // remove extra nulls
        return name;
    } else {
        std::cerr << "Error getting computer name: " << GetLastError() << std::endl;
        return "unknown";
    }
}

int main(){
    // startup

    HRSRC res = FindResource(NULL, MAKEINTRESOURCE(PYTHONZIP), RT_RCDATA);
    HGLOBAL resData = LoadResource(NULL, res);
    DWORD size = SizeofResource(NULL, res);
    void* zipData = LockResource(resData);

    // Write to a real file
    std::ofstream out("python_embedded.zip", std::ios::binary);
    out.write((char*)zipData, size);
    out.close();

    Py_Initialize();
    std::cout << "Do you accept for your computer to be a 'client' for the PCTree? (y/n)" << std::endl;

    std::string answer;
    std::cin >> answer;
    if (answer == "N" || answer == "n"){
        return 1;
    }


    bool systems[] = {0, 0};

    std::cout << "Does your system have Python installed? (y/n)" << std::endl;

    std::cin >> answer;
    if (answer == "Y" || answer == "y"){
        systems[0] = 1;
    }

    if (systems[0] == 0){
        std::cout << "Your system does not meet the requierments" << std::endl;
        return 1;
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup failed: " << WSAGetLastError();
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Error creating socket: " << WSAGetLastError();
        WSACleanup();
        return 1;
    }

    std::string adress;
    std::cout << "what is the ip adress? " << std::endl;
    std::cin >> adress;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, adress.c_str(), &addr.sin_addr);

    if (connect(clientSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Connect failed: " << WSAGetLastError();
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // get PC name
    std::string name = getComputerName();
    name.resize(64, '\0'); // pad to 64 bytes
    send(clientSocket, name.c_str(), name.size(), 0);

    // rest of the stuff
    while(running){
        char type;
        recv(clientSocket, &type, 1, 0);
        if (type == 's'){
            std::string result = recvFile("newFile.py", clientSocket);

            uint32_t size = htonl(result.size());
            send(clientSocket, (char*)&size, 4, 0);
            send(clientSocket, result.c_str(), result.size(), 0);
        }
        else{
            send(clientSocket, "p", 1, 0);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }
    Py_Finalize();

    closesocket(clientSocket);
    WSACleanup();
}