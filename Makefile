# Makefile

all: client serveur

client: client.c
	gcc -o client client.c

serveur: serveur.c
	gcc -o serveur serveur.c

clean:
	rm -vf *.o
