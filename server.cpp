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
#include <ctime>
#define MAX_CLIENTS 10
#define CHOOSE_SERVER 100;

// 30 - оптравляем номер комнаты
// 31 - создать новую комнату
//-10 - не смогли принять

struct sockaddr_in NewServAddr;

using namespace std;

// структура для сообщения
struct mes
{
   int command = 0;
   char name[256] = "Server";
   char message[256];
};

// структура для инфы сервера
struct serv_info
{
   char id[256];
   bool isOnline;
   char port[256];
   bool isProtected;
   char password[256];
};

// проверка существует ли файл
bool exists(string name)
{
   ifstream f(name.c_str());
   return f.good();
}

// настраиваем сервер
int CreateServer(int &Socket, struct sockaddr_in *servAddr)
{
   in_addr ip_to_num;
   int err = 0;
   // айпи даем
   if (inet_pton(AF_INET, "127.0.0.1", &ip_to_num) < 0)
   {
      cout << "Error inet_pton" << endl;
      return 1;
   }
   // получаем сокет
   Socket = socket(AF_INET, SOCK_STREAM, 0);
   if (Socket<0)
   {
      cout << "Error initialization socket" << endl;
      return 1;
   }
   else
      cout << "Server socket initialization is OK" << endl;

   // пихаем данные о сервере в структуру
   bzero(servAddr, sizeof(sockaddr_in));
   servAddr->sin_family = AF_INET;
   servAddr->sin_addr = ip_to_num;
   servAddr->sin_port = 0;

   // юиндим ее к сокету
   if (bind(Socket, (sockaddr *)servAddr, sizeof(sockaddr_in)))
   {
      cout << "Error bind socket" << endl;
      return 1;
   }
   else
      cout << "Bind os OK" << endl;

   // получаем порт что нам дался после бинда
   unsigned int length = sizeof(sockaddr_in);
   getsockname(Socket, (sockaddr *)servAddr, &length);

   // открываем сервер для прослушивания
   if (listen(Socket, 5) < 0)
   {
      cout << "Error listen" << endl;
      return 1;
   }
   else
   {
      cout << "Listen is OK" << endl;
   }

   return 0;
}

// отправит сообщение всем клиентам кроме одного и запишет сообщение в историю
int SendToAll(int *client_socket, mes *Message, int expect_socket, string id)
{
   //если комманда не равна 0, значит это служебное сообщение и сохранять не надо
   if (Message->command == 0)
   {
      
      ofstream file(id + string("_h.txt"), std::ios::app);
      if (file.is_open())
      {
         file << Message->name << endl;
         file << Message->message << endl;
         file.close();
      }
      else
      {
         return -1;
      }
   }
   cout << id << ": Send message to all" << endl;
   for (int i = 0; i < MAX_CLIENTS; i++)
   {
      if (client_socket[i] > 0 && client_socket[i] != expect_socket)
      {
         send(client_socket[i], Message, sizeof(mes), 0);
      }
   }
   return 0;
}

// отправляет всю историю клиенту
int SendHistory(int client_socket, string id)
{
   //проверяем существует ли файл
   if (exists(id + string("_h.txt")))
   {
      //если да, то открываем файл, проходим по всему файлу и отправляем сообщения клиенту
      ifstream file(id + string("_h.txt"));
      if (file.is_open())
      {
         string buf;
         const char *temp;
         mes *Message = new mes;
         while (getline(file, buf))
         {
            bzero(Message, sizeof(mes));
            strcpy(Message->name, buf.c_str());
            getline(file, buf);
            strcpy(Message->message, buf.c_str());
            send(client_socket, Message, sizeof(mes), 0);
         }
         //в конце отправляем сообщение что мы закончили отправлять
         Message->command = -10;
         strcpy(Message->message, "END");
         strcpy(Message->name, "Server");
         send(client_socket, Message, sizeof(mes), 0);
      }
      return -1;
   }
   else
   {
      //если файла с историей нет, то сразу отправляем сообщение что мы закончили отправлять
      mes *Message = new mes;
      Message->command = -10;
      strcpy(Message->message, "END");
      strcpy(Message->name, "Server");
      send(client_socket, Message, sizeof(mes), 0);
   }
   return 0;
}

