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
#define BIENVENUE "Connexion etablie\n"

struct sockaddr_in *adr_serveur, *adr_client;

//Nombre de clients actuellement connectés
int nbClient; 

//Liste des clients connectés
char * listeNom[MAX_CLIENTS];
int listeNumSocket[MAX_CLIENTS];

//Création de la socket du serveur
int socket_serv = socket(PF_INET, SOCK_STREAM, 0);

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