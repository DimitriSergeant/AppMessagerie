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


/* serveur.c
 * Serveur de chat:
 *  .gestion des connexions
 *  .distribution des messages
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>

#include <signal.h>

#define BUFSIZE 1024
#define MAX_CLIENTS 10
#define MAX_CONNECTIONS 5

#define DEBUT "Connexion etablie \n"
#define FIN "quit"
#define LISTE "liste"
#define ENVOI "envoi "
#define PSEUDO "pseudo : "
#define NOMDEST "nomdest "

#define UTILISATEUR_EXISTANT "Utilisateur existant\n"

#define PORT_DEFAUT 1111
#define PROTOCOLE_DEFAUT "TCP"

int main(int argc, char const *argv[])
{
	struct sockaddr_in *adr_serveur, *adr_client;
	fd_set listeClient, listeCopie;
	int socket_client;
	int opt = 1, port;
	char *message;

	int port = PORT_DEFAUT;

// On vérifie que l'appel au programme est bien fait
	switch( argc ) {
		case 1: printf("defaut service = %s\n", port); break;
		case 2: port = atoi(argv[1]); break;
		default:
		fprintf(stderr,"Usage: %s [port]\n",argv[0]);
		break;
	}
	printf("On crée le serveur ...");

//Nombre de clients actuellement connectés
	int nbClient;

//Liste des clients connectés
	char * listeNom[MAX_CLIENTS];
	int listeNumSocket[MAX_CLIENTS];

//Création de la socket du serveur
	int socket_serv = socket(PF_INET, SOCK_STREAM, 0);

//Initialisation de la socket
	bzero( (char *)adr_serveur,sizeof *adr_serveur);

//Mise à jour des informations de la socket
	adr_serveur->sin_family = AF_INET;
	adr_serveur->sin_port = htons(port);
	adr_serveur->sin_addr.s_addr = INADDR_ANY;

//Cette option permet de réutiliser l'adresse
	setsockopt(socket_serv, SOL_SOCKET, SO_REUSEADDR,(char *) &opt, sizeof(opt));

//On associe la socket au port
	if (bind(socket_serv, (struct sockaddr *)adr_serveur, sizeof(*adr_serveur)) == -1){
		fprintf(stderr, "Echec\n");
		exit(1);
	}

//On met notre socket sur écoute
	if (listen(socket_serv, MAX_CONNECTIONS) == -1){
		fprintf(stderr, "Echec de l'écoute\n");
		exit(1);
	}
	printf("[OK]: Serveur en attente sur le port %d\n", port);

	//On intialise les listes de sockets
	nbClient = getdtablesize();
	FD_ZERO(&listeCopie);
	FD_SET(serv_sock, & listeCopie);

//Gestion des clients
while( 1 ) {
	//On copie les listes de sockets
	memcpy(&listeClient, &listeCopie, sizeof(listeClient));

	// On lance la surveillance des descripteurs en lecture
	if( select(nbClient, & listeClient, 0, 0, 0) == -1 ) {
		//On vérifie qu'on a bien reçu quelquechose
		if( errno == EINTR ) continue;
		fprintf(stderr,"select: %s", strerror(errno));
		exit(1);
	}

	/* Si on recoit sur la socket de base, c'est qu'un client veux se connecter */
	if( FD_ISSET(socket_serv, &listeClient) ) {
		taille = sizeof *adr_client;
		if( (socket_client = accept(socket_serv,(struct sockaddr *)adr_client, &taille)) == -1 ) {
			fprintf(stderr, "connexion impossible\n");
			exit(1);
		}
		printf("Connexion d'un client depuis %s \n", inet_ntoa(*adr_client.sin_addr));
		fflush(stdout);

			/* Ajout du client dans les sockets a surveiller */
		FD_SET(socket_client, &listeCopie);

			/* Souhaiter la bienvenue au client puis lui envoyer la liste des connectés*/
		write(socket_client, BIENVENUE, strlen(BIENVENUE)+1);
		message = (char *)malloc (k*BUFSIZE*sizeof(char));

		strcpy(message,"Liste des connectes :\n");
		if (k==0)strcat(message,"Il n'y a personne de connecte pour le moment\n");
		for (i=0;i<k;i++){
			strcat(message,listeNom[i]);
			strcat(message,"\n");
		}
		printf("%s\n",message);
		write(socket_client,message,strlen(message));
	}
}