//меняет данные о сервере в файле
int ChangeTXT(serv_info ServInfo)
{
   string id = ServInfo.id;
   ofstream file2;
   if (exists(id + string(".txt")))
   {
      file2.open(id + string(".txt"), std::ios::trunc);
   }
   else
   {
      file2.open(id + string(".txt"));
   }
   if (file2.is_open())
   {
      if (ServInfo.isOnline)
         file2 << "1" << endl;
      else
         file2 << "0" << endl;

      file2 << ServInfo.port << endl;

      if (ServInfo.isProtected)
         file2 << "1" << endl;
      else
         file2 << "0" << endl;

      file2 << ServInfo.password;

      file2.close();
   }
   else
   {
      return -1;
   }
   return 0;
}

//добавляем пользователя в файл
int AddUser(string id, string name)
{
   ofstream file(id + string("_u.txt"), std::ios::app);
   if (file.is_open())
   {
      file << name << endl;
      file.close();
      return 0;
   }
   return -1;
}

//рвоеряем есть ли пользователь в файле
int CheckUser(string id, string name)
{
   if (!exists(id + "_u.txt"))
      return 1;
   ifstream file(id + "_u.txt");
   if (file.is_open())
   {
      string buf;
      while (getline(file, buf))
      {
         if (name.compare(buf) == 0)
         {
            file.close();
            return 0;
         }
      }
      file.close();
      return 1;
   }
   else
   {
      return -1;
   }
}

//хэндлер для сервера
void DingDong(int MainSocket, serv_info ServInfo)
{
   cout << ServInfo.id << ": Start handler" << endl;
   fd_set readfds;
   struct timeval tv;
   struct sockaddr_in clientAddr;
   int count_clients = MAX_CLIENTS + 10;

   //массив для клиентов
   int *client_socket = new int[MAX_CLIENTS];
   for (int i = 0; i < MAX_CLIENTS; i++)
   {
      client_socket[i] = 0;
   }

   bool isPreEnd = false;

   // начинаем работу
   while (1)
   {
      // заполняем всех активных клиентов в набор дескрипторов и туда же сам сервер
      FD_ZERO(&readfds);
      FD_SET(MainSocket, &readfds);
      int max_sd = MainSocket;

      for (int i = 0; i < MAX_CLIENTS; i++)
      {
         int sd = client_socket[i];
         if (sd > 0)
            FD_SET(sd, &readfds);
         if (sd > max_sd)
            max_sd = sd;
      }
      // устанавливаем макс время ожидания и вызываем селект
      struct timeval tv;
      tv.tv_sec = 60;
      tv.tv_usec = 0;

      if (select(max_sd + 1, &readfds, NULL, NULL, &tv) == 0)
      {
         //если селект вернул 0, значит никто за время не отозвался, тогда мы

         //проверяем, если это 2 раза подряд то выходим
         if (isPreEnd)
         {
            ServInfo.isOnline = false;
            ChangeTXT(ServInfo);
            cout << ServInfo.id << ": ShoutDown" << endl;
            exit(0);
         }

         //пингуем всем клиентам
         isPreEnd = true;
         cout << ServInfo.id << ": Ping All" << endl;
         mes *Message = new mes;
         Message->command = 50;
         strcpy(Message->name, "Server");
         strcpy(Message->message, "PING");
         SendToAll(client_socket, Message, 0, ServInfo.id);
         continue;
      }

      isPreEnd = false;

      // если отозвался сам сервер значит  кто-то хочет подключиться
      if (FD_ISSET(MainSocket, &readfds))
      {
         // принимаем нового клиента
         int new_client;
         unsigned int length = sizeof(clientAddr);

         if ((new_client = accept(MainSocket, (struct sockaddr *)&clientAddr, (socklen_t *)&length)) < 0)
         {
            cout << ServInfo.id << ": Error accept" << endl;
         }

         //если клиенты уже заполнили массив
         if (count_clients >= MAX_CLIENTS && count_clients != MAX_CLIENTS + 10)
         {
            cout << ServInfo.id << ": To much clients" << endl;
            mes *Message = new mes;
            Message->command = -40;
            strcpy(Message->name, "Server");
            strcpy(Message->message, "Server Full");
            send(new_client, Message, sizeof(mes), 0);
            close(new_client);
         }
         else
         {
            //увеличиваем количество клиентов
            if (count_clients == MAX_CLIENTS + 10)
               count_clients = 1;
            else
               count_clients++;
            
            cout << ServInfo.id << ": New connection, socket fd is " << new_client << " ip is " << inet_ntoa(clientAddr.sin_addr) << " port is " << ntohs(clientAddr.sin_port) << endl;

            //отправляем клиенту историю
            SendHistory(new_client, ServInfo.id);

            // пишем в чат что участник вошел
            mes *Message = new mes;
            Message->command = 20;
            strcpy(Message->name, "Server");
            strcpy(Message->message, "Участник вошел");
            SendToAll(client_socket, Message, 0, ServInfo.id);
         }

         // добавляем нового участника в массив на пустое место
         for (int i = 0; i < MAX_CLIENTS; i++)
         {
            if (client_socket[i] == 0)
            {
               client_socket[i] = new_client;
               break;
            }
         }
      }

      // проходим по всем остальным дескрипторам
      for (int i = 0; i < MAX_CLIENTS; i++)
      {
         // если нашли клиента что отозвался, значит он что-то написал
         int sd = client_socket[i];
         if (FD_ISSET(sd, &readfds))
         {
            int readsize;
            void *buf;
            mes *Message = new mes;

            // читаем что он нам дал
            readsize = recv(sd, Message, sizeof(mes), 0);
            cout << ServInfo.id << ": Recive Message with n = " << readsize << endl;

            //если вернуло 0, значит клиент отключился
            if (readsize == 0)
            {
               count_clients--;
               if (count_clients <= 0)
               {
                  ServInfo.isOnline = false;
                  ChangeTXT(ServInfo);
                  cout << ServInfo.id << ": ShutDown" << endl;
                  exit(0);
               }
               cout << ServInfo.id << ": Client Disconnect" << endl;

               Message = new mes;
               Message->command = 20;
               strcpy(Message->name, "Server");
               strcpy(Message->message, "Участник вышел");
               close(sd);
               client_socket[i] = 0;

               SendToAll(client_socket, Message, 0, ServInfo.id);
            }
            else
            {
               if(Message->command==0) SendToAll(client_socket, Message, sd, ServInfo.id);
               // если клиент прислал сообщение то отправляем это сообщение всем кроме него
               
            }
         }
      }
   }
}

