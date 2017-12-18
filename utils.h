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
#define DEFAULT_PORT 12345
#define MAXCONN 5
#define MAX_CLIENTS 10

#define Bienvenue "Connection au serveur\n"

#define LIST "list"

#define PSEUDO "[pseudo]"

#define MSGTAG "[msg]"

#define ENVOI "envoi "

#define USERNAMETAKEN "[UsernameTaken]"
