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

#define BIENVENUE "Connexion etablie\n"

#define FIN "Fin connexion"
#define T_FIN strlen(FIN)

#define PSEUDO "/pseudo"
#define T_PSEUDO strlen(PSEUDO)

#define LISTE "liste"
#define T_LISTE strlen(LISTE)

#define ENVOI "envoi "
#define T_ENVOI strlen(ENVOI)

#define LISTERETOUR "Liste des connectés :\n"
#define T_LISTERETOUR strlen(LISTERETOUR)

#define NOMDEST "nomdest "
#define T_NOMDEST strlen(NOMDEST)

#define UTILISATEUR_EXISTANT "Utilisateur existant\n"
#define T_UTILISATEUR_EXISTANT strlen(UTILISATEUR_EXISTANT)

#define ETAT_ENTRER_NOUVEAU_NOM 1
#define ETAT_INDIFFERENT 0


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
	if( argc != 4) {
		fprintf(stderr, "%s <serveur> <port> <pseudo> \n", argv[0]);
		return 1;
	}

  /* On recupere le nom du client */
	strcpy(pseudo, argv[3]);

	/* Le port ou se trouve le serveur */
	port = atoi(argv[2]);

  /* On recupere l'adresse du serveur et on met à jours les informations de la socket (adresse à joindre, port destinataire et type de socket) */
	serveur = argv[1];


  /* serveur est le nom (ou l'adresse IP) auquel le client va acceder */
	/* port le numero de port sur le serveur correspondant au  */
	/* service desire par le client */

	printf("Connexion au serveur %s via le port %d -\n", serveur, port);

	client_appli(serveur, port, pseudo);
}


