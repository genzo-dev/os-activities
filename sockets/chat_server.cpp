#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <unistd.h>
#include <arpa/inet.h>
#include <algorithm>
using namespace std;

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

vector<int> clientSockets;
mutex clientsMutex;

void broadcastMessage(const string &message, int senderSocket)
{
    lock_guard<mutex> lock(clientsMutex);
    for (int clientSocket : clientSockets)
    {
        if (clientSocket != senderSocket)
        {
            send(clientSocket, message.c_str(), message.size(), 0);
        }
    }
}

void handleClient(int clientSocket)
{
    char buffer[BUFFER_SIZE];
    string welcome = ">> Welcome to the chat! Type 'exit' to leave.\n";
    send(clientSocket, welcome.c_str(), welcome.size(), 0);

    while (true)
    {
        ssize_t bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytesRead <= 0)
        {
            break; // disconnected
        }

        buffer[bytesRead] = '\0';
        string message(buffer);

        if (message == "exit\n" || message == "exit\r\n")
        {
            string notice = ">> A user has left the chat.\n";
            broadcastMessage(notice, clientSocket);
            break;
        }

        broadcastMessage(message, clientSocket);
    }

    close(clientSocket);

    // ✅ remove client properly
    lock_guard<mutex> lock(clientsMutex);
    clientSockets.erase(
        remove(clientSockets.begin(), clientSockets.end(), clientSocket),
        clientSockets.end());
}

int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
    {
        cerr << "Error creating socket.\n";
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        cerr << "Bind failed.\n";
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, MAX_CLIENTS) < 0)
    {
        cerr << "Listen failed.\n";
        close(serverSocket);
        return 1;
    }

    cout << ">> Server running on port " << PORT << "...\n";

    while (true)
    {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int newSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientLen);

        if (newSocket < 0)
        {
            cerr << "Error accepting connection.\n";
            continue;
        }

        {
            lock_guard<mutex> lock(clientsMutex);
            if ((int)clientSockets.size() >= MAX_CLIENTS)
            {
                string fullMsg = ">> Server full. Try again later.\n";
                send(newSocket, fullMsg.c_str(), fullMsg.size(), 0);
                close(newSocket);
                continue;
            }
            clientSockets.push_back(newSocket);
        }

        string notice = ">> A new user has joined the chat.\n";
        broadcastMessage(notice, newSocket);

        thread clientThread(handleClient, newSocket);
        clientThread.detach();
    }

    close(serverSocket);
    return 0;
}
