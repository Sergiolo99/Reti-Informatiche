#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define BUF_LEN 1024
#define REQUEST_LEN 4 // REQ\0

typedef struct identification
{
      char Username[50];
      char Password[50];
}identification;

typedef struct msg_header
{
      char RequestType;
      char Options[10];
      char PortNumber[5];
}msg_header;
typedef struct msg
{
      struct msg_header Header;
      char Payload[1024 * 4];
}msg;

typedef struct Registry
{
    char Username[50];
    int Port;
    time_t timestamp_in;
    time_t timestamp_out;
}Registry;


int sendHeader(int receiver_socket, char req_type, char* options, char * port_number)
{
      char buf[1024];
      int msg_len;
      int ret;

      memset(buf, 0 , sizeof(buf));
      sprintf(buf,"%c %s %s",req_type, options, port_number);

      // dimensione header
      msg_len = strlen(buf);
      ret = send (receiver_socket, (void*)&msg_len, sizeof(int), 0);

      // gestione disconnessione
      if ( ret == 0)
        return ret;

      if ( ret < 0)
      {
            perror("Non sono riuscito a mandare la lunghezza dell' header: ");
            return ret;
      }        

      // invio header
      ret = send(receiver_socket, (void*)buf, msg_len, 0);
      if ( ret < 0)
      {
            perror("Non sono riuscito a mandare il messaggio di header");
            return ret;
      }

      return ret;
}

int recieveHeader(int sender_socket, struct msg_header * header)
{     
      int msg_len; // ci salvo la dimensione dell'header che sta arrivando
      char buf[1024]; // buffer su cui ricevo l'header
      int ret;

      memset(buf,0,sizeof(buf));

      // dimensione
      ret = recv(sender_socket, (void*)&msg_len, sizeof(int), 0); 

      // gestione disconnessione
      if ( ret == 0)
        return ret;

      if (ret < 0)
      {
            perror("errore nella ricezione della dimensione dell'header");
            return ret;
      }
      
      ret = recv(sender_socket,(void*)buf, msg_len, 0);
      if ( ret < 0 )
      {
            perror("nono sono riuscito a ricevere l'header");
            return ret;
      }

      // faccio il parsing sulla struttura header
      sscanf(buf,"%c %s %s",&header->RequestType,header->Options,header->PortNumber);
      return ret;
}


int sendMsg(char *send_buffer, int receiver_socket)
{
      int msg_len = strlen(send_buffer);
      int ret; 

      // dimensione
      ret = send(receiver_socket,(void*)&msg_len, sizeof(int), 0);

      // gestione disconnessione
      if ( ret == 0)
          return ret;

      if ( ret < 0 )
      {
            perror("Non sono riuscito ad inviare la dimensione del messaggio");
            return ret;
      }

      // messaggio
      ret = send(receiver_socket, (void*)send_buffer, msg_len, 0);
      if(ret < 0 )
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
      ret = recv(sender_socket, (void*)&msg_len, sizeof(int), 0);

      // gestione disconnessione
      if ( ret == 0)
        return ret;

      if (ret < 0)
            {
                  perror("Non sono riuscito a riceve la dimensione del messaggio");
                  return ret;
            }
      
      // messaggio
      ret = recv(sender_socket, (void*)recv_buffer, msg_len, 0);
      if (ret < 0)
            {
                  perror("Non sono riuscito a riceve il messaggio");
                  return ret;
            }

      return ret;
}

int checkClient(char *username)
{
      FILE *fptr = fopen("Users.txt", "rb");
      struct identification user; // usato per fare il parsing della struttura nel file

      while (fread(&user, sizeof(user), 1, fptr))
      {
            if (strcmp(username, user.Username) == 0)
                  return 0;
      }

      return -1;
}