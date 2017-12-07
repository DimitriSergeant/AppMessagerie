# Makefile

EXEC = client serveur
all: ${EXEC}

client.o : client.c
	gcc -c client.c -g

serveur.o : serveur.c
	gcc -c serveur.c -g

client: client.c
	gcc -o client client.c

serveur: serveur.c
	gcc -o serveur serveur.c

clean:
	rm -vf ${EXEC} *.o
