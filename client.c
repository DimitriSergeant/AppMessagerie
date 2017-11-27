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
#include <netinet/in.h>
#include <netdb.h>

#include <signal.h>

#define MAXLEN 1024

#define PROTOCOLE_DEFAUT "TCP"

#define BIENVENUE "Connexion etablie\n"

#define FIN "quit"
#define L_FIN strlen(FIN)


int main( int argc, char**argv )
{
	/* Definition de variables pour le programme */
	int port, i,envoiEnCours=0,etat=0;
	char *pseudo = (char *)malloc(MAXLEN*sizeof(char));
  char *serveur = (char *)malloc(MAXLEN*sizeof(char));
  char *nomDest;

	/* Creation de variables pour la liste des sockets a écouter */
	int ndfs;
	fd_set liste, listeAutre;

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
	/* service le numero de port sur le serveur correspondant au  */
	/* service desire par le client */

  printf("Connexion au serveur %s via le port %d -\n", adr_serveur, port);

	client_appli(serveur, port, pseudo);
}


int connection(char *serveur,char *service,char *protocole){
  struct sockaddr_in *adr_serveur;
  int res;

	adr_serveur = (struct sockaddr_in *) malloc (sizeof(struct sockaddr_in));
  //  bzero( (void *)sock_serveur, sizeof *sock_serveur );
	adr_serveur->sin_family = AF_INET;
  adr_serveur->sin_port = htons(port);
  adr_serveur->sin_addr.s_addr = inet_addr( serveur );

  /* on créé la socket */
	int num_socket = socket(AF_INET,SOCK_STREAM,0);

	adr_socket(service, NULL, SOCK_STREAM, &adr_serveur);

  if( adr_serveur->sin_addr.s_addr != -1 ){
		/* On se connecte au serveur */
		res = connect(num_socket, adr_serveur );
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
			for( i=0, res=-1; (res==-1) && (host->h_addr_list[i] != NULL ) ; i++) {
				/* On essaie de se connecter au serveurs trouvés par la requête DNS */
				bcopy( (char *) host->h_addr_list[i], (char *)&(adr_serveur.sin_addr), sizeof(adr_serveur.sin_addr) );
				res = connect(num_socket, (struct sockaddr *)&adr_serveur, sizeof(adr_serveur));
			}
		}
  }
  if( res != 0 ) {
    fprintf(stderr, "Service distant [port %s] inaccessible sur le serveur %s \n ", argv[2], argv[1] );
    return -1;
  }

	return num_socket;
}



/*****************************************************************************/
void client_appli (char *serveur,char *port, char *pseudo){

  char *protocole = PROTOCOLE_DEFAUT; /* protocole par defaut */
  char *message = (char *)malloc(MAXLEN*sizeof(char));

  int num_socket = connection(serveur,service,protocole);

  /* On envoie le pseudo du client au serveur */
	strcpy(message,"pseudo : ");
	strcat(message,nom);
	write(num_socket, message, strlen(message)+1 );


  close(num_socket);
}
