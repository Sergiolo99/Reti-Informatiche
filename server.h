#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>
//#include <arpa/inet.h>
//#include <unistd.h>
//#include <sys/socket.h>
//#include <sys/select.h>
//#include <netinet/in.h>
#include <ws2tcpip.h>
#include <winsock.h>

#define BUF_LEN 1024
#define REQUEST_LEN 4 // REQ\0

typedef struct identification
{
      char Username[50];
      char Password[50];
} identification;

typedef struct header
{
      char RequestType;
      char Options[10];
      char PortNumber[5];
} header;

typedef struct msg
{
      struct header Header;
      char Payload[1024 * 4];
} msg;

typedef struct Registry
{
      char Username[50];
      int Port;
      time_t timestamp_in;
      time_t timestamp_out;
} Registry;

typedef struct CommandCl
{
      char command[10];
      char argument[20];

}CommandCl;

int sendHeader(int receiver_socket, char req_type, char *options, char *port_number)
{
      char buf[1024];
      int msg_len;
      int ret;

      memset(buf, 0, sizeof(buf));
      sprintf(buf, "%c %s %s", req_type, options, port_number);

      // dimensione header
      msg_len = strlen(buf);
      ret = send(receiver_socket, (void *)&msg_len, sizeof(int), 0);

      // gestione disconnessione
      if (ret == 0)
            return ret;

      if (ret < 0)
      {
            perror("Non sono riuscito a mandare la lunghezza dell' header: ");
            return ret;
      }

      // invio header
      ret = send(receiver_socket, (void *)buf, msg_len, 0);
      if (ret < 0)
      {
            perror("Non sono riuscito a mandare il messaggio di header");
            return ret;
      }

      return ret;
}

int recieveHeader(int sender_socket, struct header *header)
{
      int msg_len;    // ci salvo la dimensione dell'header che sta arrivando
      char buf[1024]; // buffer su cui ricevo l'header
      int ret;

      memset(buf, 0, sizeof(buf));

      // dimensione
      ret = recv(sender_socket, (void *)&msg_len, sizeof(int), 0);

      // gestione disconnessione
      if (ret == 0)
            return ret;

      if (ret < 0)
      {
            perror("errore nella ricezione della dimensione dell'header");
            return ret;
      }

      ret = recv(sender_socket, (void *)buf, msg_len, 0);
      if (ret < 0)
      {
            perror("nono sono riuscito a ricevere l'header");
            return ret;
      }

      // faccio il parsing sulla struttura header
      sscanf(buf, "%c %s %s", &header->RequestType, header->Options, header->PortNumber);
      return ret;
}

int sendMsg(char *send_buffer, int receiver_socket)
{
      int msg_len = strlen(send_buffer);
      int ret;

      // dimensione
      ret = send(receiver_socket, (void *)&msg_len, sizeof(int), 0);

      // gestione disconnessione
      if (ret == 0)
            return ret;

      if (ret < 0)
      {
            perror("Non sono riuscito ad inviare la dimensione del messaggio");
            return ret;
      }

      // messaggio
      ret = send(receiver_socket, (void *)send_buffer, msg_len, 0);
      if (ret < 0)
      {
            perror("Non sono riuscito ad inviare il messaggio");
            return ret;
      }

      return ret;
}

int recieveMsg(char *recv_buffer, int sender_socket)
{
      int msg_len;
      int ret;

      // dimensione
      ret = recv(sender_socket, (void *)&msg_len, sizeof(int), 0);

      // gestione disconnessione
      if (ret == 0)
            return ret;

      if (ret < 0)
      {
            perror("Non sono riuscito a riceve la dimensione del messaggio");
            return ret;
      }

      // messaggio
      ret = recv(sender_socket, (void *)recv_buffer, msg_len, 0);
      if (ret < 0)
      {
            perror("Non sono riuscito a riceve il messaggio");
            return ret;
      }

      return ret;
}

int checkClient(char *username)
{
      FILE *fptr = fopen("Users.txt", "r");
      struct identification user; // usato per fare il parsing della struttura nel file

      while (fread(&user, sizeof(user), 1, fptr))
      {
            if (strcmp(username, user.Username) == 0)
                  return 0;
      }

      return -1;
}

int checkUser(FILE *fileptr, struct identification *userIdent, int size)
{
      struct identification user;
      fileptr = fopen("Users.txt", "r");
      while (fread(&user, size, 1, fileptr))
      {
            if (strcmp(user.Username, userIdent->Username) == 0 && strcmp(user.Password, userIdent->Password) == 0)
            {
                  fclose(fileptr);
                  return 0;
            }
      }
      fclose(fileptr);
      return -1;
}

int WriteLogin(char *username, char *port)
{
      struct Registry registry;
      time_t rawtime;
      FILE *fileptr = fopen("UsersHistory.txt", "r+");
      while (fread(&registry, sizeof(struct Registry), 1, fileptr))
      {     
            //searh of username
            if (strcmp(registry.Username, username) == 0)
            {
                  // add timestamp_in
                  registry.timestamp_in = time(&rawtime);
                  registry.timestamp_out = 0;
                  registry.Port = atoi(port);

                  // wite values
                  fseek(fileptr, -1 * sizeof(struct Registry), SEEK_CUR);
                  fwrite(&registry, sizeof(struct Registry), 1, fileptr);
                  fclose(fileptr);
                  return 0;
            }
      }

      fclose(fileptr);
      return -1;
}

int WriteLogout(char *username)
{
      // prende in ingresso user e cerca una corrispondenza nel file degli utenti online e ne cambia il ts
      FILE *fileptr = fopen("UsersHistory.txt", "r+");
      struct Registry registry;
      time_t rawtime;

      while (fread(&registry, sizeof(struct Registry), 1, fileptr))
      {
            if (strcmp(registry.Username, username) == 0)
            {
                  // ho trovato il record che cercavo e quindi ne aggiorno il campo timestamp_out
                  // modifico solo il timestamp
                  registry.timestamp_out = time(&rawtime);
                  // scrivo sul relativo record nel file
                  fseek(fileptr, -1 * sizeof(struct Registry), SEEK_CUR);
                  fwrite(&registry, sizeof(struct Registry), 1, fileptr);
                  fclose(fileptr);
            }
      }

      fclose(fileptr);
      return -1;
}

int checkOnline(char *Username)
{
      // apro il file di history
      FILE *fptr;
      struct Registry registro;
      fptr = fopen("UsersHistory.txt", "r");
      
      while (fread(&registro, sizeof(registro), 1, fptr))
      {
            if (strcmp(registro.Username, Username) == 0 && (registro.timestamp_out == 0) && (registro.Port != 0))
            {
                  fclose(fptr);
                  return registro.Port;
            }
      }
      fclose(fptr);
      return -1;
}