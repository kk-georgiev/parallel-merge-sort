#include <iostream>
#include <mutex>
#include <vector>
#include <thread>
#include <cstring>
#include <winsock2.h> 
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

// Mutex for synchronizing console access across multiple threads
std::mutex outputMutex;

// Function representing each client's task
void clientTask(int clientId, const std::vector<int>& data) {
   // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error: Winsock initialization failed.\n";
        return;
    }

    // Create TCP socket
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cerr << "Client " << clientId << " : Socket creation failed.\n";
        WSACleanup();
        return;
    }

    // Configure the server address
    // IPv4
    sockaddr_in serverAddr{};
   // Address family: AF_INET
    serverAddr.sin_family = AF_INET;
    // Port 8080
    serverAddr.sin_port = htons(8080);
    // Localhost
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); 

    // Attempt to connect to the server
    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cerr << "Client " << clientId << ": Connection failed.\n";
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    // Send the array size and the data itself to the server
    int size = data.size();
    
    send(clientSocket, (char*)&size, sizeof(size), 0);
    send(clientSocket, (char*)data.data(), size * sizeof(int), 0);

   // Receive the sorting times
    double singleThreadTime, multiThreadTime;
    
    recv(clientSocket, (char*)&singleThreadTime, sizeof(double), 0);
    recv(clientSocket, (char*)&multiThreadTime, sizeof(double), 0);

    // Receive the sorted array back from the server
    std::vector<int> sortedData(size);
    
    recv(clientSocket, (char*)sortedData.data(), size * sizeof(int), 0);

    // Print the results to the console
    std::lock_guard<std::mutex> lock(outputMutex);
    
    std::cout << "Client " << clientId << " : Sorted data: ";
    for (int num : sortedData){
        std::cout << num << " ";
    }
    std::cout << "\n";
    
    std::cout << "Client " << clientId << " : Single thread time :  " << singleThreadTime << " ms\n";
    std::cout << "Client " << clientId << " : Multi thread time :  " << multiThreadTime << " ms\n";
    std::cout << "\n";
    
    // Close the socket and clean up Winsock resources
    closesocket(clientSocket);
    WSACleanup();
}

int main() {
    // Sample arrays to be sorted by different clients
    std::vector<int> data1 = {};
    std::vector<int> data2 = {1};
    std::vector<int> data3 = {-21, 64, 9, -87, 38};
    std::vector<int> data4 = {-73, 45, 91, -6, 0, 28, -100, 67, -34, 12};
    std::vector<int> data5 = {-57, 22, 0, 89, -33, 14, 68, -91, 7, 45, -76, 3, 100, -8, 51, -64, 29, -2, 36, -49};

    // Create a thread for each client
    std::thread client1(clientTask, 1, data1);
    std::thread client2(clientTask, 2, data2);
    std::thread client3(clientTask, 3, data3);
    std::thread client4(clientTask, 4, data4);
    std::thread client5(clientTask, 5, data5);

    // Wait for all threads to finish
    client1.join();
    client2.join();
    client3.join();
    client4.join();
    client5.join();

    return 0;
}
