#include "utils.h"

#define NEED_NEW_NAME 1
#define DEFAULT_STATE 0

#define QUIT "quit"

void checkUsage(int argc, char **argv) {
	if (argc != 4) {
		printf("%s <server> <port> <username> \n", argv[0]);
		exit(EXIT_FAILURE);
	}
}

void setUsername(char **argv, char *username) {
	strcpy(username, argv[3]);
}

void setServerPort(char **argv, int* port) {
	*port = atoi(argv[2]);
}

void setServerAdressString(char **argv, char *serverAdressString) {
	strcpy(serverAdressString, argv[1]);
}

int initSocket(struct sockaddr_in ipAddress, int port, char* serverIP) {
	int idSocket;
	int connectReturn;

	// AF_INET -> use of IPv4
	ipAddress.sin_family = AF_INET;

	// Converts integer port into "real usable" port
	ipAddress.sin_port = htons(port);

	// Converts IPv4 number-dot to a "real usable" address
	ipAddress.sin_addr.s_addr = inet_addr(serverIP);

	// Create the socket : using IPv4, TCP (byte stream socket and 0)
	idSocket = socket(AF_INET, SOCK_STREAM, 0);

	// check the format of the given adress
	if (ipAddress.sin_addr.s_addr == -1) {
		printf("Socket error\n");
		exit(EXIT_FAILURE);
	} else {
		// Connection to the server
		// idSocket -> the id of the socket previously created
		// ipAddress -> pointer to an sockaddr struct which contains port and Id of the remote socket
		// sizeof(ipAddress) -> size of the struct
		struct sockaddr *ipAddressAsSockaddr = (struct sockaddr *) &ipAddress ;
		connectReturn = connect(idSocket, ipAddressAsSockaddr, sizeof(ipAddress) );
		printf("connectReturn = %i\n", connectReturn);
	}

	// Check if connection succeded
	if (connectReturn != 0) {
		printf("Remote service unreachable on server %s\n ", serverIP);
		exit(EXIT_FAILURE);
	}

	//return the Id of the socket created
	return idSocket;
}

void sendName(char* message, char* name, int idSocket) {

	// send NAME flag +requested name to server
	strcpy(message, NAME);
	strcat(message, name);
	write(idSocket, message, strlen(message) + 1);

	// Receive and print server's welcome message
	read(idSocket, message, strlen(WLCM_MESSAGE) + 1);
	printf("%s", message);
}



void doUserAction(char* message, char* buf, char* name, int idSocket, int *sendInProgress) {


	// If the user is sending a message
	if (*sendInProgress) {
		strcat(message, buf);
		write(idSocket, message, strlen(message) + 1);
		fflush(stdout);
		*sendInProgress = false;
	}
	// If the user asked from a disconnection
	else if (strncmp(buf, QUIT, strlen(QUIT)) == 0) {
		// Send info to server
		sprintf(buf, "Disconnection asked by %s\n", name);
		write(idSocket, buf, strlen(buf) + 1);
		// Close socket and end application
		close(idSocket);
		exit(EXIT_SUCCESS);
	}
	// If the user asked for the list of connected users
	else if (strncmp(buf, LIST, strlen(LIST)) == 0) {
		// Send this flag to server
		write(idSocket, buf, strlen(buf) + 1);
	}
	// If the user wants to send a message
	else if (strncmp(buf, SENDTO, strlen(SENDTO)) == 0) {
		char *recipientUsernameLength = (char *) malloc(sizeof(char)) ;
		char *recipientUsername = (char *) malloc(strlen(buf) - strlen(SENDTO));
		strncpy(recipientUsername, buf + strlen(SENDTO), strlen(buf) - strlen(SENDTO));
		strcpy(message, MSGTAG);
		sprintf(recipientUsernameLength, "%lu", strlen(recipientUsername) - 1);

		strcat(message, recipientUsernameLength); strcat(message, " ");
		strncat(message, recipientUsername, strlen(recipientUsername) - 1); strcat(message, " ");

		*sendInProgress = true;
		printf("Message : "); fflush(stdout);
	}
}

void askNewName(char* message, char* buf, int idSocket, int* nameState) {

	// set the end of string ('\0') to first '\n' encountered (to remove it -and anything below- from buf)
	char *p = strchr(buf, '\n');
	if (p != NULL) {
		*p = '\0';
	}

	// send NAME flag + new requested name to server
	strcpy(message, NAME);
	strcat(message, buf);
	write(idSocket, message, strlen(message) + 1);

	// set state to not new name requesting, server'll change that if needed
	*nameState = DEFAULT_STATE;
}


void readReceivedMessage(char *buf, int socket, int *nameState) {

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
			// If the server tells the user his username is already taken
			if (strncmp(buf, USERNAMETAKEN, strlen(USERNAMETAKEN)) == 0) {
				printf("Username already taken\n");
				printf("Please select another one :\n");
				// Change unsernameState to avoid the send of new message
				*nameState = NEED_NEW_NAME;
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

	char *username = (char *) malloc(BUFSIZE * sizeof(char)); // quite obvious
	char *serverAdressString = (char *) malloc(BUFSIZE * sizeof(char)); // server IP entered by user ("127.0.0.1" for example)
	int serverPort ;
	int idSocket ; // own socket number
	char * message = (char *) malloc (BUFSIZE * sizeof(char)); // buffer used to send messages to server
	int maxClientsNumber;
	fd_set socketList, currentWorkingList ; // socket lists (see further for explanation of need of 2 lists)
	int unsernameState = DEFAULT_STATE ;
	bool sendInProgress = false ; // whether the user is currently sending a message to server or not
	struct sockaddr_in ipAddress;


	char *buf = (char *)malloc(BUFSIZE * sizeof(char));


	printf("Launching application\n");
	printf("Setting up application\n");

	checkUsage(argc, argv);

	setUsername(argv, username);

	setServerAdressString(argv, serverAdressString);
	setServerPort(argv, &serverPort);

	printf("Application ready\n");
	printf("Connection to server on adress %s and port %d\n", serverAdressString, serverPort);

	// Setup socket's information (adress, destination port and socket type)
	idSocket = initSocket(ipAddress, serverPort, serverAdressString);

	// Send username and hence receive welcome message
	sendName(message, username, idSocket);

	// get descriptor table size aka max number of files open by the process => maximum number of clients
	maxClientsNumber = getdtablesize();

	// Empty socket list
	FD_ZERO(&socketList);

	// Add socket 0 -> stdin
	FD_SET(0, &socketList);

	// Add the socket with to the socket list
	FD_SET(idSocket, &socketList);

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
			if (unsernameState != NEED_NEW_NAME) {
				// parse user instruction to decide what to do and do it
				doUserAction(message, buf, username, idSocket, &sendInProgress);
			} else {
				// Ask server if "buf" is an available name
				askNewName(message, buf, idSocket, &unsernameState);
			}
		}
		// If something was received from server
		if (FD_ISSET(idSocket, &currentWorkingList)) {
			// parse received message and decide what to do
			// updates buf
			readReceivedMessage(buf, idSocket, &unsernameState);
		}
	}
	close(idSocket);
	return 0;
}