// établie la connexion au serveur via le protocole et le port donné
int connexion(char *serveur, int port, char *protocole){
  struct sockaddr_in *adr_serveur;
  int res;

	adr_serveur = (struct sockaddr_in *) malloc (sizeof(struct sockaddr_in));
  //  bzero( (void *)sock_serveur, sizeof *sock_serveur );
	adr_serveur->sin_family = AF_INET;
  adr_serveur->sin_port = htons(port);
  adr_serveur->sin_addr.s_addr = inet_addr( serveur );

  /* on créé la socket */
	int num_socket = socket(AF_INET,SOCK_STREAM,0);

	/////////////////adr_socket(port, NULL, SOCK_STREAM, &adr_serveur);

  if( adr_serveur->sin_addr.s_addr != -1 ){
		/* On se connecte au serveur */
		res = connect(num_socket, (struct sockaddr *)adr_serveur, sizeof(*adr_serveur) );
    //connect(num_socket,(struct sockaddr *)&sonadr, sizeof(sonadr));
	}else {
		/* Le serveur est designe par son nom, il faut
		 * alors lancer une requete DNS.
		 */
		struct hostent *host;

		host = gethostbyname(serveur);
		if( host == NULL ){
			switch( h_errno ) {
			     case HOST_NOT_FOUND: fprintf(stderr, "Serveur introuvable : "); break;
			     case NO_ADDRESS: fprintf(stderr, "Pas d'adresse IP pour ce serveur :"); break;
			     case NO_RECOVERY: fprintf(stderr, "Erreur fatale du serveur de noms : "); break;
			     case TRY_AGAIN: fprintf(stderr, "Serveur de noms indisponible, reessayez plus tard :"); break;
			}
		}else{
			for(int i=0, res=-1; (res==-1) && (host->h_addr_list[i] != NULL ) ; i++) {
				/* On essaie de se connecter au serveurs trouvés par la requête DNS */
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

  char *protocole = PROTOCOLE_DEFAUT; /* protocole par defaut */
  char *message = (char *)malloc(MAXLEN*sizeof(char));
	char *buf = (char *)malloc(MAXLEN*sizeof(char));
	char *nomDest;

	/* Creation de variables pour la liste des sockets a écouter */
	int ndfs, envoiEnCours=0, etat=0;
	fd_set liste, liste2;


  int num_socket = connexion(serveur, port, protocole);

  /* On envoie le pseudo du client au serveur */
	strcpy(message,"pseudo : ");
	strcat(message,pseudo);
	write(num_socket, message, strlen(message)+1 );

	/* On attend le message de bienvenue */
	read(num_socket, message, strlen(BIENVENUE)+1);
	printf("%s",message);

	/* On initialise la table des sockets a écouter */
	ndfs = getdtablesize();
	FD_ZERO(&liste);

	/* La socket 0 correspond a une saisie clavier validee par Enter */
	FD_SET(0, &liste);
	FD_SET(num_socket, &liste);


	while( 1 ) {
		/* Nettoyage du buffer */
		bzero(buf, MAXLEN);

		/* Copie des listes de sockets */
		memcpy(&liste2, &liste, sizeof(liste2));

		/* Ecoute des sockets */
		if(select(ndfs, & liste2, NULL, NULL, NULL) == -1 ){
			/* Si on a pas eu d information sur la socket, c'est peut etre qu elle est interrompue (on a rien recu) */
			if( errno == EINTR ) continue;

			fprintf(stderr, "select: %s", strerror(errno));
			exit(1);
		}
		if( FD_ISSET(0, &liste2) ) {
			/* Ecriture du client sur l'entree standard */
			buf = fgets(buf, MAXLEN, stdin);
			/* S'il n'y a pas eu de probleme avec le nom */
			if (etat == ETAT_INDIFFERENT){
				/* Et que l'utilisateur est en train d'envoyer un message */
				if (envoiEnCours){
					strcat(message, buf);
					write( num_socket, message, strlen(message)+1 );
					fflush(stdout);
					envoiEnCours=0;
				}
				/* Si l'utilisateur veut se deconnecter */
				else if( !strncmp(buf, FIN, T_FIN) ){
					/* demande de fin de connexion */
					sprintf(buf, "Deconnexion de %s \n",pseudo);
					write( num_socket, buf, strlen(buf)+1 );
					close(num_socket);
					return ;
				}
				/* Si l'utilisateur demande la liste des cients présents */
				else if( !strncmp(buf, LISTE, T_LISTE) ){
					/* demande de la liste des connectés */
					write( num_socket, buf, strlen(buf)+1 );
				}

				/* S'il demande l envoi de message */
				else if( !strncmp(buf, ENVOI, T_ENVOI) ){
					/* demande d'envoi d un message (Envoi nomDestinataire)*/
					nomDest = (char *)malloc(strlen(buf)-T_ENVOI);
					strncpy(nomDest,buf+T_ENVOI,strlen(buf)-T_ENVOI);
					strcpy(message,NOMDEST);

					char *nbr=(char *)malloc(sizeof(char)) ;
					sprintf(nbr,"%d",(int)strlen(nomDest)-1);
					strcat(message,nbr);
					strcat(message," ");
					strncat(message,nomDest,strlen(nomDest)-1);
					strcat(message, " ");
					envoiEnCours =1;
					printf("Message : ");
					fflush(stdout);
				}
			}
			else {
				/* Dans ce cas l'utilsateur a voulu se connecter avec un nom deja utilisé, on lui en redemande donc un */
				strcpy(message,"Pseudo : ");
				char *p = strchr(buf,'\n');
				if (p!=NULL) *p='\0';
				strcat(message,buf);
				write(num_socket, message, strlen(message)+1 );
				etat = ETAT_INDIFFERENT;
			}
		}
		/* Réception de données depuis la socket */
		if(FD_ISSET(num_socket, &liste2)) {
			/* S'il y a eu un probleme de lecture */
			if( read(num_socket, buf, MAXLEN) == NULL){
				printf("Déconnexion du serveur distant\n");
				close (num_socket);
				exit(1);
			/* Sinon */
			} else{
				/* Si le serveur informe l'utilisateur que son nom est deja utilisé */
				if(strncmp(buf,UTILISATEUR_EXISTANT,T_UTILISATEUR_EXISTANT)==0){
					printf("Le pseudo choisi est déjà utilisé \n");
					printf("Entrez un nouveau nom :\n");
					/* On change d'état pour ne plus permettre l'envoi de message ou autres */
					etat = ETAT_ENTRER_NOUVEAU_NOM;
					fflush(stdout);
				} else {
					/* On affiche les données qui viennent d arriver (message) */
					printf("%s",buf);
					fflush(stdout);
				}
			}
		}
	}

  close(num_socket);
}
