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
#define PORT 1234

#define BIENVENUE "Connexion etablie\n"
#define FIN "Fin connexion"

#define LISTE "liste"
#define T_LISTE strlen(LISTE)

#define NOM "nom : "
#define T_NOM strlen(NOM)

#define NOMDEST "nomdest "
#define T_NOMDEST strlen(NOMDEST)

#define UTILISATEUR_EXISTANT "Utilisateur existant\n"
#define T_UTILISATEUR_EXISTANT strlen(UTILISATEUR_EXISTANT)

struct sockaddr_in *adr_serveur, *adr_client;


int opt = 1, port;

//Nombre de clients actuellement connectés
int nbClient;

//Liste des clients connectés
char * listeNom[MAX_CLIENTS];
int listeNumSocket[MAX_CLIENTS];

//Création de la socket du serveur
int socket_serv = socket(PF_INET, SOCK_STREAM, 0);

//Initialisation de la socket


//Mise à jour des informations de la socket
adr_serveur->sin_family = AF_INET;
adr_serveur->sin_port = htons(port);
adr_serveur->sin_addr.s_addr = INADDR_ANY;

//Cette option permet de réutiliser l'adresse
setsockopt(socket_serv, SOL_SOCKET, SO_REUSEADDR,(char *) &opt, sizeof(opt));

//On associe la socket au port
if (bind(socket_serv, (struct sockaddr *)&adr_serveur, sizeof(adr_serveur)) == -1){
	fprintf(stderr, "Echec\n");
	exit(1);
}

//On met notre socket sur écoute
if (listen(socket_serv, MAX_CONNECTIONS) == -1){
	fprintf(stderr, "Echecde l'écoute\n");
	exit(1);
}
printf("[OK]: Serveur en attente sur le port %d\n", port);
