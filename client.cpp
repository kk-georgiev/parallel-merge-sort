#include <iostream>
#include <mutex>
#include <vector>
#include <thread>
#include <cstring>
#include <winsock2.h> 
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// Mutex за синхронизация на достъпа до конзолата при многопоточност
std::mutex outputMutex;

// Функция, която представлява задачата на всеки клиент
void clientTask(int clientId, const std::vector<int>& data) {
    // Инициализация на Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Error: Winsock initialization failed.\n";
        return;
    }

    // Създаване на TCP сокет
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cerr << "Client " << clientId << " : Socket creation failed.\n";
        WSACleanup();
        return;
    }

    // Настройка на сървъра 
    // IPv4 
    sockaddr_in serverAddr{};
    // Семейство на адресите AF_INET
    serverAddr.sin_family = AF_INET;
    // Порт 8080
    serverAddr.sin_port = htons(8080);
    // Локален хост
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr); 

    // Опит за свързване със сървъра
    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::lock_guard<std::mutex> lock(outputMutex);
        std::cerr << "Client " << clientId << ": Connection failed.\n";
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    // Изпращане на размера на масива и самите данни към сървъра
    int size = data.size();
    
    send(clientSocket, (char*)&size, sizeof(size), 0);
    send(clientSocket, (char*)data.data(), size * sizeof(int), 0);

    // Получаване на времената за сортиране
    double singleThreadTime, multiThreadTime;
    
    recv(clientSocket, (char*)&singleThreadTime, sizeof(double), 0);
    recv(clientSocket, (char*)&multiThreadTime, sizeof(double), 0);

    // Получаване на сортирания масив обратно от сървъра
    std::vector<int> sortedData(size);
    
    recv(clientSocket, (char*)sortedData.data(), size * sizeof(int), 0);

    // Извеждане на резултатите на конзолата
    std::lock_guard<std::mutex> lock(outputMutex);
    
    std::cout << "Client " << clientId << " : Sorted data: ";
    for (int num : sortedData){
        std::cout << num << " ";
    }
    std::cout << "\n";
    
    std::cout << "Client " << clientId << " : Single thread time :  " << singleThreadTime << " ms\n";
    std::cout << "Client " << clientId << " : Multi thread time :  " << multiThreadTime << " ms\n";
    std::cout << "\n";
    
    // Затваряне на сокета и почистване на Winsock ресурсите
    closesocket(clientSocket);
    WSACleanup();
}

int main() {
    // Примерни масиви за сортиране от различни клиенти
    std::vector<int> data1 = {};
    std::vector<int> data2 = {1};
    std::vector<int> data3 = {-21, 64, 9, -87, 38};
    std::vector<int> data4 = {-73, 45, 91, -6, 0, 28, -100, 67, -34, 12};
    std::vector<int> data5 = {-57, 22, 0, 89, -33, 14, 68, -91, 7, 45, -76, 3, 100, -8, 51, -64, 29, -2, 36, -49};

    // Създаване на нишка за всеки клиент
    std::thread client1(clientTask, 1, data1);
    std::thread client2(clientTask, 2, data2);
    std::thread client3(clientTask, 3, data3);
    std::thread client4(clientTask, 4, data4);
    std::thread client5(clientTask, 5, data5);

    // Чакаме всички нишки да приключат
    client1.join();
    client2.join();
    client3.join();
    client4.join();
    client5.join();

    return 0;
}