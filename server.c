/* Mirciu Andrei-Constantin */
/* 323CD */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include<stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdbool.h> 
#include <arpa/inet.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	// sockfd este socketul TCP, iar sockfd2 este socketul UDP
	int sockfd, sockfd2, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret, ret2;
	// exit_command folosit pentru a putea inchide conexiunea
	// serverului (0 - server deschis, 1 - server inchis);
	// nr_sockets folosit pentru a retine numarul maxim
	// de clienti conectati la server
	int exit_command = 0, index = 0, nr_sockets = 0;
	int nr_ids = 0;
	int ID;
	// boolean ce imi spune daca exista deja un client
	// cu id-ul specificat
	bool client_id_exists = false;
	socklen_t clilen;

	// aloc o structura de tipul mesajului UDP
	udp_msg *received_msg = malloc(sizeof(udp_msg));
	if (received_msg == NULL) {
		return -1;
	}

	// numarul de clienti TCP conectati la server
	int nr_clients = 0;

	// aloc vectorul de clienti
	tcp_client *clienti = malloc(sizeof(tcp_client));
	if (clienti == NULL) {
		return -1;
	}
	
	fd_set read_fds;  // multimea de citire folosita in select()
	fd_set tmp_fds;	  // multime folosita temporar
	int fdmax;        // valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// golesc multimea de descriptori de citire (read_fds) 
	// si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// deschid socket TCP (are SOCK_STREAM drept type)
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	// deschid socket UDP (are SOCK_DGRAM drept type)
	sockfd2 = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sockfd2 < 0, "socket2");

	// preiau portul
	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	// setez informatiile referitoare la adresa socketului
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// asociez pentru cei doi socketi portul de pe masina locala
	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");

	ret2 = bind(sockfd2, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	DIE(ret2 < 0, "bind2");

	// asculta cererile ce provin de la clienti
	// la server se pot conecta oricati clienti, insa in coada de asteptare
	// pot fi maxim MAX_CLIENTS (5) clienti
	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// adaug descriptorii de citire
	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);
	FD_SET(sockfd2, &read_fds);
	fdmax = sockfd2;

	// pentru a putea primi id-ul de la client
	char *client_id = malloc(sizeof(char));
	if (client_id == NULL) {
		return -1;
	}

	// pentru a retine socketul fiecarui client
	char *retain_clients_socket = malloc(sizeof(char));
	if (retain_clients_socket == NULL) {
		return -1;
	}

	// pentru a retine id-ul fiecarui client
	char *retain_clients_id = malloc(sizeof(char));
	if (retain_clients_id == NULL) {
		return -1;
	}

	while (1) {
		tmp_fds = read_fds; 
		// select() pentru a controla mai multi descriptori	
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			// daca i apartine multimii descriptorilor de citire
			if (FD_ISSET(i, &tmp_fds)) {

				read_command:
				// descriptorul 0 este pentru citirea de la stdin
				if (i == 0) {
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN, stdin);
					if (strstr(buffer, "exit")) {
						memset(buffer, 0, BUFLEN);
						// trimite "exit" la clienti pentru a anunta intentia serverului 
						// de a opri conexiunea
						strncpy(buffer, "exit", 4);
						for (index = 0; index < nr_sockets; index++) {
							// daca nu sunt clienti conectati, nu am cui sa ii trimit mesaj
							if (nr_clients == 0) {
								goto exit;
							}
							// daca clientul nu e deja deconectat, ii trimit mesaj
							if (retain_clients_socket[index] != -1) {
								n = send(retain_clients_socket[index], buffer, strlen(buffer), 0);
								DIE(n < 0, "send");
							}	
						}

						exit:
						exit_command = 1;
						break;

					} else {
						//printf("Comanda primita de server este eronata.\n");
						goto read_command;
					} 
				// daca primesc mesaj de la un client UDP
				} else if (i == sockfd2) {
					memset(buffer, 0, BUFLEN);
					n = recv(sockfd2, received_msg, sizeof(buffer), 0);
					DIE(n < 0, "recv");
					// trimit mai departe mesajul la toti clientii TCP abonati
					// la topicul din mesajul primit
					for (index = 0; index < nr_sockets; index++) {
						// verific daca clientul este abonat la topic
						if (strstr(clienti[(int)retain_clients_socket[index]].subscribed_topics, 
							received_msg -> topic) != NULL) {
							n = send(retain_clients_socket[index], received_msg, sizeof(buffer), 0); 
							DIE(n < 0, "send");
						}
					}
					continue;
				}
				
				if (i == sockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					// adaug noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					memset(client_id, 0, strlen(client_id));
					// receptionez id-ul subscriber-ului nou
					n = recv(newsockfd, client_id, sizeof(client_id), 0);
					DIE(n < 0, "recv");
					// verific ca noul subscriber sa nu aiba un ID
					// deja existent; in cazul in care ID-ul deja exista,
					// trimit un mesaj de eroare
					for (index = 0; index < nr_sockets; index++) {
						if (retain_clients_id[index] == atoi(client_id)) {
							client_id_exists = true;
							ID = atoi(client_id);
							n = send(newsockfd, "error", strlen("error"), 0);
							DIE(n < 0, "send");
						}
						if (nr_sockets == 0) {
								break;
						} 
					}
					// daca id-ul clientului nu este deja conectat, 
					// adaug cu succes subscriberul
					if (!client_id_exists) {
						printf("New client %d connected from %s.\n", atoi(client_id), inet_ntoa(cli_addr.sin_addr));
						// setez id-ul si socketul subscriberului
						clienti[newsockfd].id = atoi(client_id);
						clienti[newsockfd].socket = newsockfd;
						// cresc numarul de clienti TCP
						nr_clients++;
						// retin socketul si id-ul in vectori speciali
						retain_clients_socket[nr_sockets++] = newsockfd;
						retain_clients_id[nr_ids++] = atoi(client_id);
					}

					client_id_exists = false;
					
				} else {
					// primesc un mesaj de la client
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// clientul a inchis conexiunea
						// verific din nou ca ID-ul clientului sa fie unic
						for (index = 0; index < nr_sockets; index++) {
							if (retain_clients_id[index] == ID) {
								client_id_exists = true;
								n = send(newsockfd, "error", strlen("error"), 0);
								DIE(n < 0, "send");
							}
							if (nr_sockets == 0) {
								break;
							} 
						}
						ID = 0;
						if (!client_id_exists && ID != clienti[i].id) {
							printf("Client %d disconnected.\n", clienti[i].id);

							// scad numarul de clienti TCP
							nr_clients--;
							// parcurg vectorul in care am retinut socketii clientilor TCP
							// si setez cu -1 socketul  si id-ul clientului deconectat
							for (index = 0; index < nr_sockets; index++) {
								if (retain_clients_socket[index] == clienti[i].socket) {
									retain_clients_socket[index] = -1;
								}
								if (retain_clients_id[index] == clienti[i].id) {
									retain_clients_id[index] = -1;
								}
							}
							
							close(i);
							// scot din multimea de citire socketul inchis 
							FD_CLR(i, &read_fds);
						}
						client_id_exists = false;
					} else {
						// clientul nu a inchis conexiunea, deci trebuie sa verific mesajul primit
						// pch = pointer caracter
						char *pch = strtok(buffer, " "); // comanda primita
						if (strcmp(pch, "subscribe") == 0) {
							pch = strtok(NULL, " ");
							// clientul s-a abonat la topicul pch, asa ca il adaug la subscribed_topics
							strcat(clienti[i].subscribed_topics, pch);
						}

						if (strcmp(pch, "unsubscribe") == 0) {
							pch = strtok(NULL, " ");
							// clientul s-a dezabonat de la topicul pch, asa ca il inlocuiesc
							// cu o alta valoare in subscribed_topics
							char *result = replaceWord(clienti[i].subscribed_topics, pch, "#");
							memcpy(clienti[i].subscribed_topics, result, strlen(clienti[i].subscribed_topics));
						}
					}
				}
			}
			
		}
		// daca am primit comanda "exit", ies din bucla infinita
		// pentru a putea inchide conexiunea
		if (exit_command == 1) {
			break;
		}
		
	}

	close(sockfd);
	close(sockfd2);

	return 0;
}