//меню что обрабатывает клиента что выбирает сервер
void Menu(int client_socket)
{
   cout << "Menu" << ": Start menu" << endl;

   mes *Message = new mes;
   serv_info ServInfo;
   int n = 1;
   while (1)
   {
      //если н = 0 то выходим
      if (n == 0){
         cout << "Menu" << ": ShutDown" << endl;
         exit(0);
      }
         
      n = recv(client_socket, Message, sizeof(mes), 0);


      if (n > 0)
      {
         if (Message->command == 30)
         {

            //принимаем айди команты, к которой хочет подключиться клиент
            cout << "Menu" << ": Recive ID from client" << endl;
            char *idc = Message->message;
            strcpy(ServInfo.id, idc);

            cout << "Menu" << ": Check " << ServInfo.id << " room" << endl;

            //проверяем, существует ли файл с инфой о комнате с таким айди
            if (exists(ServInfo.id + string(".txt")))
            {
               //если да
               cout << ": " << "Menu" << ServInfo.id + string(".txt") << " exist" << endl;

               mes *MessageS = new mes;
               strcpy(MessageS->message, "YEP");
               strcpy(MessageS->name, "Server");
               send(client_socket, MessageS, sizeof(mes), 0);

               // открываем файл с инфой и считываем все что в нем есть
               ifstream file(ServInfo.id + string(".txt"));
               if (file.is_open())
               {
                  cout << "Menu" << ": Start reading from file" << endl;
                  string buf;

                  getline(file, buf);
                  cout << "ONLINE: " << buf << endl;
                  if (buf.compare("0") == 0)
                     ServInfo.isOnline = false;
                  else
                     ServInfo.isOnline = true;

                  getline(file, buf);
                  strcpy(ServInfo.port, buf.c_str());
                  cout << "PORT: " << ServInfo.port << endl;

                  getline(file, buf);
                  cout << "PROTECTED: " << buf << endl;
                  if (buf.compare("0") == 0)
                     ServInfo.isProtected = false;
                  else
                     ServInfo.isProtected = true;

                  getline(file, buf);
                  strcpy(ServInfo.password, buf.c_str());
                  cout << "PASSWORD: " << ServInfo.password << endl;

                  // закрываем файл
                  file.close();
               }
               else{
                  cout << "Menu" << ": File not opened" << endl;
                  n=0;
                  continue;
               }

               //если сервер под паролем
               if (ServInfo.isProtected)
               {
                  cout << "Menu" << ": Server protected" << endl;
                  cout << "Menu" << ": Check user" << endl;
                  
                  //проверяем, есть ли пользователь в списке пользователей сервера
                  switch (CheckUser(ServInfo.id, Message->name))
                  {
                  case 1:
                  {
                     //если нет, то
                     cout << "Menu" << ": New User" << endl;
                     while (1)
                     {
                        //просим пароль
                        cout << "Menu" << ": Call password" << endl;
                        MessageS->command = 41;
                        strcpy(MessageS->message, "Введите пароль от сервера");
                        send(client_socket, MessageS, sizeof(mes), 0);

                        //принимаем пароль от клиента
                        if (recv(client_socket, Message, sizeof(mes), 0) > 0)
                        {
                           //если пароль совпал, то  добавляем пользователя в список
                           if (strcmp(ServInfo.password, Message->message) == 0)
                           {
                              AddUser(ServInfo.id, Message->name);
                              break;
                           }
                           else
                           {
                              //если пароль не совпал, то пишем что не совпал
                              MessageS->command = 42;
                              strcpy(MessageS->message, "Пароль неверный");
                              send(client_socket, MessageS, sizeof(mes), 0);
                           }
                        }
                        else{
                           cout << "Menu" << ": ShutDown" << endl;
                           exit(0);
                        }
                           
                     }

                     break;
                  }
                  case 0:
                  {
                     //если пользователь уже есть в списке
                     cout << "Menu" << ": Old User" << endl;
                     break;
                  }
                  case -1:
                  {
                     cout << "Menu" << ": Error in check user" << endl;
                     MessageS->command = -1;
                     strcpy(MessageS->message, "Повторите");
                     break;
                  }
                  default:
                     break;
                  }
               }

               //пишем что пользователь прошел проверку
               MessageS->command = 40;
               strcpy(MessageS->message, "PASS");
               send(client_socket, MessageS, sizeof(mes), 0);

               // если сервер открыт, то отправляем клиенту номер порта сервера, его мы берем из файла
               if (ServInfo.isOnline)
               {
                  cout << "Menu" << ": Server already Online" << endl;
                  in_port_t temp = htons(stoi(ServInfo.port));
                  send(client_socket, &temp, sizeof(in_port_t), 0);
                  cout << "Menu" << ": ShutDown" << endl;
                  exit(0);
               }
               else
               {
                  cout << "Menu" << ": Server is offline" << endl;
                  // если сервер не открыт, то мы должны его открыть
                  int NewServerSocket;
                  struct sockaddr_in NewServerAddr;
                  int pid;

                  // настраиваем сервер
                  cout << "Menu" << ": Launch server" << endl;
                  if (CreateServer(NewServerSocket, &NewServerAddr) == 0)
                  {

                     // выводим о нем информацию
                     cout << "ip - " << inet_ntoa(NewServerAddr.sin_addr) << endl;
                     cout << "port - " << ntohs(NewServerAddr.sin_port) << endl;

                     // меняем переменную что он онлайн и переписываем файл с новыми данными
                     ServInfo.isOnline = true;
                     strcpy(ServInfo.port, to_string(ntohs(NewServerAddr.sin_port)).c_str());
                     ChangeTXT(ServInfo);

                     // отправляем клиенту порт нового сервера для чата
                     cout << "Menu" << ": Send Port to client" << endl;
                     send(client_socket, &NewServerAddr.sin_port, sizeof(in_port_t), 0);

                     // ГДЕ ТО ТУТ ЕЩЕ И ВЫЗВАТЬ ФУНКЦИЮ ЧТО РАБОТАЕТ С СЕРВЕРОМ, НО ПОСМОТРЕТЬ, МОЖНО ЛИ СДЕЛАТЬ ЭТО В ЭТОМ ПРОЦЕССЕ,
                     // ЕСЛИ НЕТ ТО НЕ ЕБНЕТСЯ ЛИ ВСЕ КОГДА ЭТОТ ПРОЦЕСС ЗАКРОЕТСЯ, А ОБРАБОТЧИК СЕРВЕРА БУДЕТ В ДОЧЕРНЕМ ПРОЦЕССЕ
                     cout << "Menu" << ": Start handler" << endl;
                     close(client_socket);
                     DingDong(NewServerSocket, ServInfo);
                  }
               }
            }
            else
            {
               //если файла с инфой о сервере нет, значит он не существует
               cout << "Menu" <<": "<<ServInfo.id<<" not found" << endl;
               mes *Message = new mes;
               strcpy(Message->message, "NOPE");
               strcpy(Message->name, "Server");
               send(client_socket, Message, sizeof(mes), 0);
            }
         }
         //если пользователь хочет создать комнату
         else if (Message->command == 31)
         {
            cout << "Menu" << ": Create Room" << endl;

            serv_info *NewServ = new serv_info;

            //принимаем инфу о сервере что клиент хочет создать
            n = recv(client_socket, NewServ, sizeof(serv_info), 0);
            cout << "Menu" << ": Recive server info and n = " << n << endl;

            if (n > 0)
            {
               std::srand(std::time(nullptr));
               int random_number;
               string id_maybe;
               int count = 0;

               //создаем 4-х значный айди случайно, проверяя есть ли такой сервер или он свободен
               cout << "Menu" << ": Create ID" << endl;

               while (1)
               {
                  count++;
                  random_number = std::rand() % 9000 + 1000;
                  id_maybe = to_string(random_number);

                  if (!exists(id_maybe + string(".txt")))
                     break;

                  if (count >= 9999)
                     break;
               }
               if (count < 9999)
               {
                  cout << "Menu" << ": ID is" << id_maybe << endl;
                  strcpy(NewServ->id, id_maybe.c_str());
                  ChangeTXT(*NewServ);
                  
                  mes *MesPort = new mes;
                  MesPort->command = 31;
                  strcpy(MesPort->message, id_maybe.c_str());

                  cout << "Menu" << ": Send ID to client" << endl;
                  send(client_socket, MesPort, sizeof(mes), 0);
               }
               else
               {
                  cout << "Menu" << ": ID not create" << endl;
                  mes *MesPort = new mes;
                  MesPort->command = -31;
                  strcpy(MesPort->message, "NOPE");

                  send(client_socket, MesPort, sizeof(mes), 0);
               }
            }
            else{
               continue;
            }
         }
      }
   }
}


