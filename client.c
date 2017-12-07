/******************************************************************************/
/*							Application: Messagerie	multi-utilisateurs										*/
/******************************************************************************/
/*									      																										*/
/*			 									programme  CLIENT				      											*/
/*									      																										*/
/******************************************************************************/
/*									      																										*/
/*		Auteurs : Dimitri SERGEANT , Léo VALETTE											 					*/
/*									      																										*/
/******************************************************************************/


/* client.c
 * Arguments : nom du serveur, port, pseudo du client
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//#include <signal.h>

#define MAXLEN 1024

#define PROTOCOLE_DEFAUT "TCP"

#define DEBUT "Connexion etablie \n"
#define FIN "quit"
#define LISTE "liste"
#define ENVOI "envoi "
#define NOMDEST "nomdest "
#define UTILISATEUR_EXISTANT "Utilisateur existant\n"

#define ETAT_NOUVEAU_NOM 1
#define ETAT_AUTRE 0


void client_appli (char *serveur, int port, char *pseudo);


/**********************************************************/
/*--------------- programme client -----------------------*/


int main( int argc, char**argv )
{
	/* Definition de variables pour le programme */
	int port;
	char *pseudo = (char *)malloc(MAXLEN*sizeof(char));
	char *serveur = (char *)malloc(MAXLEN*sizeof(char));

	/* On vérifie que l'appel au programme est bien fait */
	if( argc != 3) {
		fprintf(stderr, "%s <serveur> <port> \n", argv[0]);
		return 1;
	}
  /* On recupere le pseudo du client */
	printf("Pseudo: ");
	scanf(" %s", pseudo);
	printf("\n");
	/* On recupère le port ou se trouve le serveur */
	port = atoi(argv[2]);
	/* on récupère l'adresse du serveur */
	serveur = argv[1];

  /* serveur est le nom (ou l'adresse IP) auquel le client va acceder */
	/* port le numero de port sur le serveur correspondant au  */
	/* service desire par le client */
	printf("Connexion au serveur %s depuis le port %d -\n", serveur, port);

	client_appli(serveur, port, pseudo);
}


// établie la connexion au serveur via le protocole et le port donné
int connexion(char *serveur, int port, char *protocole){
  struct sockaddr_in *adr_serveur;
  int res;
	struct hostent *host;

	adr_serveur = (struct sockaddr_in *) malloc (sizeof(struct sockaddr_in));
  //  bzero( (void *)sock_serveur, sizeof *sock_serveur );

	/* On met à jours les informations de la socket : adresse à joindre, port destinataire et type de socket */
	adr_serveur->sin_family = AF_INET;
  adr_serveur->sin_port = htons(port);
  adr_serveur->sin_addr.s_addr = inet_addr( serveur );

  /* on créé la socket */
	int num_socket = socket(AF_INET,SOCK_STREAM,0);

	/////////////////adr_socket(port, NULL, SOCK_STREAM, &adr_serveur);

  if( adr_serveur->sin_addr.s_addr != -1 ){
		/* On se connecte au serveur */
		res = connect(num_socket, (struct sockaddr *)adr_serveur, sizeof(*adr_serveur) );
	}else{
		/* Le serveur est designe par son nom, on va donc lancer une requete DNS. */
		host = gethostbyname(serveur);

		if( host == NULL ){
			switch( h_errno ) {
			     case HOST_NOT_FOUND: fprintf(stderr, "Serveur introuvable : "); break;
			     case NO_ADDRESS: fprintf(stderr, "Pas d'adresse IP pour ce serveur :"); break;
			     case NO_RECOVERY: fprintf(stderr, "Erreur fatale du serveur de nom : "); break;
			     case TRY_AGAIN: fprintf(stderr, "Serveur de nom indisponible, reessayez plus tard :"); break;
			}
		}else{
			for(int i=0, res=-1; (res==-1) && (host->h_addr_list[i] != NULL ) ; i++) {
				/* On essaie de se connecter au serveur trouvé par la requête DNS */
				bcopy( (char *) host->h_addr_list[i], (char *)&(adr_serveur->sin_addr), sizeof(adr_serveur->sin_addr) );
				res = connect(num_socket, (struct sockaddr *)adr_serveur, sizeof(*adr_serveur) );
			}
		}
  }
  if( res != 0 ) {
    fprintf(stderr, "Service distant [port %d] inaccessible sur le serveur %s \n ", port, serveur );
    return -1;
  }
	return num_socket;
}

