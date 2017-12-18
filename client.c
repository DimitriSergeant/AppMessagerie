
/******************************************************************************/
/*							Application: Messagerie	multi-utilisateurs										*/
/******************************************************************************/
/*									      																										*/
/*			 									programme CLIENT    		      											*/
/*									      																										*/
/******************************************************************************/
/*									      																										*/
/*		Auteurs : Dimitri SERGEANT , Léo VALETTE											 					*/
/*									      																										*/
/******************************************************************************/
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

void setServeurAdressString(char **argv, char *serveurAdressString) {
	strcpy(serveurAdressString, argv[1]);
}

int initSocket(struct sockaddr_in sin_serveur, int port, char* IPServeur) {
	int num_socket;
	int connectReturn;

	// AF_INET -> on utilise IPv4
	sin_serveur.sin_family = AF_INET;
	// On convertie le le port entier en port utilisable
	sin_serveur.sin_port = htons(port);
	// Convertion pour utiliser l'adresse IPv4
	sin_serveur.sin_addr.s_addr = inet_addr(IPServeur);

	//création du socket
	num_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (sin_serveur.sin_addr.s_addr == -1) {
		printf("Erreur Socket\n");
		exit(EXIT_FAILURE);
	} else {
		// Connexion au serveur
		// num_socket -> l'id du socket du serveur
		// sin_serveur -> pointeur vers un sockaddr
		struct sockaddr *sin = (struct sockaddr *) &sin_serveur ;
		connectReturn = connect(num_socket, sin, sizeof(sin_serveur) );
	}
	if (connectReturn != 0) {
		printf("Impossible d'acceder au serveur %s depuis le port %i\n ", IPServeur, port);
		exit(EXIT_FAILURE);
	}else {
		printf("-- Connexion etablie --\n");
		printf("Connexion au serveur %s depuis le port : %i \n", IPServeur, port);
	}
	return num_socket;
}

void envoiPseudo(char* message, char* pseudo, int num_socket) {

	// envoi le flag PSEUDO au serveur, ainsi que le pseudo de l'utilisateur
	strcpy(message, PSEUDO);
	strcat(message, pseudo);
	write(num_socket, message, strlen(message) + 1);

	// Recoit et affiche le message de bienvenue
	read(num_socket, message, strlen(BIENVENUE) + 1);
	printf("%s", message);
}



void ActionUtilisateur(char* message, char* buf, char* pseudo, int num_socket, int *envoiEnCours) {
	// Si l'utilisateur est en train d'envoyer un message
	if (*envoiEnCours) {
		strcat(message, buf);
		write(num_socket, message, strlen(message) + 1);
		fflush(stdout);
		*envoiEnCours = false;
	}
	// Si l'utilisateur se déconnecte
	else if (strncmp(buf, QUIT, strlen(QUIT)) == 0) {
		// Send info to serveur
		sprintf(buf, "Déconnexion de %s\n", pseudo);
		write(num_socket, buf, strlen(buf) + 1);
		// Close socket and end application
		close(num_socket);
		exit(EXIT_SUCCESS);
	}
	// Si l'utilisateur demande la liste des personnes connectées
	else if (strncmp(buf, LISTE, strlen(LISTE)) == 0) {
		write(num_socket, buf, strlen(buf) + 1);
	}
	// Si l'utilisateur souhaite envoyer un message
	else if (strncmp(buf, ENVOI, strlen(ENVOI)) == 0) {
		char *recipientpseudoLength = (char *) malloc(sizeof(char)) ;
		char *recipientpseudo = (char *) malloc(strlen(buf) - strlen(ENVOI));
		strncpy(recipientpseudo, buf + strlen(ENVOI), strlen(buf) - strlen(ENVOI));
		strcpy(message, MSGTAG);
		sprintf(recipientpseudoLength, "%lu", strlen(recipientpseudo) - 1);

		strcat(message, recipientpseudoLength); strcat(message, " ");
		strncat(message, recipientpseudo, strlen(recipientpseudo) - 1); strcat(message, " ");

		*envoiEnCours = true;
		printf("Message : "); fflush(stdout);
	}
}

