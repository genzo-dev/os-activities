#include <iostream>
#include <thread>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

#define PORT 8080
#define BUFFER_SIZE 1024

void receiveMessages(int socketFd)
{
    char buffer[BUFFER_SIZE];
    while (true)
    {
        ssize_t bytesRead = recv(socketFd, buffer, BUFFER_SIZE - 1, 0);
        if (bytesRead <= 0)
        {
            cout << ">> Disconnected from server.\n";
            break;
        }
        buffer[bytesRead] = '\0';
        cout << buffer;
        cout.flush();
    }
}

int main()
{
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        cerr << "Error creating socket.\n";
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // localhost

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        cerr << "Connection to server failed.\n";
        return 1;
    }

    cout << ">> Connected to the chat.\n";

    thread recvThread(receiveMessages, clientSocket);
    recvThread.detach();

    string message;
    while (true)
    {
        getline(cin, message);
        message += "\n";
        send(clientSocket, message.c_str(), message.size(), 0);

        if (message == "exit\n")
        {
            cout << ">> Leaving chat...\n";
            break;
        }
    }

    close(clientSocket);
    return 0;
}