/*****************************************************************************/
void client_appli (char *serveur, int port, char *pseudo){

  char *protocole = PROTOCOLE_DEFAUT; /* TCP */
  char *message = (char *)malloc(MAXLEN*sizeof(char));
	char *buf = (char *)malloc(MAXLEN*sizeof(char));
	char *nomDest;

	/* Creation de variables pour la liste des sockets a écouter */
	int table_size, etat=0;
	int envoiUtilisateur = 0;
	fd_set liste, liste2;

  int num_socket = connexion(serveur, port, protocole);

  /* On envoie le pseudo du client au serveur */
	strcpy(message,"pseudo : ");
	strcat(message,pseudo);
	write(num_socket, message, strlen(message)+1 );

	/* On attend le message de bienvenue */
	read(num_socket, message, strlen(DEBUT)+1);
	printf("%s",message);

	/* On initialise la table des sockets a écouter */
	table_size = getdtablesize();
	FD_ZERO(&liste);

	/* La socket 0 correspond a une saisie clavier achevée par l'utilisation de la touche entrée*/
	FD_SET(0, &liste);
	FD_SET(num_socket, &liste);

	while( 1 ) {
		/* Mise à 0 de tous les bits du buffer */
		bzero(buf, MAXLEN);

		/* Copie des listes de sockets */
		memcpy(&liste2, &liste, sizeof(liste2));

		/* Ecoute des sockets */
		if(select(table_size, &liste2, NULL, NULL, NULL) == -1 ){
			/* Si on a pas eu d information sur la socket, on test si elle n'a pas été interrompue */
			if( errno == EINTR ) continue;
			fprintf(stderr, "select: %s", strerror(errno));
			exit(1);
		}

		if( FD_ISSET(0, &liste2) ) {
			/* Ecriture du client sur l'entree standard */
			buf = fgets(buf, MAXLEN, stdin);
			/* S'il n'y a pas eu de probleme avec le nom */
			if (etat == ETAT_AUTRE){
				/* Si l'utilisateur est en train d'envoyer un message */
				if (envoiUtilisateur){
					strcat(message, buf);
					write( num_socket, message, strlen(message)+1 );
					fflush(stdout);
					envoiUtilisateur=0;
				}
				/* Si l'utilisateur veut se deconnecter */
				else if(!strncmp(buf, FIN, strlen(FIN))){
					/* demande au serveur la fin de connexion */
					sprintf(buf, "Deconnexion de %s \n",pseudo);
					write( num_socket, buf, strlen(buf)+1 );
					close(num_socket);
					return ;
				}
				/* Si l'utilisateur demande la liste des cients présents */
				else if(!strncmp(buf, LISTE, strlen(LISTE)) ){
					write( num_socket, buf, strlen(buf)+1 );
				}

				/* S'il demande l envoi de message */
				else if(!strncmp(buf, ENVOI, strlen(ENVOI)) ){
					/* demande d'envoi d un message (Envoi nomDestinataire)*/
					nomDest = (char *)malloc(strlen(buf)-strlen(ENVOI));
					strncpy(nomDest,buf+strlen(ENVOI),strlen(buf)-strlen(ENVOI));
					strcpy(message,NOMDEST);

					char *n=(char *)malloc(sizeof(char)) ;
					sprintf(n,"%d",(int)strlen(nomDest)-1);
					strcat(message,n);
					strcat(message," ");
					strncat(message,nomDest,strlen(nomDest)-1);
					strcat(message, " ");
					envoiUtilisateur = 1;
					printf("Message : ");
					fflush(stdout);
				}
			}
			else{
				/* Dans ce cas l'utilsateur a voulu se connecter avec un nom deja utilisé, il doit en saisir un nouveau */
				strcpy(message,"Pseudo : ");
				char *p = strchr(buf,'\n');
				if (p!=NULL) *p='\0';
				strcat(message,buf);
				write(num_socket, message, strlen(message)+1 );
				etat = ETAT_AUTRE;
			}
		}
		/* Réception de données depuis la socket */
		if(FD_ISSET(num_socket, &liste2)) {
			/* S'il y a eu un probleme de lecture */
			if( read(num_socket, buf, MAXLEN) == NULL){
				printf("Déconnexion du serveur distant\n");
				close (num_socket);
				exit(1);
			}else{
				/* Si le serveur informe l'utilisateur que son pseudo n'est pas disponible */
				if(strncmp(buf,UTILISATEUR_EXISTANT,strlen(UTILISATEUR_EXISTANT))==0){
					printf("Le pseudo choisi est déjà utilisé \n Pseudo :\n");
					/* On change d'état pour ne plus permettre l'envoi de message */
					etat = ETAT_NOUVEAU_NOM;
					fflush(stdout);
				} else {
					/* On affiche les données qui on été récéptionnées  */
					printf("%s",buf);
					fflush(stdout);
				}
			}
		}
	}
  close(num_socket);
}
