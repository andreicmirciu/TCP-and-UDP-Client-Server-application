Mirciu Andrei-Constantin 323CD

	In implementarea temei am realizat modelul de aplicatie client-server
necesar pentru transmiterea mesajelor. In cazul de fata, clientii pot
fi atat de tipul TCP, cat si UDP, serverul avand rolul de a primi
mesaje de la clienti UDP si de a le transmite mai departe
clientilor TCP abonati la topicurile respective.

	De asemenea, pentru ca aplicatia sa poata functiona in mod corespunzator,
am construit mai multe structuri in fisierul "helpers.h": "udp_msg" 
reprezinta tipul mesajului trimis de clientul UDP serverului, "tcp_client" 
contine informatii despre un client TCP, respectiv am realizat cate o structura
de tip payload pentru fiecare tip de date (int, short_real, float, string).

	server.c: Pentru a putea comunica cu cele doua tipuri de clienti,
am deschis atat un socket TCP (sockfd), cat si unul UDP (sockfd2).
Pentru a putea conecta la server oricati subscriberi, ma folosesc de
conceptul de multiplexare. Astfel, adaug la multimea de descriptori de
citire, folositi de apelul select(), socketul TCP, socketul UDP, respectiv
0 (pentru citirea de la stdin). In continuare, intr-o structura repetitiva,
pentru fiecare numar natural pana la descriptorul cu cea mai mare valoare,
verific daca acesta apartine multimii descriptorilor de citire, situatie
in care tratez mai multe cazuri (daca numarul este 0, sockfd sau sockfd2).

	Daca descriptorul este 0, preiau intr-un buffer comanda primita de la
stdin. In cazul in care primesc "exit", trimit un mesaj fiecarui client
TCP conectat pentru a anunta ca urmeaza sa se inchida conexiunea. Pentru
a nu trimite mesaje clientilor care s-au deconectat deja, setez socketul
acestora cu -1. Avand in vedere ca orice alta comanda primita de server de 
la tastatura este eronata, afisez un mesaj de eroare si astept sa introduc 
din nou alta comanda (ma folosesc de goto).

	In caz contrar, daca descriptorul este cel de la socketul UDP, inseamna 
ca trebuie sa primesc mesaj de la clientul UDP. In continuare, pentru fiecare 
client TCP, daca acesta este abonat la topicul primit, trimit mesajul pe 
socketul corespunzator (fiecare client TCP are propriul socket).

	Daca descriptorul este cel de la socketul TCP, inseamna ca un subscriber
nou s-a conectat la server. Pentru a tine evidenta clientilor, am alocat
dinamic un vector "clienti", in care retin, pentru fiecare client in parte,
datele necesare. Altfel, inseamna ca serverul primeste de la clientul TCP
un mesaj. Daca apelul de recv() intoarce 0, clientul respectiv a inchis
conexiunea, fapt afisat de server. In caz contrar, verific tipul mesajului
primit (daca e subscribe/unsubscribe). Cum serverul trebuie sa tina evidenta
topicurilor la care este abonat fiecare client, pentru subscribe adaug cu
strcat noul topic la restul, iar pentru unsubscribe, modific titlul respectiv
din vectorul de topicuri cu caracterul "#" (fac in acest fel deoarece nu pot
sterge efectiv topicul, asa ca am ales sa il inlocuiesc cu un caracter 
invalid).

	subscriber.c: Pentru a putea comunica cu serverul, deschid un socket TCP.
De asemenea, dezactivez algoritmul Nagle cu ajutorul functiei setsockopt pe
acest socket. In mod asemanator cu serverul, adaug la descriptorii de citire
0 si sockfd, verificand in continuare in functie de fiecare.

	Daca descriptorul este 0, verific daca comanda primita de la stdin este 
una dintre exit/subscribe/unsubscribe si ii trimit serverului (pentru a putea
tine evidenta).

	Daca descriptorul este cel de la socketul TCP, inseamna ca a fost primit
un mesaj de la server. Dupa ce primesc mesajul in buffer, il adaug intr-o 
structura de tipul mesajului UDP, urmand sa parsez continutul in functie
de tipul de date si respectiv sa afisez la stdout ce am primit.

	Nu in ultimul rand, atat in server, cat si in subscriber, actionez in
mod corespunzator pentru cazul in care doi clienti vor sa se conecteze
folosind acelasi id (inchid conexiunea pentru clientul cu id duplicat).

P.S: Mentionez ca m-am folosit de scheletul pus la dispozitie pentru
laboratorul 8. De asemenea, functia char *replaceWord(const char *s, 
const char *oldW, const char *newW) din header-ul "helpers.h" am preluat-o 
de pe urmatorul site:
https://www.geeksforgeeks.org/c-program-replace-word-text-another-given-word/
	Functionalitatea temei am testat-o folosind Windows 
Subsystem for Linux. Pentru a rula tema pe un anumit port si adresa IP,
trebuie modificati parametrii corespunzatori din fisierul Makefile.
Totodata, comanda make run_subscriber poate fi folosita doar daca se 
defineste in prealabil in Makefile si parametrul ${ID_CLIENT} (in
general am folosit doar make run_server, iar subscriberii i-am conectat
manual, fara Makefile).
	Precizez ca nu am implementat componenta de store&foreword si faptul
ca am probleme (uneori) cu afisarea valorilor primite.

 
