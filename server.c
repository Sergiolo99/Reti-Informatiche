#include "server.h"

int main(int argc, char *argv[])
{

	struct sockaddr_in server_addr, client_addr;
	struct identification server, user;
	struct msg serverMsg;

	int ret, newfd, listener, addrlen, i, len, k, fdmax, port;

	fd_set master;
	fd_set read_fds;

	char buffer[BUF_LEN];
	char command[1024];

	FILE *UsersFile;

	if (argc != 2)
	{
		perror("Arguments don't match ");
		exit(1);
	}

	printf("------------------ Server online ------------------\n\n");
	printf("Commands: \n\n");
	printf("1 help\n2 list\n3 esc\n\n");

	memset(&user, 0, sizeof(user));
	memset(&server, 0, sizeof(server));

	// Reset FDs
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(0, &master);

	/* Creazione indirizzo di bind */
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(4242);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	/* Creazione socket */
	listener = socket(AF_INET, SOCK_STREAM, 0);
	ret = bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr));

	if (ret < 0)
	{
		perror("Bind non riuscita\n");
		exit(0);
	}

	listen(listener, 10);

	// Aggiungo il socket di ascolto (listener), creato dalla socket()
	// all'insieme dei descrittori da monitorare (master)
	FD_SET(listener, &master);

	// Aggiorno il massimo
	fdmax = listener;

	// main loop
	for (;;)
	{

		// Inizializzo il set read_fds, manipolato dalla select()
		read_fds = master;

		// Mi blocco in attesa di descrottori pronti in lettura
		// imposto il timeout a infinito
		// Quando select() si sblocca, in &read_fds ci sono solo
		// i descrittori pronti in lettura!
		ret = select(fdmax + 1, &read_fds, NULL, NULL, NULL);

		// Spazzolo i descrittori
		for (i = 0; i <= fdmax; i++)
		{

			// controllo se i  pronto
			if (FD_ISSET(i, &read_fds))
			{
				if (i == 0)
				{

					// Checking of command
					fscanf(stdin, "%s", command);
					if (strlen(command) > 1)
					{
						printf("Command doesn't exist\n");
						break;
					}
					if (command[0] < '1' || command[0] > '3')
					{
						printf("Command doesn't exist\n");
						break;
					}

					// ------------------ switching comandi server -------------------
					switch (command[0])
					{
					case '1':
						// show
						break;
					case '2':
						// list
						break;
					case '3':
						// esc
						printf("------------------ server closed ------------------\n");
						close(listener);
						exit(1);
						break;
					}
				}
				else
				{
					if (i == listener)
					{

						fflush(stdout);
						addrlen = sizeof(client_addr);
						// faccio accept() e creo il socket connesso 'newfd'
						newfd = accept(i, (struct sockaddr *)&client_addr,
									   (socklen_t *)&addrlen);

						// Aggiungo il descrittore al set dei socket monitorati
						FD_SET(newfd, &master);

						// Aggiorno l'ID del massimo descrittore
						if (newfd > fdmax)
						{
							fdmax = newfd;
						}

						printf("New client request accepted\n");
					}
					// se non  il listener, 'i''  un descrittore di socket
					// connesso che ha fatto la richiesta di orario, e va servito
					// **senza poi chiudere il socket** perch l'orario
					// potrebbe essere chiesto nuovamente al server
					else
					{
						struct header header_master;
						char portString[5];
						// Metto la richiesta nel buffer (pacchetto "REQ\0")
						ret = recieveHeader(i, &header_master);

						if (ret == 0)
						{
							printf("CHIUSURA client rilevata!\n");
							fflush(stdout);
							// il client ha chiuso il socket, quindi
							// chiudo il socket connesso sul server
							close(i);
							// rimuovo il descrittore newfd da quelli da monitorare
							FD_CLR(i, &master);
						}
						else if (ret < 0)
						{
							perror("ERRORE! \n");
							// si  verificato un errore
							close(i);
							// rimuovo il descrittore newfd da quelli da monitorare
							FD_CLR(i, &master);
						}
						else
						{
							switch (header_master.RequestType)
							{
							case 'R':
							{ //Registro
								struct Registry registro;

								recieveMsg(buffer, i);
								printf("Registrer started\n");
								sscanf(buffer, "%s %s", server.Username, server.Password);

								if (checkClient(server.Username) == 0)
								{
									sendHeader(i, 'R', "registered", "0000");
								}
								else
								{
									sendHeader(i, 1, "register", "0000");
									UsersFile = fopen("Users.txt", "r+");
									fwrite(&server, sizeof(server), 1, UsersFile);
									fclose(UsersFile);

									printf("Resgistro done\n");
									break;
								}
							}

							case 'L':
							{ //Login
								recieveMsg(buffer, i);
								sscanf(buffer, "%s %s", user.Username, user.Password);

								if (checkUser(UsersFile, &user, sizeof(user)) == 0)
								{
									printf("Login started\n");
									sendHeader(i, 2, "login", "8000");

									// add timestamp_in
									WriteLogin(user.Username, header_master.PortNumber);
									printf("Login done\n");
									break;
								}
							}
							case 'C':
							{ //Chat
								recieveMsg(buffer, i);
								printf("Chat di %s", buffer);

								port = checkOnline(buffer);
								sprintf(portString, "%i", port);

								if(port == 0){
									printf("Port 0");
								}

								// send if online or offline
								if(port == -1){
									sendHeader(i, 3, 'no', portString);
								}else{
									sendHeader(i, 3, 'yes', portString);
								}
								break;
							}

								if (ret < 0)
								{
									perror("Errore in fase di comunicazione \n");
								}
							}
						}
					}
				}
				// se i  il listener, ho ricevuto una richiesta di connessione
				// (un client ha invocato connect())

				// break;
			}
			// ci arrivo solo se monitoro stdin (descrittore 0)
			// -> rompo il while e passo a chiudere il listener
		}
	}
}