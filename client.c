#include "utils.h"

#define NOUVEAU_PSEUDO 1
#define DEFAULT_STATE 0

#define QUIT "quit"

void checkUsage(int argc, char **argv) {
	if (argc != 4) {
		printf("%s <serveur> <port> <pseudo> \n", argv[0]);
		exit(EXIT_FAILURE);
	}
}

void setPseudo(char **argv, char *pseudo) {
	strcpy(pseudo, argv[3]);
}

void setPortServeur(char **argv, int* port) {
	*port = atoi(argv[2]);
}

void setServeurAdressString(char **argv, char *serverAdressString) {
	strcpy(serverAdressString, argv[1]);
}

int initSocket(struct sockaddr_in sin_serveur, int port, char* IPServeur) {
	int num_socket;
	int connectReturn;

	// AF_INET -> use of IPv4
	sin_serveur.sin_family = AF_INET;
	// Converts integer port into "real usable" port
	sin_serveur.sin_port = htons(port);
	// Converts IPv4 number-dot to a "real usable" address
	sin_serveur.sin_addr.s_addr = inet_addr(IPServeur);

	// Create the socket : using IPv4, TCP (byte stream socket and 0)
	num_socket = socket(AF_INET, SOCK_STREAM, 0);

	// check the format of the given adress
	if (sin_serveur.sin_addr.s_addr == -1) {
		printf("Erreur Socket\n");
		exit(EXIT_FAILURE);
	} else {
		// Connection to the server
		// num_socket -> the id of the socket previously created
		// sin_serveur -> pointer to an sockaddr struct which contains port and Id of the remote socket
		// sizeof(sin_serveur) -> size of the struct
		struct sockaddr *sin = (struct sockaddr *) &sin_serveur ;
		connectReturn = connect(num_socket, sin, sizeof(sin_serveur) );
		printf("connectReturn = %i\n", connectReturn);
	}
	// Check if connection succeded
	if (connectReturn != 0) {
		printf("Impossible d'acceder au serveur %s depuis le port %d\n ", IPServeur, port);
		exit(EXIT_FAILURE);
	}
	//return the Id of the socket created
	return num_socket;
}

void envoiPseudo(char* message, char* pseudo, int num_socket) {

	// send NAME flag +requested pseudo to server
	strcpy(message, NOM);
	strcat(message, pseudo);
	write(num_socket, message, strlen(message) + 1);

	// Receive and print server's welcome message
	read(num_socket, message, strlen(WLCM_MESSAGE) + 1);
	printf("%s", message);
}



void ActionUtilisateur(char* message, char* buf, char* pseudo, int num_socket, int *sendInProgress) {
	// If the user is sending a message
	if (*sendInProgress) {
		strcat(message, buf);
		write(num_socket, message, strlen(message) + 1);
		fflush(stdout);
		*sendInProgress = false;
	}
	// If the user asked from a disconnection
	else if (strncmp(buf, QUIT, strlen(QUIT)) == 0) {
		// Send info to server
		sprintf(buf, "Déconnexion de %s\n", pseudo);
		write(num_socket, buf, strlen(buf) + 1);
		// Close socket and end application
		close(num_socket);
		exit(EXIT_SUCCESS);
	}
	// If the user asked for the list of connected users
	else if (strncmp(buf, LIST, strlen(LIST)) == 0) {
		// Send this flag to server
		write(num_socket, buf, strlen(buf) + 1);
	}
	// If the user wants to send a message
	else if (strncmp(buf, SENDTO, strlen(SENDTO)) == 0) {
		char *recipientpseudoLength = (char *) malloc(sizeof(char)) ;
		char *recipientpseudo = (char *) malloc(strlen(buf) - strlen(SENDTO));
		strncpy(recipientpseudo, buf + strlen(SENDTO), strlen(buf) - strlen(SENDTO));
		strcpy(message, MSGTAG);
		sprintf(recipientpseudoLength, "%lu", strlen(recipientpseudo) - 1);

		strcat(message, recipientpseudoLength); strcat(message, " ");
		strncat(message, recipientpseudo, strlen(recipientpseudo) - 1); strcat(message, " ");

		*sendInProgress = true;
		printf("Message : "); fflush(stdout);
	}
}