// мейн
int main()
{
 
   // все переменные и настраиваем сервер
   struct sockaddr_in servAddr, clientAddr;
   int ServSock = 0, NewServerSocket = 0;
   struct sockaddr_in NewServerAddr;

   cout<< "Main" <<": Start main"<<endl;

   if (CreateServer(ServSock, &servAddr) > 0)
   {
      return 1;
   }

   
   int pid = 0;
   int ClientSock = 0;

   // выводим о нем информацию
   unsigned int length = sizeof(servAddr);
   cout<< "Main" <<": Server"<<endl;
   cout << "ip - " << inet_ntoa(servAddr.sin_addr) << endl;
   cout << "port - " << ntohs(servAddr.sin_port) << endl;

   // начинаем принимать
   while (1)
   {
      length = sizeof(clientAddr);
      ClientSock = accept(ServSock, (sockaddr *)&clientAddr, &length);
      if (ClientSock < 0)
      {
         cout<< "Main" <<": Error accept"<<endl;
      }
      else
      {
         /// когда новый клиент
         // ПЕРЕРАБОАТЬ ВСЕ, КИДАТЬ ЕГО В МЕНЮ
         cout<< "Main" <<": New Client"<<endl;

         int pid;

         pid = fork();

         // в главном процессе остаемся и отправляем порт клиенту
         if (pid == 0)
         {
            break;
         }

         close(ClientSock);

         // send(ClientSock, &NewServerAddr.sin_port, sizeof(in_port_t), 0);
         // close(NewServerSocket);
      }
   }
   if (pid != 0)
      return 0;

   cout<< "Main" <<": Start menu"<<endl;
   close(ServSock);
   Menu(ClientSock);

   // новый процесс закрывает сокет сервера и начинает функцию работы с сервером.
}
