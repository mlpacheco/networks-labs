#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "hashtable.h"

struct Item * hashTable[SIZE];

unsigned long hashCode(unsigned char * key) {
    unsigned long hash = 5381;
    int c;
    while (c = *key++) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

struct Item *search(unsigned char * key) {
    //get the hash
    unsigned long code = hashCode(key);
    int hashIndex = code % SIZE;

    //move in array until an empty
    while(hashTable[hashIndex] != NULL) {
        if(strcmp(hashTable[hashIndex]->key, key) == 0) {
            return hashTable[hashIndex];
        }

        //go to next cell
        ++hashIndex;

        //wrap around the table
        hashIndex %= SIZE;
    }

    return NULL;
}

void insert(char * key, char * data, int flag) {
    struct Item *item = (struct Item*)malloc(sizeof(item));

    strcpy(item->data, data);
    strcpy(item->key, key);
    item->flag = flag;

    //get the hash
    unsigned long code = hashCode(key);
    int hashIndex = code % SIZE;

    //move in array until an empty or deleted cell
    while(hashTable[hashIndex] != NULL) {
        //go to next cell
        ++hashIndex;

        //wrap around the table
        hashIndex %= SIZE;
    }

    hashTable[hashIndex] = item;
}

void display() {
    int i = 0;
    //printf("Routing table\n");
    printf("[src, dest, flag]\n");
    for(i = 0; i<SIZE; i++) {
        if(hashTable[i] != NULL) {
            printf("[%s, %s, %d]\n", hashTable[i]->key, hashTable[i]->data,
                   hashTable[i]->flag);
        } /*else {
            printf(" ~~ ");
        }*/
    }
    printf("\n");
}

/*int main() {
    struct Item* item;

    insert("(src-IP,src-port)", "(router2-IP,data-port-2)");
    insert("(router2-IP,data-port-2)", "(src-IP,src-port)");
    insert("(router1-IP,data-port-1)", "(router3-IP,data-port-3)");
    insert("(router3-IP,data-port-3)", "(router1-IP,data-port-1)");

    display();
    item = search("(router3-IP,data-port-3)");
    if(item != NULL) {
        printf("Element found: %s\n", item->data);
    } else {
        printf("Element not found\n");
    }

    item = search("(router4-IP,data-port-4)");
    if(item != NULL) {
        printf("Element found: %s\n", item->data);
    } else {
        printf("Element not found\n");
    }
}*/


