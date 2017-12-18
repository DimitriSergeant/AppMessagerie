
/******************************************************************************/
/*							Application: Messagerie	multi-utilisateurs										*/
/******************************************************************************/
/*									      																										*/
/*			 									programme  SERVEUR			      											*/
/*									      																										*/
/******************************************************************************/
/*									      																										*/
/*		Auteurs : Dimitri SERGEANT , Léo VALETTE											 					*/
/*									      																										*/
/******************************************************************************/

#include "utils.h"

//Vérification de l'appel au programme
void checkUsage(int argc, char **argv){
	if (argc > 2){
		printf("Usage: %s <port>\n",argv[0]);
		exit(EXIT_FAILURE);
	}
}

// Défini le port
int setPort(int argc, char **argv){

	int result ;

	switch(argc) {
		case 2: 
		result = atoi(argv[1]); 
		printf("Port utilisé : %i\n", result);
		break;
		default: 
		result = DEFAULT_PORT ;
		printf("Port par défaut : %i\n", result);
		break;
	}
	return result ;

}

// Création et mise à jour des informations de la socket
void setupIP(struct sockaddr_in *address, int port){
	(*address).sin_family = AF_INET;
	(*address).sin_port = htons(port); 
	(*address).sin_addr.s_addr = INADDR_ANY; 


	void association(int serverSocketNumber, struct sockaddr_in serverAddress){
		struct sockaddr *serverAddressAsSockaddr = (struct sockaddr *) &serverAddress ;
		int size = sizeof(serverAddress) ;
		if(bind(serverSocketNumber, serverAddressAsSockaddr, size) == -1){
			printf("Erreur d'association: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}


	void ecoute(int serverSocketNumber, int nbMaxWaitingConnections){
		if(listen(serverSocketNumber, nbMaxWaitingConnections) == -1){
			printf("Erreur d'écoute, impossible d'écouter la socket %i\n", serverSocketNumber);
			exit(EXIT_FAILURE);
		}
	}


	void writeUsersList(int clientSock, char *(*clientNameList)[MAX_CLIENTS], int clientsConnectes){

	// Make sure "message" is long enough for every name to display
		char * message = (char *)malloc (BUFSIZE * sizeof(char));

		strcpy(message,"\nUtilisateurs connectés :\n");
		if (clientsConnectes == 0){
			strcat(message,"Personne d'autre n'est en ligne\n");
		}
		for (int i = 0 ; i < clientsConnectes; i++){
			strcat(message,(*clientNameList)[i]);
			strcat(message,"\n");
		}
		strcat(message, "\n");
		write(clientSock,message,strlen(message));
	}

	char *lecture(int sock){

		char *message = (char *) malloc(BUFSIZE * sizeof(char));

		int nbRead = read(sock, message, BUFSIZE);

		if(nbRead > 0){
			printf("Message reçu: %s", message);
			return message;
		}
		else{
			return NULL;
		}
	}

	int searchDisconnectingID(int listSize, int *list, int socketToFind){
		int result = 0;

		while(result < listSize && list[result] != socketToFind){
			result++;
		}

		return result ;
	}

	int getIntLength (int value){
		int l = 1;
		while(value > 9){
			l++;
			value /= 10;
		}
		return l;
	}

	int main(int argc, char **argv){


	int port ; //port d'écoute
	int serverSocketNumber ; // socket d'écoute du serveur

	struct sockaddr_in serverAddress ; // IP du serveur
	int nbMaxWaitingConnections = 10 ;
	int maxClientsNumber ;

	fd_set clientsList, currentWorkingList; 

	socklen_t addrSize ;
	struct sockaddr_in clientAddr ; // IP d'un client
	int clientSock; // socket du client

	int fd; // file descriptor (ie socket) to iterate

	char * clientNameList[MAX_CLIENTS]; // Noms des clients
	int clientSocketList[MAX_CLIENTS]; // et leur socket correspondantes

	int clientsConnectes = 0 ;

	char *message;

	int disconnectingID ;

	int doublon_pseudo = false;

	int recipientSocket ;
	int senderSocket ;

	checkUsage(argc, argv);

	printf("Création et initialisation du serveur\n");

	// Initialisation du serveur

	port = setPort(argc, argv);

	// Création de la socket du serveur
	serverSocketNumber = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Cette option permet de réutiliser l'adresse
	int option_value = true ;
	setsockopt(serverSocketNumber, SOL_SOCKET, SO_REUSEADDR,(char *) &option_value, sizeof(int));

	// Mise à jour des informations de la socket
	setupIP(&serverAddress, port);

	// Association de la socket au port
	association(serverSocketNumber, serverAddress);

	// Début d'écoute sur la socket du serveur
	ecoute(serverSocketNumber, nbMaxWaitingConnections);

	printf("Serveur prêt, écoute sur le port : %i\n", port);




	maxClientsNumber = getdtablesize();

	FD_ZERO(&currentWorkingList);
	FD_SET(serverSocketNumber, &currentWorkingList);


	//Gestion des clients
	while(true) {

		//On copie les listes de sockets
		memcpy(&clientsList, &currentWorkingList, sizeof(clientsList));

		//On lance la surveillance des descripteurs en lecture
		if(select(maxClientsNumber, &clientsList, 0, 0, 0) == -1){
			//On vérifie qu'on a bien reçu quelquechose
			if(errno == EINTR) {
				continue;
			}

			fprintf(stderr,"erreur: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		// Si la requête concernée est la socket de base, c'est qu'un client veut se connecter
		if(FD_ISSET(serverSocketNumber, &clientsList)){
			addrSize = sizeof clientAddr;
			if((clientSock = accept(serverSocketNumber, ((struct sockaddr *)&clientAddr), &addrSize)) == -1){
				fprintf(stderr, "Impossible d'établir la connection\n");
				exit(EXIT_FAILURE);
			}
			printf("Nouvelle connection depuis %s \n", inet_ntoa(clientAddr.sin_addr));

			//On ajoute le client dans les sockets sous surveillance
			FD_SET(clientSock, &currentWorkingList);

			//On confirme sa connection au client puis on lui donne la liste des autres connectés
			write(clientSock, BIENVENUE, strlen(BIENVENUE) + 1);
			writeUsersList(clientSock, &clientNameList, clientsConnectes);

		}

		// On vérifie si les sockets des clients ont bougé
		for(fd = 0; fd < maxClientsNumber ; fd++){
			if(fd != serverSocketNumber && FD_ISSET(fd, &clientsList)){

				message = lecture(fd) ;
				//Si un client déconnecte
				if(message == NULL){

					disconnectingID = searchDisconnectingID(clientsConnectes, clientSocketList, fd);
					if (disconnectingID < clientsConnectes){
						printf("utilisateur déconnecté : %s\n", clientNameList[disconnectingID]);
						while(disconnectingID < clientsConnectes - 1){
							clientSocketList[disconnectingID] = clientSocketList[disconnectingID + 1];
							clientNameList[disconnectingID] = clientNameList[disconnectingID + 1];
							disconnectingID++;
						}
						clientsConnectes--;
					}

					//On clôt la socket correspondante
					FD_CLR(fd, &currentWorkingList);
					close(fd);
				}
				else{

					// On ajouteun client
					if (strncmp(message, PSEUDO, strlen(PSEUDO)) == 0){
						char * wantedName = message + strlen(PSEUDO) ; // get wanted name by removing "[name]"
						printf("\nVérification de la disponibilité du pseudo %s ", wantedName);

						doublon_pseudo = false;
						//On vérifie que le pseudo n'est pas déjà utilisé
						for(int i = 0 ; i < clientsConnectes ; i++){
							if (strlen(wantedName) == strlen(clientNameList[i])){
								if(strncmp(wantedName, clientNameList[i], strlen(wantedName)) == 0){
									printf("✘\n");
									write(fd, PSEUDOPRIS, strlen(PSEUDOPRIS) + 1);
									doublon_pseudo = true;
									break;
								}
							}
						}
						printf("\n");

						//Si le pseudo est libre
						if(doublon_pseudo == false){
							clientNameList[clientsConnectes]= (char *) malloc(BUFSIZE * sizeof(char));
							strncat(clientNameList[clientsConnectes], wantedName,strlen(wantedName));
							clientSocketList[clientsConnectes] = (long int) malloc(sizeof(int)); // Why does it asks for a LONG int ?
							clientSocketList[clientsConnectes] = clientSock;
							clientsConnectes++;
						}
					}

					//Affichage de la liste des connectés
					else if (strncmp(message,LISTE,strlen(LISTE)) == 0){
						message = (char *)malloc (clientsConnectes * BUFSIZE * sizeof(char));
						strcpy(message,"\nUtilisateurs connectés :\n");
						if (clientsConnectes == 1) {
							strcat(message,"Personne d'autre n'est en ligne\n");
						} else {
							for (int i = 0 ; i < clientsConnectes ; i++){
								// Add every other user name to "message"
								if(clientSocketList[i]!=fd){
									strcat(message,clientNameList[i]);
									strcat(message,"\n");
								}
							}

						}
						strcat(message, "\n");
						write(fd,message,strlen(message));
					}

					//Envoie des messagees
					else if (strncmp(message, MSGTAG, strlen(MSGTAG)) == 0){

						char *lengthChar = (char *) malloc(BUFSIZE * sizeof(char));
						strncpy(lengthChar, message + strlen(MSGTAG), BUFSIZE);
						int recipientNameLength = atoi(lengthChar);
						int nbDigitsrecipientNameLength = getIntLength(recipientNameLength + 1);1`-th char of message and with a length of recipientNameLength
						char *recipientName = (char *) malloc(BUFSIZE * sizeof(char));
						strncpy(recipientName, message + strlen(MSGTAG) + nbDigitsrecipientNameLength + 1, recipientNameLength);

						char *messageBody= (char *) malloc(BUFSIZE * sizeof(char));
						int nbCharToIgnore = strlen(MSGTAG) + nbDigitsrecipientNameLength + 1 + recipientNameLength + 1;
						strncpy(messageBody, message + nbCharToIgnore, strlen(message) - nbCharToIgnore);

						//Recherche de la socket liée
						recipientSocket = 0;
						while(recipientSocket < clientsConnectes && strcmp(clientNameList[recipientSocket],recipientName) != 0){
							recipientSocket++;
						}

						if(recipientSocket < clientsConnectes){
							//Recherche de l'émetteur
							senderSocket = 0;
							while(senderSocket < clientsConnectes && clientSocketList[senderSocket] != fd){
								senderSocket++;
							}
							strcpy(message,clientNameList[senderSocket]);
							strcat(message, "> ");
							strcat(message, messageBody);
							write(clientSocketList[recipientSocket], message, strlen(message));
							printf("Message envoyé\n");
						}
						else{
							strcpy(message, recipientName);
							strcat(message, "N'existe pas sur le serveur\n");
							write(fd, message, strlen(message));
						}
					}
				}
			}
		}
	}
}
