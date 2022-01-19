#include "server.h"

int main(int argc, char *argv[])
{

	struct sockaddr_in server_addr, client_addr;
	struct identification server, user;
	struct msg sv_msg;

	int ret, newfd, listener, addrlen, i, len, k;

	fd_set master;
	fd_set read_fds;
	int fdmax;

	char buffer[BUF_LEN];
	char ServerPort[5] = "4242";
	char Command[1024];

	FILE *UsersFile, *ChatBuffer, *RegistryFile;

	if (argc != 2)
	{
		perror("Arguments don't match ");
		exit(1);
	}

	strcpy(ServerPort, argv[1]);
	printf("La porta selezionata è %s \n", ServerPort);

	printf("------------------ server online ------------------\n\n");
	printf("Comandi disponibili\n\n");
	printf("1 <help>\n2 <list>\n3 <esc>\n\n");

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

					// Check della correttezza del comando sullo stdin
					fscanf(stdin, "%s", Command);
					if (strlen(Command) > 1)
					{
						printf("Command doesn't exist\n");
						break;
					}
					if (Command[0] < '1' || Command[0] > '3')
					{
						printf("Command doesn't exist\n");
						break;
					}

					// ------------------ switching comandi server -------------------
					switch (Command[0])
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
						struct msg_header header_master;
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
								printf("Registro di utente nuovo\n");
								sscanf(buffer, "%s %s", server.Username, server.Password);

								if (checkClient(server.Username) == 0)
								{
									sendHeader(i, 'R', "registered", "0000");
								}
								else
								{
									sendHeader(i, 1, "register", "0000");
									UsersFile = fopen("Users.txt", "ab");
									fwrite(&server, sizeof(server),1,UsersFile);
									fclose(UsersFile);

									strcpy(registro.Username, server.Username);
									registro.Port = 0;
									registro.timestamp_in = 0;
									registro.timestamp_out = 0;

									RegistryFile = fopen("UsersHistory.txt","ab");
									fwrite(&registro, sizeof(struct Registry),1,RegistryFile);
									
									printf("Resgistro fatto\n");
									break;
								}
							}

							case 'L':
							{ //Login
								recieveMsg(buffer, i);
								sscanf(buffer, "%s %s", user.Username, user.Password);
								printf("Login utente\n");
								sendHeader(i, 2, "login", "8000");
								printf("Login fatto\n");
								break;
							}
							case 'C':
							{ //Chat
								recieveMsg(buffer, i);
								printf("Chat di %s", buffer);
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