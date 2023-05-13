#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <strings.h>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <pthread.h>

// ДОДЕЛАТЬ
// ОФОРМЛЕНИЕ ЕБАТЬ
// ВОЗМОЖНОСТЬ КОМАНД И В ТОМ ЧИСЛЕ ВЫХОДА В МЕНЮ
// САМО МЕНЮ СДЕЛАТЬ
// ДОАБВИТЬ ЧТО ЕСЛИ ВСЕ ЕБНУЛОСЬ ТО ПЕРЕПОДКЛЮЧЕНИЕ

using namespace std;

// структура для сообщения
char *name = "User";

struct mes
{
    int command = 0;
    char name[256];
    char message[256];
};

struct serv_info
{
    char id[256] = "0000";
    bool isOnline = false;
    char port[256] = "10000";
    bool isProtected;
    char password[256];
};

// переменные для сервера
int number = 3;
char *text = "3";
int port = 0;
char *ip_string = "127.0.0.1";
in_addr ip_to_num;
bool with_end = false;

// функция что берет параметры и записывает их в переменные
// УБРАТЬ ЛИШНЕЕ
void getParameters(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0)
        {
            i++;
            port = stoi(argv[i]);
            continue;
        }
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--name") == 0)
        {
            i++;
            name = argv[i];
            continue;
        }
        else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--ip") == 0)
        {
            i++;
            ip_string = argv[i];
            continue;
        }
    }
}

