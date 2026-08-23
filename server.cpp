#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cstring>
#include <algorithm>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

// Merge two already-sorted subarrays: start..mid and mid+1..end
void merge(std::vector<int>& arr, int start, int mid, int end){
   // Uses two temporary vectors, leftHalf and rightHalf
    std::vector<int> leftHalf(arr.begin() + start, arr.begin() + mid + 1);
    std::vector<int> rightHalf(arr.begin() + mid + 1, arr.begin() + end + 1);

    int i = 0, j = 0, k = start;

    // Merge the elements from the two subarrays
    while(i < leftHalf.size() && j < rightHalf.size()){
        arr[k++] = (leftHalf[i] <= rightHalf[j]) ? leftHalf[i++] : rightHalf[j++];
    }
    // Append the remaining elements
    while(i < leftHalf.size()){
        arr[k++] = leftHalf[i++];
    }
    while(j < rightHalf.size()){
        arr[k++] = rightHalf[j++];
    }
}

// Recursive single-threaded Merge Sort
void mergeSortSingleThread(std::vector<int>& arr, int start, int end){
    if(start >= end){
        return;
    }

    int mid = start + (end - start)/2;

    // Split the array into two halves, sort each half, and merge them
    mergeSortSingleThread(arr, start, mid);
    mergeSortSingleThread(arr, mid + 1, end);
    
    merge(arr, start, mid, end);
}

// Parallel execution of Merge Sort
void mergeSortMultiThread(std::vector<int>& arr, int start, int end, unsigned int threadCount = 0){
    if(start >= end){
        return;
    }

    int mid = start + (end - start)/2;

    // If we have available resources, spawn two new threads
    if(threadCount < std::thread::hardware_concurrency()){
        std::thread leftThread(mergeSortMultiThread, std::ref(arr), start, mid, threadCount + 1);
        std::thread rightThread(mergeSortMultiThread, std::ref(arr), mid + 1, end, threadCount + 1);
        
        leftThread.join();
        rightThread.join();
    }
    // Otherwise, fall back to the single-threaded version
    else{
        mergeSortSingleThread(arr, start, mid);
        mergeSortSingleThread(arr, mid + 1, end);
    }
    
    merge(arr, start, mid, end);
}

// Handle a client socket
void handleClient(SOCKET client){
    // Receive the array size
    int size;
    recv(client, (char*)&size, sizeof(size), 0);

    // Limit to guard against an excessively large array
    const int MAX_SIZE = 100000;
    if (size < 0 || size > MAX_SIZE) {
        std::cerr << "Error: Client requested too large array.\n";
        
        int error = -1;
        send(client, (char*)&error, sizeof(error), 0);
        
        closesocket(client);
        return;
    }

    // If the size is 0, return empty results
    if(size == 0){
        std::vector<int> emptyData;
        double emptyTime = 0.0;
        
        send(client, (char*)&emptyTime, sizeof(double), 0);
        send(client, (char*)emptyData.data(), 0, 0);
        
        closesocket(client);
        return;
    }
    
    // Receive the data
    std::vector<int> data(size);
    recv(client, (char*)data.data(), size *sizeof(int), 0);

    std::vector<int> dataSingleThread = data;

    // Single-threaded sort
    auto startSingleThread = std::chrono::high_resolution_clock::now();
    mergeSortSingleThread(dataSingleThread, 0, size - 1);
    auto endSingleThread = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> durationSingleThread = endSingleThread - startSingleThread;
    double singleThreadTime = durationSingleThread.count() * 1000;
    
    send(client, (char*)&singleThreadTime, sizeof(double), 0);

    // Multi-threaded sort
    auto startMultiThread = std::chrono::high_resolution_clock::now();
    mergeSortMultiThread(data, 0, size - 1);
    auto endMultiThread = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> durationMultiThread = endMultiThread - startMultiThread;
    double multiThreadTime = durationMultiThread.count() * 1000;
    
    send(client, (char*)&multiThreadTime, sizeof(double), 0);
   
    // Send the sorted data
    send(client, (char*)data.data(), size * sizeof(int), 0);

    closesocket(client);
}

int main() {
    // Initialize the Winsock library
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error: Winsock initialization failed.\n";
        return 1;
    }

    // Create TCP socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error: Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    // Configure the server
    // IPv4
    sockaddr_in serverAddr{};
    // Address family: AF_INET
    serverAddr.sin_family = AF_INET;
    // IP address (0.0.0.0)
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    // Port 8080
    serverAddr.sin_port = htons(8080);

    // Bind
    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error: Bind failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Listen
    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cerr << "Error: Listen failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is running on port 8080.\n";

    // Accept clients
    while (true) {
        SOCKET client = accept(serverSocket, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            std::cerr << "Error: Accept failed.\n";
            continue;
        }
        
        std::thread(handleClient, client).detach();
    }

    closesocket(serverSocket);
    WSACleanup();
 
    return 0;
}
