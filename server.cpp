#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cstring>
#include <algorithm>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

// Сливане на два вече сортирани подмасива start..mid и mid+1..end
void merge(std::vector<int>& arr, int start, int mid, int end){
    // Използва два временни вектора leftHalf и rightHalf
    std::vector<int> leftHalf(arr.begin() + start, arr.begin() + mid + 1);
    std::vector<int> rightHalf(arr.begin() + mid + 1, arr.begin() + end + 1);

    int i = 0, j = 0, k = start;

    // Сливане на елементите от двата подмасива
    while(i < leftHalf.size() && j < rightHalf.size()){
        arr[k++] = (leftHalf[i] <= rightHalf[j]) ? leftHalf[i++] : rightHalf[j++];
    }
    // Добавяне на оставащите елементи
    while(i < leftHalf.size()){
        arr[k++] = leftHalf[i++];
    }
    while(j < rightHalf.size()){
        arr[k++] = rightHalf[j++];
    }
}

// Рекурсивен Merge Sort в един нишка
void mergeSortSingleThread(std::vector<int>& arr, int start, int end){
    if(start >= end){
        return;
    }

    int mid = start + (end - start)/2;

    // Разделяне на масива на две половини, сортиране на двете половини и сливане
    mergeSortSingleThread(arr, start, mid);
    mergeSortSingleThread(arr, mid + 1, end);
    
    merge(arr, start, mid, end);
}

// Паралелно изпълнение на Merge Sort
void mergeSortMultiThread(std::vector<int>& arr, int start, int end, unsigned int threadCount = 0){
    if(start >= end){
        return;
    }

    int mid = start + (end - start)/2;

    // Ако имаме ресурси създаваме две нови нишки
    if(threadCount < std::thread::hardware_concurrency()){
        std::thread leftThread(mergeSortMultiThread, std::ref(arr), start, mid, threadCount + 1);
        std::thread rightThread(mergeSortMultiThread, std::ref(arr), mid + 1, end, threadCount + 1);
        
        leftThread.join();
        rightThread.join();
    }
    // Ако не минаваме на еднонишкова версия
    else{
        mergeSortSingleThread(arr, start, mid);
        mergeSortSingleThread(arr, mid + 1, end);
    }
    
    merge(arr, start, mid, end);
}

// Обработване на клиентски сокет
void handleClient(SOCKET client){
    //Получаване на размер на масива
    int size;
    recv(client, (char*)&size, sizeof(size), 0);

    // Лимит за защита от прекалено голям масив
    const int MAX_SIZE = 100000;
    if (size < 0 || size > MAX_SIZE) {
        std::cerr << "Error: Client requested too large array.\n";
        
        int error = -1;
        send(client, (char*)&error, sizeof(error), 0);
        
        closesocket(client);
        return;
    }

    // Ако размерът е 0 връщаме празни резултати
    if(size == 0){
        std::vector<int> emptyData;
        double emptyTime = 0.0;
        
        send(client, (char*)&emptyTime, sizeof(double), 0);
        send(client, (char*)emptyData.data(), 0, 0);
        
        closesocket(client);
        return;
    }
    
    // Получаване на данните
    std::vector<int> data(size);
    recv(client, (char*)data.data(), size *sizeof(int), 0);

    std::vector<int> dataSingleThread = data;

    // Еднонишково сортиране
    auto startSingleThread = std::chrono::high_resolution_clock::now();
    mergeSortSingleThread(dataSingleThread, 0, size - 1);
    auto endSingleThread = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> durationSingleThread = endSingleThread - startSingleThread;
    double singleThreadTime = durationSingleThread.count() * 1000;
    
    send(client, (char*)&singleThreadTime, sizeof(double), 0);

    // Многнишково сортиране
    auto startMultiThread = std::chrono::high_resolution_clock::now();
    mergeSortMultiThread(data, 0, size - 1);
    auto endMultiThread = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<double> durationMultiThread = endMultiThread - startMultiThread;
    double multiThreadTime = durationMultiThread.count() * 1000;
    
    send(client, (char*)&multiThreadTime, sizeof(double), 0);
   
    // Изпращане на сортираните данни
    send(client, (char*)data.data(), size * sizeof(int), 0);

    closesocket(client);
}

int main() {
    // Инициализация на Winsock библиотеката
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error: Winsock initialization failed.\n";
        return 1;
    }

    // Създаване на TCP сокет
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Error: Socket creation failed.\n";
        WSACleanup();
        return 1;
    }

    // Настройка на сървъра 
    // IPv4 
    sockaddr_in serverAddr{};
    // Семейство на адресите AF_INET
    serverAddr.sin_family = AF_INET;
    // IP адрес (0.0.0.0)
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    // Порт 8080
    serverAddr.sin_port = htons(8080);

    // bind
    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error: Bind failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // listen
    if (listen(serverSocket, 5) == SOCKET_ERROR) {
        std::cerr << "Error: Listen failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server is running on port 8080.\n";

    // Приемане на клиенти
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