void *Catch(void *ClientSockVoid)
{
    int *ClientSock = (int *)ClientSockVoid;
    while (1)
    {
        mes *Message = new mes;
        int n = recv(*ClientSock, Message, sizeof(mes), 0);
        if (n == 0)
            cout << "СЕРВЕР ОТКЛЮЧИЛСЯ" << endl;
        else
        {
            if (Message->command != 50)
            {
                // и выводит его
                cout << Message->name << endl;
                cout << Message->message << endl
                     << endl
                     << endl;
            }
            else
            {
                send(*ClientSock, Message, sizeof(mes), 0);
            }
        }
    }
    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    

    // обновляем параметры
    getParameters(argc, argv);

    int ServSock = 0;
    int err = 0;

    // ставим нужный айпи
    err = inet_pton(AF_INET, ip_string, &ip_to_num);
    if (err < 0)
    {
        cout << "Error inet_pton" << endl;
        return 1;
    }

    // получаем сокет
    int ClientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (ClientSock < 0)
    {
        cout << "Error initialization socket" << endl;
        return 1;
    }
    else
        cout << "Client socket initialization is OK" << endl;

    // заполняем инфу о сервере
    struct sockaddr_in servAddr;
    bzero((char *)&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(port);
    servAddr.sin_addr = ip_to_num;

    // выводим ее
    cout << "number - " << number << endl;
    cout << "server ip - " << inet_ntoa(servAddr.sin_addr) << endl;
    cout << "server port - " << ntohs(servAddr.sin_port) << endl;

    // подключаемся к основному серверу
    unsigned int length = sizeof(servAddr);
    err = connect(ClientSock, (struct sockaddr *)&servAddr, length);
    if (err < 0)
    {
        cout << "Error connect" << endl;
        return 1;
    }
    else
    {
        cout << "Connect is OK" << endl;
    }

    // входим в меню
    while (1)
    {
        std::cout << "\033[2J\033[1;1H";
        cout << "1)Присоединиться к комнате" << endl;
        cout << "2)Создать новую команту" << endl;

        string number;
        getline(cin, number);

        if (number.compare("1") == 0 || number.compare("1)") == 0)
        {
            std::cout << "\033[2J\033[1;1H";

            cout << "Введите номер чата, к котору вы желаете присоединиться:" << endl;
            getline(cin, number);

            mes *Message = new mes;
            Message->command = 30;
            strcpy(Message->name, name);
            strcpy(Message->message, number.c_str());

            if (send(ClientSock, Message, sizeof(mes), 0) < 0)
                cout << "Pizdec" << endl;

            if (recv(ClientSock, Message, sizeof(mes), 0) <= 0)
            {
                cout << "ОШИБКА СЕРВЕРА" << endl;
                return 1;
            }

            if (strcmp(Message->message, "NOPE") == 0)
                continue;
            else
                break;
        }

        else if (number.compare("2") == 0 || number.compare("2)") == 0)
        {
            serv_info *ServInfo = new serv_info;
            string buf;

            std::cout << "\033[2J\033[1;1H";

            cout << "Нужен пароль? (y/n):" << endl;
            getline(cin, buf);

            if (buf.compare("y") == 0 || buf.compare("yes") == 0)
            {
                ServInfo->isProtected = true;
                cout << "Введите пароль для комнаты:" << endl;
                getline(cin, buf);
                strcpy(ServInfo->password, buf.c_str());
            }
            else
            {
                ServInfo->isProtected = false;
                strcpy(ServInfo->password, "NONE");
            }

            mes *Message = new mes;
            Message->command = 31;
            strcpy(Message->name, name);
            strcpy(Message->message, "ADD ROOM");

            if (send(ClientSock, Message, sizeof(mes), 0) < 0)
            {
                cout << "Не получилось отправить сообщение" << endl;
                return 1;
            }

            if (send(ClientSock, ServInfo, sizeof(serv_info), 0) < 0)
            {
                cout << "Не получилось отправить сообщение" << endl;
                return 1;
            }

            mes *newMes = new mes;
            if (recv(ClientSock, newMes, sizeof(mes), 0) <= 0)
            {
                cout << "ОШИБКА СЕРВЕРА" << endl;
                return 1;
            }

            if (newMes->command != -31)
            {
                cout << "Номер комнаты: " << newMes->message << endl;
            }
            char c;
            c = getchar();
        }
    }

    while (1)
    {
        mes *Message = new mes;
        if (recv(ClientSock, Message, sizeof(mes), 0) <= 0)
        {
            cout << "ОШИБКА СЕРВЕРА" << endl;
            return 1;
        }

        if (Message->command == 40)
            break;

        if (Message->command == 41)
        {
            cout << "Введите пароль от комнаты:" << endl;
            string buf;

            getline(cin, buf);

            mes *MessageU = new mes;
            strcpy(MessageU->name, name);
            strcpy(MessageU->message, buf.c_str());

            send(ClientSock, MessageU, sizeof(mes), 0);
        }

        else if (Message->command == 42)
        {
            cout << "Неверный пароль" << endl;
        }
    }

    // передаем этот айди серверу и ждем пока он отправит нам порт нового сервера
    in_port_t newport;
    void *buf;

    recv(ClientSock, &newport, sizeof(in_port_t), 0);

    // заполняем инфу о новом сервере
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = newport;
    servAddr.sin_addr = ip_to_num;

    // закрываем основной сервер
    close(ClientSock);

    // выводим инфу о новом серевере
    cout << "server ip - " << inet_ntoa(servAddr.sin_addr) << endl;
    cout << "server port - " << ntohs(servAddr.sin_port) << endl;

    // новый сокет
    ClientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (ClientSock < 0)
    {
        cout << "Ошибка в инициализировании сокета" << endl;
        return 1;
    }
    else
        cout << "Сокет инициализирвоан" << endl;

    // подключаем к новому серверу с чатом
    err = connect(ClientSock, (struct sockaddr *)&servAddr, length);
    if (err < 0)
    {
        cout << "Ошибка в подключениее к серверу" << endl;
        return 1;
    }
    else
    {
        cout << "Подклюичилсь к серверу" << endl;
    }

    // опять все очищаем и получаем всю историю от сервера и выводим ее
    std::cout << "\033[2J\033[1;1H";

    // получаем историю
    while (1)
    {
        mes *Message = new mes;

        if (recv(ClientSock, Message, sizeof(mes), 0) < 0)
        {
            cout << "ОШИБКА СЕРВЕРА" << endl;
            return 1;
        }

        else
        {
            if (Message->command == -10)
                break;

            cout << Message->name << endl;
            cout << Message->message << endl
                 << endl
                 << endl;
        }
    }
    // разделяем процессы чтобы один писал другой выводил
    pthread_t tid;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&tid, &attr, Catch, &ClientSock);

    // процесс куда мы пишем
    while (1)
    {

        // принимаем текст
        string textt;
        getline(cin, textt);

        if (textt.compare("/exit") == 0 || textt.compare("/e") == 0)
        {
            pthread_cancel(tid);
            close(ClientSock);
            return 0;
        }

        cout << endl
             << endl
             << endl;

        mes *Message = new mes;
        strcpy(Message->name, name);
        const char *temp = textt.c_str();
        strcpy(Message->message, temp);

        // отправляем
        if (send(ClientSock, Message, sizeof(mes), 0) < 0)
            cout << "Не получилсоь отправить" << endl;
    }

    // процесс который принимает сообщения
}
