#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/select.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <vector>

// Глобальная переменная для сигнала
volatile sig_atomic_t wasSigHup = 0;

// Обработчик сигнала
void sigHupHandler(int r) {
    wasSigHup = 1;
}

int main() {
    // Регистрация обработчика сигнала
    struct sigaction sa;
    sigaction(SIGHUP, NULL, &sa);
    sa.sa_handler = sigHupHandler;
    sa.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    // Блокировка сигнала
    sigset_t blockedMask, origMask;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    // Создаем серверный сокет
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 10) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port 8080...\n");
    std::vector<int> clients;
    // Основной цикл работы
    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(serverSocket, &fds);
        int maxFd = serverSocket;

        // Добавляем клиентские сокеты
        for (auto clientIt = clients.begin(); clientIt != clients.end(); clientIt++) {
            FD_SET(*clientIt, &fds);
            if (*clientIt > maxFd) {
                maxFd = *clientIt;
            }
        }

        // Ожидание событий с учетом сигналов
        if (pselect(maxFd + 1, &fds, NULL, NULL, NULL, &origMask) == -1) {
            if (errno == EINTR) {
                if (wasSigHup) {
                    printf("Received SIGHUP signal!\n");
                    wasSigHup = 0; // Сбрасываем флаг
                }
                continue; // Продолжаем цикл
            } else {
                perror("pselect");
                break;
            }
        }

        // Обработка новых соединений
        if (FD_ISSET(serverSocket, &fds)) {
            int clientSocket = accept(serverSocket, NULL, NULL);
            if (clientSocket == -1) {
                perror("accept");
            } else {
                printf("New connection accepted: %d\n", clientSocket);

                // Закрываем все соединения, кроме одного
                if (clients.empty()) {
                    clients.push_back(clientSocket);
                } else {
                    printf("Closing connection: %d\n", clientSocket);
                    close(clientSocket);
                }
            }
        }

        // Обработка данных от клиентов
        for (auto clientIt = clients.begin(); clientIt != clients.end();) {
            int fd = *clientIt;
            if (FD_ISSET(fd, &fds)) {
                char buffer[1024];
                ssize_t bytesRead = read(fd, buffer, sizeof(buffer));
                if (bytesRead <= 0) {
                    // Ошибка или клиент закрыл соединение
                    printf("Connection closed: %d\n", fd);
                    close(fd);
                    clientIt = clients.erase(clientIt);
                } else {
                    printf("Received %zd bytes from client %d\n", bytesRead, fd);
                    ++clientIt;
                }
            } else {
                ++clientIt;
            }
        }
    }

    // Закрытие серверного сокета
    close(serverSocket);
    return 0;
}
