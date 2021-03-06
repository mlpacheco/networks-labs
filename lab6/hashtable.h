#ifndef HASH_H_
#define HASH_H_

#define SIZE 50

struct Item {
    char data[500];
    char key[500];
    int flag;
};

extern struct Item * hashTable[SIZE];

unsigned long hashCode(unsigned char * key);

struct Item * search(unsigned char * key);

void insert(char * key, char * data, int flag);

void display();

#endif
