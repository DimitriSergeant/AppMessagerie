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
#include <sys/socket.h>
#include <arpa/inet.h>

#include <signal.h>

#define BUFSIZE 1024
#define MAX_CLIENTS 10
#define MAX_CONNECTIONS 5

#define DEBUT "Connexion etablie \n"
#define FIN "quit"
#define LISTE "liste"
#define ENVOI "envoi"
#define PSEUDO "pseudo : "
#define NOMDEST "nomdest "

#define UTILISATEUR_EXISTANT "Utilisateur existant\n"

#define PORT_DEFAUT 1111
#define PROTOCOLE_DEFAUT "TCP"


char *lecture_client(int socket){
	char *message = (char *)malloc(BUFSIZE * sizeof(char));
	int nb_lu;
	nb_lu = read(socket, message, BUFSIZE);

	if( nb_lu > 0 ) {
		printf("Message: %s\n", message);
		fflush(stdout);
		return message;
	}
	else{
		return NULL;
	}
}



int main(int argc, char const *argv[])
{
	struct sockaddr_in *adr_serveur, *adr_client;
	fd_set listeClient, listeCopie;
	int socket_client;
	int opt = 1, nb_co=0;
	char *message;

	int port = PORT_DEFAUT;

// On vérifie que l'appel au programme est bien fait
	switch( argc ) {
		case 1: printf("defaut service = %i\n", port); break;
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
	FD_SET(socket_serv, & listeCopie);

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

	// Si la requête concernée est la socket de base, c'est qu'un client veut se connececter
		if( FD_ISSET(socket_serv, &listeClient) ) {
			int taille = sizeof *adr_client;
			if( (socket_client = accept(socket_serv,(struct sockaddr *)adr_client, &taille)) == -1 ) {
				fprintf(stderr, "connexion impossible\n");
				exit(1);
			}
			printf("Connexion d'un client depuis %s \n", inet_ntoa(adr_client->sin_addr));
			fflush(stdout);

			//On ajoute le client dans les sockets sous surveillance
			FD_SET(socket_client, &listeCopie);
		//On confirme sa connection au client puis on lui donne la liste des autres connectés
			write(socket_client, DEBUT, strlen(DEBUT)+1);
			message = (char *)malloc (nb_co*BUFSIZE*sizeof(char));


			if (nb_co==0) {
				strcat(message,"Il n'y a personne de connecte pour le moment\n");
			}else{
				strcpy(message,"Liste des connectes :\n");
				for (int i=0;i<nb_co;i++){
					strcat(message,listeNom[i]);
					strcat(message,"\n");
				}
			}
			nb_co++;
			printf("%s\n",message);
			write(socket_client,message,strlen(message));
		}
	}

	// On vérifie si les sockets des clients ont bougé*/
	for(int fd=0; fd<nbClient; fd++ ){
		if( fd != socket_serv && FD_ISSET(fd, &listeClient) ){
				//Si un client déconnecte
			if( (message = lecture_client(fd)) == NULL ) {
				int i=0;
					//On cherche qui a déconnecté
				while(  i<nb_co && listeNumSocket[i]!=fd ){
					i++;
				}
				if (i<nb_co){
					printf("Deconnexion de : %s\n",listeNom[i]);
						//On cherchele nom du client pour le supprimer
					while(i<nb_co-1){
						listeNumSocket[i]=listeNumSocket[i+1];
						listeNom[i] = listeNom[i+1];
						i++;
					}
					nb_co--;
				}
					//Et on clot la socket correspondante
				close(fd);
				FD_CLR(fd, &listeCopie);
			}
			else{
					//On ajoute un client
				if (strncmp(message,PSEUDO,strlen(PSEUDO))==0){
					printf("verification du pseudo \n");
					printf("%s \n",message+strlen(PSEUDO));

						//On vérifie que le pseudo n'est pas déjà utilisé
					int i=0;
					int doublon_nom = 0;
					for(int i=0;i<nb_co;i++){
						if (strlen(message+strlen(PSEUDO))==strlen(listeNom[i])){
							if(strncmp(message+strlen(PSEUDO),listeNom[i],strlen(message)-strlen(PSEUDO)-1)==0){
									//Le client doit donc changer de peseudo
								write( fd, UTILISATEUR_EXISTANT, strlen(UTILISATEUR_EXISTANT)+1 );
								doublon_nom = 1;
								break;
							}
						}
					}

						//Si le pseudo esy libre
					if(doublon_nom==0){
							/* On l'ajoute à la liste des noms */
						listeNom[nb_co]= (char *) malloc (BUFSIZE*sizeof(char));
						strncat(listeNom[nb_co],message+strlen(PSEUDO),strlen(message)-strlen(PSEUDO));

						listeNumSocket[nb_co] = (int) malloc(sizeof(int));
						listeNumSocket[nb_co] = socket_client;
						nb_co++;
					}
				}

					//Affichage de la liste des connectés
				else if (strncmp(message,LISTE,strlen(PSEUDO))==0){
					message = (char *)malloc (nb_co*BUFSIZE*sizeof(char));
					strcpy(message,"connectés :\n");
					if (nb_co==1) strcat(message,"Personne n'est en ligne\n");
					for (int i=0;i<nb_co;i++){
						if(listeNumSocket[i]!=fd){
							strcat(message,listeNom[i]);
							strcat(message,"\n");
						}
					}
					write(fd,message,strlen(message));
				}

					//Envoie des messagees
				else if (strncmp(message,NOMDEST,strlen(NOMDEST))==0){
					char *nomDestinataire= (char *)malloc(BUFSIZE * sizeof(char));
					char *messageDest= (char *)malloc(BUFSIZE * sizeof(char));
					char *c = (char *)malloc(sizeof(char));
					strncpy(c,message+strlen(NOMDEST),1);
					int lg = atoi(c);
					strncpy(nomDestinataire,message+strlen(NOMDEST)+2,lg);
					strncpy(messageDest,message+strlen(NOMDEST)+lg+2,strlen(message)-lg-3);

						//Recherche de la socket liée
					int i=0;
					while(i < nb_co && strcmp(listeNom[i],nomDestinataire)!=0){i++;}

					if(i<nb_co){
					//Recherche de l'émetteur
						int j=0;
						while(j<nb_co && listeNumSocket[j]!=fd){j++;}
						strcpy(message,listeNom[j]);
						strcat(message," dit : ");
						strcat(message,messageDest);
						printf("%s\n",message);
						write(listeNumSocket[i],message,strlen(message));
					}
					else{
						write(fd,"Le pseudo de destinataire n'existe pas\n",strlen("Le pseudo de destinataire n'existe pas\n"));
					}
				}
			}
		}
	}
}
