#include "server.h"

#define BUFFER_SIZE 1024
#define RESPONSE_LEN 9

int main(int argc, char *argv[]) {

	int ret, sd;

	struct msg clMsg;
	struct identification user;
	struct sockaddr_in srv_addr, cl_addr, cl_listen_addr;

	fd_set master, readfds;

	char buffer[BUFFER_SIZE];
	char Port[5];

	FD_ZERO(&master);
	FD_ZERO(&readfds);
	FD_SET(0, &master);

	/* Creazione socket */
	sd = socket(AF_INET, SOCK_STREAM, 0);

	/* Creazione indirizzo del server */
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(4242);
	inet_pton(AF_INET, "127.0.0.1", &srv_addr.sin_addr);

	if (argc != 2)
		{
				perror("Il numero di parametri che ho inserito all'avvio è sbagliato");
				exit(1);
		}
	strcpy(Port,argv[1]);
	printf("Port: %s\n",Port);
	/* connessione */
	ret = connect(sd, (struct sockaddr*) &srv_addr, sizeof(srv_addr));

	if (ret < 0) {
		perror("Errore in fase di connessione: \n");
		exit(1);
	}

	for (;;) {
		char cmdString[1024], headerBuffer[1024], sendbuffer[1024];
		char cmd[10],port[10],username[10],password[10];
		struct msg_header header;

		fgets(cmdString,1024-1,stdin);
		sscanf(cmdString, "%s %s %s %s", cmd, port, username, password);

		printf("Parametri:\nport:%s\nusername:%s\npwd:%s\n",port,username,password);

		if(strcmp(cmd,"register") == 0){
			sendHeader(sd,'R',"register",Port);
			sprintf(sendbuffer,"%s %s", port, username);
		}
		else if(strcmp(cmd,"login") == 0){
			sendHeader(sd,'L',"login",Port);
			strcpy(user.Username, username);
			strcpy(user.Password, password);
			sprintf(sendbuffer,"%s %s", username, password);
		}

		//send credentials to server
		sendMsg(sendbuffer,sd);

		//recieve response from server
		recieveHeader(sd,&header);

		if(header.RequestType == 1){
			if (strcmp(header.Options ,"register")==0)
			{
				printf("Client registered succesfully\n");
				break;
			}else{
				printf("Client already registered\n");
				break;
			}
			
		}else if(header.RequestType == 2){
			if(strcmp(header.Options,"login") == 0){
				printf("User logged in\n");
				break;
			}
		}
		printf("Terminado");

	} //chiudo il "for sempre" (linea 40)

}