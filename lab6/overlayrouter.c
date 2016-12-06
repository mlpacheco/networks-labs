#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include "hashtable.h"

#define MAX_BUFF 2000

volatile sig_atomic_t erase_entry = 0;

// signal to deal with concurrency
void child_sig_handler(int signum) {
    int status;
    wait(&status);
}

void alarm_sig_handler(int signum) {
    erase_entry = 1;
}

int confirm_entry(char * router_ip, char * router_data_port) {
    char key[MAX_BUFF + 1];
    struct Item* item1;
    struct Item* item2;

    snprintf(key, sizeof(key), "(%s,%s)", router_ip, router_data_port);
    item1 = search(key);
    item1->flag = 1;
    item2 = search(item1->data);
    if (strcmp(item2->data, key) == 0) {
    item2->flag = 1;
    }

    printf("Table after confirmation\n");
    display();
}

int add_entry(char * src_ip, int src_port, char * dest_ip, int dest_port) {
    char key[MAX_BUFF + 1];
    char value[MAX_BUFF + 1];

    snprintf(key, sizeof(key), "(%s,%d)", src_ip, src_port);
    snprintf(value, sizeof(value), "(%s,%d)", dest_ip, dest_port);
    insert(key, value, 0);
    insert(value, key, 0);

    printf("Routing table update\n");
    display();

}

