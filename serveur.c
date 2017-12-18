
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


void checkUsage(int argc, char **argv){
	if (argc > 2){
		printf("Usage: %s <port>\n",argv[0]);
		exit(EXIT_FAILURE);
	}
}

// CheccurrentlyActiveClientsright usage of command and set port as parameter if defined, as DEFAULT_PORT otherwise
int setPort(int argc, char **argv){

	int result ;

	switch(argc) {
		case 2: // port provided
		    result = atoi(argv[1]); // atoi : string to int
		    printf("Used port : %i\n", result);
		    break;
		default: // port not provided
		result = DEFAULT_PORT ;
		printf("Unspecified port. Default port : %i\n", result);
		break;
	}
	return result ;

}

// sets up the IP connection with address the server sockaddr_in and port the server connection port
// Used http://man7.org/linux/man-pages/man7/ip.7.html
void setupIP(struct sockaddr_in *address, int port){
	(*address).sin_family = AF_INET;
	(*address).sin_port = htons(port); // from host to network
	(*address).sin_addr.s_addr = INADDR_ANY; // bind to all local interfaces
}


void bindWrapper(int serverSocketNumber, struct sockaddr_in serverAddress){
	struct sockaddr *serverAddressAsSockaddr = (struct sockaddr *) &serverAddress ;
	int size = sizeof(serverAddress) ;
	if(bind(serverSocketNumber, serverAddressAsSockaddr, size) == -1){
		printf("Bind error: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
}


void listenWrapper(int serverSocketNumber, int nbMaxWaitingConnections){
	if(listen(serverSocketNumber, nbMaxWaitingConnections) == -1){
		printf("Listen error: could not listen on socket %i\n", serverSocketNumber);
		exit(EXIT_FAILURE);
	}
}


void writeUsersList(int clientSock, char *(*clientNameList)[MAX_CLIENTS], int currentlyActiveClients){

	// Make sure "message" is long enough for every name to display
	char * message = (char *)malloc (BUFSIZE * sizeof(char));

	strcpy(message,"*****\nConnected users :\n");
	if (currentlyActiveClients == 0){
		strcat(message,"Nobody else connected yet\n");
	}
	for (int i = 0 ; i < currentlyActiveClients; i++){
		strcat(message,(*clientNameList)[i]);
		strcat(message,"\n");
	}
	strcat(message, "*****\n");
	write(clientSock,message,strlen(message));
}

char *readWrapper(int sock){

	char *message = (char *) malloc(BUFSIZE * sizeof(char));

	int nbRead = read(sock, message, BUFSIZE);

	if(nbRead > 0){
		printf("Received message: %s", message);
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


	int port ; // Listening port
	int serverSocketNumber ; // server listening socket

	struct sockaddr_in serverAddress ; // Server IP address

	int nbMaxWaitingConnections = 10 ; // Max number of waiting connection on a listen
	int maxClientsNumber ; // Max number of client running at the same time, depends on process file descriptor table size

	fd_set clientsList, currentWorkingList; // socket lists

	socklen_t addrSize ;
	struct sockaddr_in clientAddr ; // an IP for a connecting user
	int clientSock;

	int fd; // file descriptor (ie socket) to iterate

	char * clientNameList[MAX_CLIENTS]; // name of connected user's names
	int clientSocketList[MAX_CLIENTS]; // their corresponding socket number

	int currentlyActiveClients = 0 ; // Quite obvious

	char *message; // raw received message

	int disconnectingID ; // Used to identify which socket just quit app (to mention it to others and close it)

	int nameExists = false;

	int recipientSocket ;
	int senderSocket ;

	checkUsage(argc, argv);

	printf("Booting server\n");
	printf("Setting up server\n");

	/* ##### SERVER SETUP ##### */

	port = setPort(argc, argv);

	// create server socket, chosing an IPv4 domain and a byte stream socket => TCP
	serverSocketNumber = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Enable local address reuse
	int option_value = true ;
	setsockopt(serverSocketNumber, SOL_SOCKET, SO_REUSEADDR,(char *) &option_value, sizeof(int));

	// Setup server IP
	setupIP(&serverAddress, port);

	// Bind socket and server address
	bindWrapper(serverSocketNumber, serverAddress);

	// Start listening on server socket
	listenWrapper(serverSocketNumber, nbMaxWaitingConnections);

	printf("Server ready, listening on port %i\n", port);

	/* ##### END SERVER SETUP ##### */



	// get descriptor table size aka max number of files open by the process => maximum number of clients
	maxClientsNumber = getdtablesize();

	// Empty socket list
	FD_ZERO(&currentWorkingList);
	// Add serverSocket to socket list
	FD_SET(serverSocketNumber, &currentWorkingList);

	while(true) {

		// first list won't move during iteration, second is dynamic in iteration
		// it is copied here to ensure static list is up to date
		memcpy(&clientsList, &currentWorkingList, sizeof(clientsList));

		// listen list's sockets
		if(select(maxClientsNumber, &clientsList, 0, 0, 0) == -1){
			// EINTR just means "nothing received"
			if(errno == EINTR) {
				continue;
			}

			fprintf(stderr,"Select error: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		// A message on server's socket means that there is a new user requesting connection
		if(FD_ISSET(serverSocketNumber, &clientsList)){

			// The creation of the connection is made via accept procedure
			addrSize = sizeof clientAddr;
			if((clientSock = accept(serverSocketNumber, ((struct sockaddr *)&clientAddr), &addrSize)) == -1){
				fprintf(stderr, "Accept error: couldn't establish connection\n");
				exit(EXIT_FAILURE);
			}
			// print new client IP address
			printf("New connection from %s \n", inet_ntoa(clientAddr.sin_addr));

			// Add new user's socket to socket list
			FD_SET(clientSock, &currentWorkingList);

			// Send a welcoming message followed by users list
			write(clientSock, WLCM_MESSAGE, strlen(WLCM_MESSAGE) + 1);
			writeUsersList(clientSock, &clientNameList, currentlyActiveClients);

		}

		// Every socket is inspected to get its commands ("busy waiting")
		for(fd = 0; fd < maxClientsNumber ; fd++){
			if(fd != serverSocketNumber && FD_ISSET(fd, &clientsList)){

				message = readWrapper(fd) ;
				// From that point, 4 possibilities :
				// 1) the user deconnects itself
				// 2) the user asks for a name (durin its connection)
				// 3) the user asks for the list of connected users
				// 4) the user asks to send a message to someone
				// There is no other case to deal with as only correct command are transmitted to the server

				// Case 1)
				if(message == NULL){

					disconnectingID = searchDisconnectingID(currentlyActiveClients, clientSocketList, fd);

					// If disconnectingID is active (ie it has a name) must remove said name
					if (disconnectingID < currentlyActiveClients){
						printf("User disconnected : %s\n", clientNameList[disconnectingID]);
						// Removing from clientSocketList and clientNameList (by left shifting rest of arrays)
						while(disconnectingID < currentlyActiveClients - 1){
							clientSocketList[disconnectingID] = clientSocketList[disconnectingID + 1];
							clientNameList[disconnectingID] = clientNameList[disconnectingID + 1];
							disconnectingID++;
						}
						currentlyActiveClients--;
					}

					// Remove current socket from list and close it
					FD_CLR(fd, &currentWorkingList);
					close(fd);
				}
				else{

					// Case 2)
					if (strncmp(message, NAME, strlen(NAME)) == 0){
						char * wantedName = message + strlen(NAME) ; // get wanted name by removing "[name]"
						printf("\nChecking name availability of %s ", wantedName);

						nameExists = false;
						for(int i = 0 ; i < currentlyActiveClients ; i++){
							if (strlen(wantedName) == strlen(clientNameList[i])){
								if(strncmp(wantedName, clientNameList[i], strlen(wantedName)) == 0){
									printf("✘\n");
									write(fd, USERNAMETAKEN, strlen(USERNAMETAKEN) + 1);
									nameExists = true;
									break;
								}
							}
						}
						printf("\n");

						// Add user's name to user's names list and its socket to sockets list
						if(nameExists == false){
							clientNameList[currentlyActiveClients]= (char *) malloc(BUFSIZE * sizeof(char));
							strncat(clientNameList[currentlyActiveClients], wantedName,strlen(wantedName));
							clientSocketList[currentlyActiveClients] = (long int) malloc(sizeof(int)); // Why does it asks for a LONG int ?
							clientSocketList[currentlyActiveClients] = clientSock;
							currentlyActiveClients++;
						}
					}

					// Case 3)
					else if (strncmp(message,LIST,strlen(LIST)) == 0){

						// Segmentation fault if use of writeUsersList here. Why ? (valgrind makes me think wrong parameters passing)
						// same with futur code needing to pass clientNameList or clientSocketList as parameters


						message = (char *)malloc (currentlyActiveClients * BUFSIZE * sizeof(char));

						strcpy(message,"*****\nConnected users :\n");
						if (currentlyActiveClients == 1) {
							strcat(message,"Nobody else connected yet\n");
						} else {
							for (int i = 0 ; i < currentlyActiveClients ; i++){
								// Add every other user name to "message"
								if(clientSocketList[i]!=fd){
									strcat(message,clientNameList[i]);
									strcat(message,"\n");
								}
							}

						}
						strcat(message, "*****\n");
						// Send "message" (usernames list) to requesting user
						write(fd,message,strlen(message));
					}

					// Case 4)
					else if (strncmp(message, MSGTAG, strlen(MSGTAG)) == 0){

						// message = MSGTAG<username length + 1>␣<username>␣<message>

						// Get name length
						char *lengthChar = (char *) malloc(BUFSIZE * sizeof(char));
						strncpy(lengthChar, message + strlen(MSGTAG), BUFSIZE);
						int recipientNameLength = atoi(lengthChar);
						int nbDigitsrecipientNameLength = getIntLength(recipientNameLength + 1);// + 1 because client sends username length + 1

						// Get recipient name
						// put it in recipientName, from message, starting at `strlen(MSGTAG)+nbDigitsrecipientNameLength+1`-th char of message and with a length of recipientNameLength
						char *recipientName = (char *) malloc(BUFSIZE * sizeof(char));
						strncpy(recipientName, message + strlen(MSGTAG) + nbDigitsrecipientNameLength + 1, recipientNameLength);

						// Get message
						char *messageBody= (char *) malloc(BUFSIZE * sizeof(char));
						// message starts at `strlen(MSGTAG) + nbDigitsrecipientNameLength + 1 + recipientNameLength + 1`-th char
						int nbCharToIgnore = strlen(MSGTAG) + nbDigitsrecipientNameLength + 1 + recipientNameLength + 1;
						strncpy(messageBody, message + nbCharToIgnore, strlen(message) - nbCharToIgnore);


						// get recipient socket number
						recipientSocket = 0;
						while(recipientSocket < currentlyActiveClients && strcmp(clientNameList[recipientSocket],recipientName) != 0){
							recipientSocket++;
						}

						if(recipientSocket < currentlyActiveClients){
							//get sender socket number
							senderSocket = 0;
							while(senderSocket < currentlyActiveClients && clientSocketList[senderSocket] != fd){
								senderSocket++;
							}
							strcpy(message,clientNameList[senderSocket]);
							strcat(message, "> ");
							strcat(message, messageBody);
							write(clientSocketList[recipientSocket], message, strlen(message));
							printf("Message sent\n");
						}
						else{
							strcpy(message, recipientName);
							strcat(message, " doesn't exist on server\n");
							write(fd, message, strlen(message));
						}
					}
				}
			}
		}
	}
}
