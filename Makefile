EXEC=client serveur

all:$(EXEC)

client : client.c
	gcc -g client.c -o client -Wall

serveur : serveur.c
	gcc -g serveur.c -o serveur -Wall

clean:
	rm $(EXEC)
