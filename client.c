#include "server.h"

#define BUFFER_SIZE 1024
#define RESPONSE_LEN 9

int main(int argc, char *argv[])
{

	int ret, sd, fdmax, listener, socketClient;

	struct msg clMsg;
	struct identification user;
	struct sockaddr_in srv_addr, cl_addr, cl_listen_addr;

	fd_set master, readfds;

	char buffer[BUFFER_SIZE];
	char Port[5];
	char dest[10];

	FD_ZERO(&master);
	FD_ZERO(&readfds);
	FD_SET(0, &master);

	/* Creazione socket server*/
	sd = socket(AF_INET, SOCK_STREAM, 0);

	/* Creazione indirizzo del server */
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(4242);
	inet_pton(AF_INET, "127.0.0.1", &srv_addr.sin_addr);

	/*Creazione socket client*/
	listener = socket(AF_INET, SOCK_STREAM, 0);
	ret = bind(listener, (struct sockaddr *)&cl_listen_addr, sizeof(cl_listen_addr));

	if (ret < 0)
	{
		perror("Bind non riuscita\n");
		exit(0);
	}

	FD_SET(listener, &master);
	fdmax = listener;

	/*Creazione indirizo device*/
	memset(&cl_addr, 0, sizeof(cl_addr));
	cl_listen_addr.sin_family = AF_INET;
	cl_listen_addr.sin_port = htons(atoi(Port));
	inet_pton(AF_INET, "127.0.0.1", &cl_listen_addr.sin_addr);

	if (argc != 2)
	{
		perror("Parameters don't match");
		exit(1);
	}
	strcpy(Port, argv[1]);

	/* connessione */
	ret = connect(sd, (struct sockaddr *)&srv_addr, sizeof(srv_addr));

	if (ret < 0)
	{
		perror("Error in conexion phase: \n");
		exit(1);
	}

	for (;;)
	{
		char cmdString[1024], headerBuffer[1024], sendbuffer[1024];
		char cmd[10], port[10], username[10], password[10];
		struct header header;

		fgets(cmdString, 1024 - 1, stdin);
		sscanf(cmdString, "%s %s %s %s", cmd, port, username, password);

		if (strcmp(cmd, "register") == 0)
		{
			sendHeader(sd, 'R', "register", Port);
			sprintf(sendbuffer, "%s %s", port, username);
		}
		else if (strcmp(cmd, "login") == 0)
		{
			sendHeader(sd, 'L', "login", Port);
			strcpy(user.Username, username);
			strcpy(user.Password, password);
			sprintf(sendbuffer, "%s %s", username, password);
		}

		//send credentials to server
		sendMsg(sendbuffer, sd);

		//recieve response from server
		recieveHeader(sd, &header);

		if (header.RequestType == 1)
		{
			if (strcmp(header.Options, "register") == 0)
			{
				printf("Client registered succesfully\n");
			}
			else
			{
				printf("Client already registered\n");
			}
		}
		else if (header.RequestType == 2)
		{
			if (strcmp(header.Options, "login") == 0)
			{
				printf("User logged in\n");
				break;
			}
		}
	}

	printf("---------------- Commands ----------------\n");
	printf("1 hanging:\n");
	printf("2 show <username>:\n");
	printf("3 chat <username>:\n");
	printf("4 share <file_name>: \n");
	printf("5 out:\n");

	fflush(stdin);

	for (;;)
	{
		readfds = master;
		printf("Select call\n");
		select(fdmax + 1, &readfds, NULL, NULL, NULL);

		for (int i = 0; i <= fdmax; i++)
		{
			// Devo ciclare fra i descrittori per servire quelli pronti
			if (FD_ISSET(i, &readfds))
			{
				// Ho trovato un descrittore pronto
				// Controllo il tipo del descrittore
				if (i == 0)
				{
					struct CommandCl cmd;
					char string[1024];

					memset(&cmd, 0, sizeof(cmd));

					if (fgets(string, 1024 - 1, stdin) == 0)
					{
						perror("Command not readed");
						fflush(stdin);
					}

					sscanf(string, "%s %s", cmd.command, cmd.argument);

					//check command
					if (strcmp(cmd.command, "hanging") != 0 || strcmp(cmd.command, "show") != 0 || strcmp(cmd.command, "chat") != 0 || strcmp(cmd.command, "share") != 0 || strcmp(cmd.command, "out") != 0)
					{
						printf("Comando non valido:\n");
						break;
					}

					switch (cmd.command[0])
					{
					case 'h':
					{
						break;
					}

					case 's':
					{
						break;
					}

					case 'c':
					{
						struct header header;
						char sendbuffer[1024];

						sendHeader(sd, 'C', "null", "0000");
						strcpy(dest, cmd.argument);
						sprintf(sendbuffer, "%s", dest);

						//send msg request to server
						sendMsg(sendbuffer, sd);
						//recieve response from server
						recieveHeader(sd, &header);

						//if online
						if (strcmp(header.Options, "yes") == 0)
						{
							//Creation of socket and adress
							memset(&cl_addr, 0, sizeof(cl_addr));
							cl_addr.sin_family = AF_INET;
							cl_addr.sin_port = htons(atoi(header.PortNumber));
							inet_pton(AF_INET, "127.0.0.1", &cl_listen_addr.sin_addr);

							socketClient = socket(AF_INET, SOCK_STREAM, 0);

							//Connection to dest
							ret = connect(socketClient, (struct sockaddr *)&cl_addr, sizeof(cl_addr));
							if (ret < 0)
							{
								printf("Connection with user failed");
							}

							sendMsg(user.Username, socketClient);

							FD_SET(socketClient, &master);
							fdmax = socketClient;
						}
						break;
					}
					}
				}
				else
				{
					if(i==listener){

						int addrlen = sizeof(cl_addr);

						int accpt = accept(i,(struct sockaddr *)&cl_addr, (socklen_t *)&addrlen);
						fdmax = accpt;
						FD_SET(accpt, &master);
						
						char new[20];
						memset(new,0,sizeof(new));
						recieveMsg(new,accpt);

						printf("New client connecting");
					}else{
						char buffMessage[1024];
						char clUsername[20];

						memset(clUsername,0,sizeof(clUsername));

						printf("Messagio: %s\n", buffMessage);
					}
				}
			}
		}
	}