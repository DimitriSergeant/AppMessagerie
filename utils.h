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
#include <arpa/inet.h>

typedef int bool;
#define true 1
#define false 0

#define BUFSIZE 1024
#define PSEUDOSIZE 50
#define DEFAULT_PORT 1111
#define MAXCONN 5
#define MAX_CLIENTS 10

#define BIENVENUE "Connection au serveur\n"

#define LISTE "liste"

#define PSEUDO "[pseudo]"

#define MSGTAG "[msg]"

#define ENVOI "envoi "

#define PSEUDOPRIS "[PseudoPris]"