int confirm_hop(char * src_ip, int port, char * localaddr, int my_port) {
    char message[MAX_BUFF + 1];
    snprintf(message, sizeof(message), "$%s$%d$", localaddr, my_port);
    printf("I am entering the function to send confirm hop\n");
    int sd_back_hop, n_bytes;
    struct sockaddr_in back_hop;
    struct in_addr host_addr;
    struct hostent *ipv4address;
    char buffer[MAX_BUFF + 1];

    inet_pton(AF_INET, src_ip, &host_addr);
    ipv4address = gethostbyaddr(&host_addr, sizeof host_addr, AF_INET);
    if (!ipv4address) {
        perror("unknown address");
        return -1;
    }

    socklen_t addrsize = sizeof(back_hop);

    // set hop info
    memset(&back_hop, 0, sizeof(back_hop));
    back_hop.sin_family = AF_INET;
    bcopy(ipv4address->h_addr, &(back_hop.sin_addr.s_addr), ipv4address->h_length);
    back_hop.sin_port = htons(port);

    if ((sd_back_hop = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    n_bytes = sendto(sd_back_hop, message, strlen(message), 0, (struct sockaddr *) &back_hop,
                     sizeof(back_hop));

    close(sd_back_hop);
}

int make_hop(char * dest_ip, int port, char * message, int my_port) {
    int sd_hop, n_bytes, dest_port;
    struct sockaddr_in addr_hop, addr_my;
    struct in_addr host_addr;
    struct hostent *ipv4address;
    char buffer[MAX_BUFF + 1];

    inet_pton(AF_INET, dest_ip, &host_addr);
    ipv4address = gethostbyaddr(&host_addr, sizeof host_addr, AF_INET);
    if (!ipv4address) {
        perror("unknown address");
        return -1;
    }

    socklen_t addrsize = sizeof(addr_hop);

    // set hop info
    memset(&addr_hop, 0, sizeof(addr_hop));
    addr_hop.sin_family = AF_INET;
    bcopy(ipv4address->h_addr, &(addr_hop.sin_addr.s_addr), ipv4address->h_length);
    addr_hop.sin_port = htons(port);

    // set my info
    memset(&addr_my, 0, sizeof(addr_my));
    addr_my.sin_family = AF_INET;
    addr_my.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_my.sin_port = htons(my_port);

    if ((sd_hop = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    if (bind(sd_hop, (struct sockaddr *)&addr_my, sizeof(addr_my)) < 0) {
        perror("bind error");
        return -1;
    }

    n_bytes = sendto(sd_hop, message, strlen(message), 0, (struct sockaddr *) &addr_hop,
                     sizeof(addr_hop));

    printf("Sent %s\n", message);
    n_bytes = recvfrom(sd_hop, buffer, sizeof(buffer), 0,
                       (struct sockaddr *) &addr_hop, &addrsize);

    // add entry to database
    if (n_bytes > 0) {
        buffer[n_bytes] = '\0';
        printf("Received ACK: %s\n", buffer);
        dest_port = atoi(buffer);
        return dest_port;
    }

    close(sd_hop);
    return -1;

}

int count_separators(char * buffer) {
    int count = 0;
    int i;
    for(i = 0; i < strlen(buffer); i++) {
        if(buffer[i] == '$') {
            count++;
        }
    }
    return count;
}

int main(int argc, char *argv[]) {

    // check that we have all needed params
    if (argc != 2) {
        printf("Run: %s [server-port]\n", argv[0]);
        return -1;
    }

    pid_t k;
    int sd_server, port_server, n_bytes, count;
    struct sockaddr_in addr_server;
    char buffer[MAX_BUFF + 1];
    char message[MAX_BUFF + 1];
    char key[MAX_BUFF + 1];
    char last_router[MAX_BUFF + 1];
    char src_addr[MAX_BUFF + 1];
    char * router_ip;
    char * ptr;
    char * router_data_port;
    char * localaddr;
    char iface[] = "eth0";
    struct ifreq ifr;
    int beg_ip, src_port, my_port;

    socklen_t addrsize = sizeof(addr_server);
    port_server = atoi(argv[1]);

    // stuff to handle concurrency signals
    //struct sigaction sigchld_action;
    //memset (&sigchld_action, 0, sizeof (sigchld_action));
    //sigchld_action.sa_handler = child_sig_handler;
    //sigaction (SIGCHLD, &sigchld_action, NULL);

    // set info
    memset(&addr_server, 0, sizeof(addrsize));
    addr_server.sin_family = AF_INET;
    addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_server.sin_port = htons(port_server);

    // create socket
    if ((sd_server = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket error");
        return -1;
    }

    // bind information to the socket
    if ((bind(sd_server, (struct sockaddr *)&addr_server,
              sizeof(addr_server))) < 0) {
        perror("bind error");
        return -1;
    }

    int num_clients = 0;
    while(1) {
        // receive client request through tunnel request
        printf("I am listening...\n");
        n_bytes = recvfrom(sd_server, buffer, sizeof(buffer), 0,
                          (struct sockaddr *) &addr_server, &addrsize);

        printf("Received %d bytes\n", n_bytes);
        // fork a process to deal with this client
        // this is implemented to allow multiple clients to use this router
        if (n_bytes <= 0) {
            continue;
        }

        count = 0;
        buffer[n_bytes] = '\0';
        printf("%s\n", buffer);

        // count separators to see what kind of message I am receiving
        // >= 4 separators -> routing build packet
        // other -> unknown
        count = count_separators(buffer);

        // building path
        if (count < 4) {
            continue;
        }

        memcpy(src_addr, &inet_ntoa(addr_server.sin_addr)[0],
        strlen(inet_ntoa(addr_server.sin_addr)));
        src_addr[strlen(inet_ntoa(addr_server.sin_addr))] = '\0';
        src_port = ntohs(addr_server.sin_port);

        // get local address
        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);
        ioctl(sd_server, SIOCGIFADDR, &ifr);
        localaddr = inet_ntoa(( (struct sockaddr_in *)&ifr.ifr_addr )->sin_addr);

        printf("Received data from address %s through port %d\n", src_addr, src_port);
        printf("local address: %s\n", localaddr);

        buffer[n_bytes - 1] = '\0';
        printf("%s\n", buffer);

        ptr = strrchr(buffer, '$');
        beg_ip = ptr - buffer + 1;
        printf("beg_ip: %d\n", beg_ip);

        memcpy(last_router, &buffer[beg_ip], strlen(buffer) - beg_ip);
        last_router[strlen(buffer) - beg_ip] = '\0';
        printf("last_router=%s\n", last_router);
        printf("localaddr=%s\n", localaddr);
        if (strcmp(last_router, localaddr) != 0) {
            continue;
        }


        // first I need to return the ACK
        num_clients++;
        my_port = port_server + num_clients;
        snprintf(message, sizeof(message), "%d", my_port);
        sendto(sd_server, message, strlen(message), 0,
               (struct sockaddr *)&addr_server, sizeof(addr_server));

        printf("ACK: %s\n", message);

        // now I need to forward to the next in line
        memcpy(message, &buffer[0], strlen(buffer) - strlen(last_router));
        message[strlen(buffer) - strlen(last_router)] = '\0';
        printf("message: %s, length: %lu\n", message, strlen(message));

        if (count == 4) {
            // i am the last router
            printf("I am the last router\n");

            char * dest_ip = strtok(message, "$");
            char * dest_port = strtok(NULL, "$");
            confirm_hop(src_addr, port_server, localaddr, my_port);
            // add entry to routing table
            add_entry(src_addr, src_port,  dest_ip, atoi(dest_port));
        } else {
            printf("I am an intermediate router\n");
            memcpy(buffer, &message[0], strlen(message));
            buffer[strlen(message) - 1] = '\0';
            ptr = strrchr(buffer, '$');
            beg_ip = ptr - buffer + 1;
            printf("beg_ip: %d\n", beg_ip);

            memcpy(last_router, &buffer[beg_ip], strlen(buffer) - beg_ip);
            last_router[strlen(buffer) - beg_ip] = '\0';
            printf("next_router=%s\n", last_router);

            int dest_port = make_hop(last_router, port_server, message, my_port);

            printf("dest_port: %d\n", dest_port);

            // didn't get a valid ACK
            if (dest_port == -1) {
                continue;
            }

            // add entry to routing table
            add_entry(src_addr, src_port, last_router, dest_port);

            // get confirmation message
            // set alarm to delete router table entry if not confirmed
            //alarm(30);
            n_bytes = recvfrom(sd_server, buffer, sizeof(buffer), 0,
                               (struct sockaddr *) &addr_server, &addrsize);

            if (n_bytes < 0) {
                continue;
            }
            buffer[n_bytes] = '\0';
            count = count_separators(buffer);

            if (count != 3) {
                continue;
            }

            // disable alarm once we have gotten the confirmation
            //alarm(0);

            printf("I am receiving a hop back message\n");

            router_ip = strtok(buffer, "$");
            router_data_port = strtok(NULL, "$");

            confirm_entry(router_ip, router_data_port);

            // confirm back
            confirm_hop(src_addr, port_server, localaddr, my_port);
        }

     }

}
