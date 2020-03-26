/* Mirciu Andrei-Constantin */
/* 323CD */
#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

#define BUFLEN      2000    // dimensiunea maxima a bufferului de date
#define MAX_UDP_CONTENT 1500
#define MAX_CLIENTS 5   // numarul maxim de clienti in asteptare

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)    \
    do {                                    \
        if (assertion) {                    \
            fprintf(stderr, "(%s, %d): ",   \
                    __FILE__, __LINE__);    \
            perror(call_description);       \
            exit(EXIT_FAILURE);             \
        }                                   \
    } while(0)

typedef struct {
    char topic[50];
    uint8_t tip_date;
    char continut[MAX_UDP_CONTENT]; 
} udp_msg;

typedef struct {
    int id;
    int socket;
    char subscribed_topics[BUFLEN];
} tcp_client;

typedef struct {
    char sign;
    uint32_t content;
} payload_int;

typedef struct {
    uint16_t content;
} payload_short_real;

typedef struct {
    char sign;
    uint32_t number;
    uint8_t power;
} payload_float;

typedef struct {
    char content[MAX_UDP_CONTENT];
} payload_string;

//https://www.geeksforgeeks.org/c-program-replace-word-text-another-given-word/
char *replaceWord(const char *subscribed_topics, const char *oldTopic, 
                                 const char *newTopic) { 
    char *result; 
    int i, cnt = 0; 
    int newTopicLen = strlen(newTopic); 
    int oldTopicLen = strlen(oldTopic); 
  
    // Counting the number of times old topic 
    // occurs in the subscribed topics
    for (i = 0; subscribed_topics[i] != '\0'; i++) 
    { 
        if (strstr(&subscribed_topics[i], oldTopic) == &subscribed_topics[i]) 
        { 
            cnt++; 
  
            // Jumping to index after the old topic. 
            i += oldTopicLen - 1; 
        } 
    } 
  
    // Making new string of enough length 
    result = (char *)malloc(i + cnt * (newTopicLen - oldTopicLen) + 1); 
  
    i = 0; 
    while (*subscribed_topics) 
    { 
        // compare the substring with the result 
        if (strstr(subscribed_topics, oldTopic) == subscribed_topics) 
        { 
            strcpy(&result[i], newTopic); 
            i += newTopicLen; 
            subscribed_topics += oldTopicLen; 
        } 
        else
            result[i++] = *subscribed_topics++; 
    } 
  
    result[i] = '\0'; 
    return result; 
}

#endif