void demandePseudo(char* message, char* buf, int num_socket, int* etatPseudo) {

	// ajout d'un caractère '\0' a la fin de la string
	char *p = strchr(buf, '\n');
	if (p != NULL) {
		*p = '\0';
	}

	// envoi du flag PSEUDO ainsi que le pseudo de l'utilisateur au serveur
	strcpy(message, PSEUDO);
	strcat(message, buf);
	write(num_socket, message, strlen(message) + 1);

	// met l'etat du pseudo a default : il n'a pas besoin de redemander un pseudo
	*etatPseudo = DEFAULT_STATE;
}


void lectureMessageServeur(char *buf, int socket, int *etatPseudo) {

	int readResult;
	readResult = read(socket, buf, BUFSIZE);

	// Erreur de lecture
	if (readResult == -1) {
		perror("read() \n");
		printf("Déconnexion du serveur\n");
		close (socket);
		exit(EXIT_FAILURE);
	} else {
		if (readResult == 0) {
			printf("Serveur introuvable\n");
			printf("Déconnexion du serveur\n");
			close (socket);
			exit(EXIT_FAILURE);
		} else {
			// Si le serveur dit que le pseudo est déjà utilisé
			if (strncmp(buf, PSEUDOPRIS, strlen(PSEUDOPRIS)) == 0) {
				printf("Pseudo déjà utilisé\n");
				printf("Veuillez en saisir un nouveau :\n");
				// Change etatPseudo pour eviter un nouveau message
				*etatPseudo = NOUVEAU_PSEUDO;
				fflush(stdout);
			} else {
				// Affichage du message du serveur
				printf("%s", buf);
				fflush(stdout);
			}
		}
	}
}



int main( int argc, char**argv ) {

	char *pseudo = (char *) malloc(BUFSIZE * sizeof(char));
	char *serveurAdressString = (char *) malloc(BUFSIZE * sizeof(char)); // adresse IP du serveur ("127.0.0.1" par exemple)
	int portServeur ;
	int num_socket ;
	char * message = (char *) malloc (BUFSIZE * sizeof(char));
	int maxClients;
	fd_set socketList, currentWorkingList ;
	int etatPseudo = DEFAULT_STATE ;
	bool envoiEnCours = false ;
	struct sockaddr_in sin_serveur;


	char *buf = (char *)malloc(BUFSIZE * sizeof(char));


	printf("Application de Tchat\n");

	checkUsage(argc, argv);

	setPseudo(argv, pseudo);

	setServeurAdressString(argv, serveurAdressString);
	setPortServeur(argv, &portServeur);

	// Initialisation du socket et des informations correspondantes
	num_socket = initSocket(sin_serveur, portServeur, serveurAdressString);
	envoiPseudo(message, pseudo, num_socket);

	// On récupère le nombre maximum de clients disponibiles
	maxClients = getdtablesize();

	// On rempli socketlist de la valeur 0
	FD_ZERO(&socketList);

	// Initialisation des descripteurs à surveiller en lecture
  // entrée standard
	FD_SET(STDIN_FILENO, &socketList);

	// Ajoute le socket à la liste
	FD_SET(num_socket, &socketList);

	while (true) {
		// Reinitialisation du buffer
		bzero(buf, BUFSIZE);
		//sauvegarde de la liste avec laquelle on travaille
		memcpy(&currentWorkingList, &socketList, sizeof(currentWorkingList));

		if (select(maxClients, &currentWorkingList, 0, 0, 0) == -1) {
			// Si on ne recois rien
			if (errno == EINTR) {
				continue;
			}
			fprintf(stderr, "Select error: %s", strerror(errno));
			exit(EXIT_FAILURE);
		}
		// Si on recois des informations depuis stdin
		if (FD_ISSET(STDIN_FILENO, &currentWorkingList)) {
			// on récupère ce que le client écrit dans notre buffer
			buf = fgets(buf, BUFSIZE, stdin);
			//Si le pseudo demandé correspond a un utilisé par un autre utilisateur
			if (etatPseudo != NOUVEAU_PSEUDO) {
				//On parse l'instruction de l'utilisateur et on execute
				ActionUtilisateur(message, buf, pseudo, num_socket, &envoiEnCours);
			} else {
				// on demande si le PSEUDO est valide
				demandePseudo(message, buf, num_socket, &etatPseudo);
			}
		}
		// si on recoit quelquechose du serveur
		if (FD_ISSET(num_socket, &currentWorkingList)) {
			// On regarde ce que le serveur nous envoie et on affiche/execute l'action appropriée
			lectureMessageServeur(buf, num_socket, &etatPseudo);
		}
	}
	close(num_socket);
	return 0;
}
