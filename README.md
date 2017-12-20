# AppMessagerieRSX
Programmation d'une application de messagerie instantanée en utilisant les sockets

Lancement de l'application :

make
Ouvrir plusieurs terminaux

premier terminal:
./serveur <port>
Exemple port 1234 (optionnel car port défaut 1111)

autres terminaux:
./client <serveur> <port> <pseudo>
Exemple serveur : 127.0.0.1
Port : celui entré au dessus ou le par défaut
Pseudo : pseudo de l'utilisateur


Commandes disponibles pendant l'utilisation de l'application:

- envoi <pseudo>
- liste
- quit