void askNewpseudo(char* message, char* buf, int num_socket, int* pseudoState) {

	// set the end of string ('\0') to first '\n' encountered (to remove it -and anything below- from buf)
	char *p = strchr(buf, '\n');
	if (p != NULL) {
		*p = '\0';
	}

	// send NAME flag + new requested pseudo to server
	strcpy(message, NAME);
	strcat(message, buf);
	write(num_socket, message, strlen(message) + 1);

	// set state to not new pseudo requesting, server'll change that if needed
	*pseudoState = DEFAULT_STATE;
}


void readReceivedMessage(char *buf, int socket, int *pseudoState) {

	int readResult;
	readResult = read(socket, buf, BUFSIZE);

	// A read error has occured
	if (readResult == -1) {
		printf("Read error\n");
		printf("Disconnection from the remote server\n");
		close (socket);
		exit(EXIT_FAILURE);

	} else {
		if (readResult == 0) {
			printf("Server shutdown\n");
			printf("Disconnection from the remote server\n");
			close (socket);
			exit(EXIT_FAILURE);
		} else {
			// If the server tells the user his pseudo is already taken
			if (strncmp(buf, pseudoTAKEN, strlen(pseudoTAKEN)) == 0) {
				printf("pseudo already taken\n");
				printf("Please select another one :\n");
				// Change unserpseudoState to avoid the send of new message
				*pseudoState = NOUVEAU_PSEUDO;
				fflush(stdout);
			} else {
				// Display server message
				printf("%s", buf);
				fflush(stdout);
			}
		}
	}
}



int main( int argc, char**argv ) {

	char *pseudo = (char *) malloc(BUFSIZE * sizeof(char)); // quite obvious
	char *serverAdressString = (char *) malloc(BUFSIZE * sizeof(char)); // server IP entered by user ("127.0.0.1" for example)
	int serverPort ;
	int num_socket ; // own socket number
	char * message = (char *) malloc (BUFSIZE * sizeof(char)); // buffer used to send messages to server
	int maxClientsNumber;
	fd_set socketList, currentWorkingList ; // socket lists (see further for explanation of need of 2 lists)
	int unserpseudoState = DEFAULT_STATE ;
	bool sendInProgress = false ; // whether the user is currently sending a message to server or not
	struct sockaddr_in sin_serveur;


	char *buf = (char *)malloc(BUFSIZE * sizeof(char));


	printf("Launching application\n");
	printf("Setting up application\n");

	checkUsage(argc, argv);

	setPseudo(argv, pseudo);

	setServeurAdressString(argv, serverAdressString);
	setPortServeur(argv, &serverPort);

	printf("Application ready\n");
	printf("Connection to server on adress %s and port %d\n", serverAdressString, serverPort);

	// Setup socket's information (adress, destination port and socket type)
	num_socket = initSocket(sin_serveur, serverPort, serverAdressString);

	// Send pseudo and hence receive welcome message
	envoiPseudo(message, pseudo, num_socket);

	// get descriptor table size aka max number of files open by the process => maximum number of clients
	maxClientsNumber = getdtablesize();

	// Empty socket list
	FD_ZERO(&socketList);

	// Add socket 0 -> stdin
	FD_SET(0, &socketList);

	// Add the socket with to the socket list
	FD_SET(num_socket, &socketList);

	while (true) {

		// Reinit message buffer
		bzero(buf, BUFSIZE);

		// first list won't move during iteration, second is dynamic in iteration
		// it is copied here to ensure static list is up to date
		memcpy(&currentWorkingList, &socketList, sizeof(currentWorkingList));

		// socketListn list's sockets
		if (select(maxClientsNumber, &currentWorkingList, 0, 0, 0) == -1) {
			// EINTR just means "nothing received"
			if (errno == EINTR) {
				continue;
			}

			fprintf(stderr, "Select error: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		// If something was received from stdin
		if (FD_ISSET(0, &currentWorkingList)) {
			//Write client on stdout
			buf = fgets(buf, BUFSIZE, stdin);

			//If the user chose an available name
			if (unsernameState != NOUVEAU_PSEUDO) {
				// parse user instruction to decide what to do and do it
				ActionUtilisateur(message, buf, pseudo, num_socket, &sendInProgress);
			} else {
				// Ask server if "buf" is an available name
				askNewName(message, buf, num_socket, &unsernameState);
			}
		}
		// If something was received from server
		if (FD_ISSET(num_socket, &currentWorkingList)) {
			// parse received message and decide what to do
			// updates buf
			readReceivedMessage(buf, num_socket, &unsernameState);
		}
	}
	close(num_socket);
	return 0;
}
