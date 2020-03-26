/* Mirciu Andrei-Constantin */
/* 323CD */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <unistd.h>
#include <string.h>
#include<stdint.h>
#include<math.h> // pentru a putea folosi pow()
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h> // pentru a putea folosi TCP_NODELAY
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s id_client server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	// float folosit pentru afisarea valorilor de tipul FLOAT
	float nr; 
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	
	if (argc < 4) {
		usage(argv[0]);
	}

	// tratez cazul in care ID-ul este prea mare
	if (strlen(argv[1]) > 10) {
		fprintf(stderr, "ID-ul nu trebuie sa aiba mai mult de 10 caractere ASCII.\n");
		exit(0);
	}

	// aloc o structura de tipul mesajului UDP
	udp_msg *received_msg = malloc(sizeof(udp_msg));
	if (received_msg == NULL) {
		return -1;
	}

	// aloc payload pentru tipul de date INT
	payload_int *pi = malloc(sizeof(payload_int));
	if (pi == NULL) {
		return -1;
	}

	// aloc payload pentru tipul de date SHORT_REAL
	payload_short_real *psr = malloc(sizeof(payload_short_real));
	if (psr == NULL) {
		return -1;
	}

	// aloc payload pentru tipul de date FLOAT
	payload_float *pf = malloc(sizeof(payload_float));
	if (pf == NULL) {
		return -1;
	}

	// aloc payload pentru tipul de date STRING
	payload_string *ps = malloc(sizeof(payload_string));
	if (ps == NULL) {
		return -1;
	}

	fd_set read_fds;  // multimea de citire folosita in select()
	fd_set tmp_fds;	  // multime folosita temporar
				

	// se goleste multimea de descriptori de citire (read_fds) 
	// si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	// obtinerea descriptorului pentru socketul TCP
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");
	int flag = 1;
	// dezactivez algoritmul Nagle
	n = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));
	DIE(n < 0, "setsockopt");

	// setez informatiile referitoare la adresa socketului
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	// clientul trebuie sa se conecteze la server
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	// adaug descriptorii de citire
	FD_SET(sockfd, &read_fds); 
    FD_SET(0, &read_fds);

    // trimit mesaj serverului cu id-ul
    n = send(sockfd, argv[1], strlen(argv[1]), 0);
    DIE(n < 0, "send");
    int fdmax = sockfd; // valoare maxima fd din multimea read_fds
	while (1) {
  		tmp_fds = read_fds;
  		// select() pentru a controla mai multi descriptori
  		n = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
  		DIE(n < 0, "select");

  		// daca citesc de la stdin
  		if (FD_ISSET(0, &tmp_fds)) {
  			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			// trimit mesaj la server cu ce am primit de 
			// la stdin
			n = send(sockfd, buffer, strlen(buffer), 0);
			DIE(n < 0, "send");

			// pch = pointer caracter
			char *pch;
			char *topic;
			// daca citesc "exit", inchid conexiunea
			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}
			// preiau primul cuvand din buffer, 
			// adica comanda primita
			pch = strtok(buffer, " ");
			
			if (strcmp(pch, "subscribe") == 0) {
				topic = strtok(NULL, " ");
				printf("subscribed %s\n", topic);
			} 

			if (strcmp(pch, "unsubscribe") == 0) {
				topic = strtok(NULL, " ");
				printf("unsubscribed %s\n", topic);
			}
  		}

  		// daca primesc mesaj de la server
  		if (FD_ISSET(sockfd, &tmp_fds)) {
  			memset(buffer, 0, BUFLEN);
  			n = recv(sockfd, buffer, BUFLEN, 0);
  			DIE(n < 0, "recv");
  			// daca primesc "exit", inchid conexiunea
  			if (strstr(buffer, "exit")) {
				break;
			}
			// daca primesc "error", inchid conexiunea (tratez
			// cazul cand doi clienti cu acelasi ID vor sa se
			// conecteze la server)
			if (strstr(buffer, "error")) {
				fprintf(stderr, "Exista deja un client cu ID-ul %d.\n", atoi(argv[1]));
				exit(0);
			}

			memset(received_msg, 0, BUFLEN);
			// pun continutul bufferului in structura de mesaj UDP
			memcpy(received_msg, buffer, BUFLEN);
			// parsez continutul primit in functie de tipul de date
			switch(received_msg -> tip_date) {
				case 0: memset(pi, 0, BUFLEN);
						// pun continutul mesajului UDP in payload-ul specific
						memcpy(pi, received_msg -> continut, BUFLEN);
						// daca numarul este pozitiv
						if (pi -> sign == 0) {
							printf("%s:%d - %s - INT - %d\n", inet_ntoa(serv_addr.sin_addr), 
								atoi(argv[3]), received_msg -> topic, pi -> content);
							break;
						} else { // numarul este negativ
							printf("%s:%d - %s - INT - %d\n", inet_ntoa(serv_addr.sin_addr), 
								atoi(argv[3]), received_msg -> topic, (pi -> content) * (-1));
							break;
						}

				case 1: memset(psr, 0, BUFLEN);
						// pun continutul mesajului UDP in payload-ul specific
						memcpy(psr, received_msg -> continut, BUFLEN);
						printf("%s:%d - %s - SHORT_REAL - %.2f\n", inet_ntoa(serv_addr.sin_addr), 
							atoi(argv[3]), received_msg -> topic, (float)ntohs(psr -> content) / 100);
						break;

				case 2: memset(pf, 0, BUFLEN);
						// pun continutul mesajului UDP in payload-ul specific
						memcpy(pf, received_msg -> continut, BUFLEN);
						// daca numarul este pozitiv
						if (pf -> sign == 0) {
							// calculez efectiv numarul pe baza informatiilor primite
							nr = pf -> number / pow(10, pf -> power);
							printf("%s:%d - %s - FLOAT - %g\n", inet_ntoa(serv_addr.sin_addr), 
								atoi(argv[3]), received_msg -> topic, nr);
							break;
						} else { // numarul este negativ
							nr = pf -> number / pow(10, pf -> power);
							printf("%s:%d - %s - FLOAT - %g\n", inet_ntoa(serv_addr.sin_addr), 
								atoi(argv[3]), received_msg -> topic, nr * (-1));
							break;
						}
						
				case 3: memset(ps, 0, BUFLEN);
						// pun continutul mesajului UDP in payload-ul specific
						memcpy(ps, received_msg -> continut, BUFLEN);
						printf("%s:%d - %s - STRING - %s\n", inet_ntoa(serv_addr.sin_addr), 
							atoi(argv[3]), received_msg -> topic, ps -> content);
						break;

				default: break;
				
			}
			
  		}
		
	}

	close(sockfd);

	return 0;
}
