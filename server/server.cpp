#ifndef BUF_SIZE
#define BUF_SIZE 10000
#endif

#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <arpa/inet.h>	/* for inet_ntoa */
#include "../task/task.hpp"
#include "../serialization/serialization.hpp"

#include <sys/mman.h>


using namespace boost::archive;
using namespace std;

//static long client_number = 0;
static long *client_number;


void do_work(int rqst) {
    char buffer[BUF_SIZE];
    // Receiving data
    ssize_t n = recv(rqst, buffer, BUF_SIZE, 0);
    if (n == -1) {
        perror("ERROR reading from socket");
        exit(1);
    }

    deserialize(buffer);

    // Sending data
    strcpy(buffer, palindromes(buffer).c_str());
    serialize(buffer);

    n = send(rqst, buffer, BUF_SIZE, 0);
    if (n == -1) {
        perror("ERROR writing to socket");
        exit(1);
    }

    shutdown(rqst, 2);    /* close the connection */
    cout << *client_number << ". Work with this client is finished" << endl;
}

void *thread_function(void *sock) {
    do_work(*(int *) sock);
    return NULL;
}


int main(int argc, char **argv) {
    if (argc != 2) {
        perror("Must be one parameter - server type: fork, pthread or asio");
        exit(1);
    }

    client_number = (long *) mmap(NULL, sizeof *client_number, PROT_READ | PROT_WRITE,
                                  MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    uint16_t port = 3000;    /* port number */
    int rqst;       /* socket accepting the request */
    socklen_t alen;       /* length of address structure */
    struct sockaddr_in my_addr;    /* address of this service */
    struct sockaddr_in client_addr;  /* client's address */
    int sockoptval = 1;


    /* create a TCP/IP socket */
    int svc;
    if ((svc = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("cannot create socket");
        exit(1);
    }

    /* allow immediate reuse of the port */
    setsockopt(svc, SOL_SOCKET, SO_REUSEADDR, &sockoptval, sizeof(int));

    /* bind the socket to our source address */
    memset((char *) &my_addr, 0, sizeof(my_addr));  /* 0 out the structure */
    my_addr.sin_family = AF_INET;   /* address family */
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(svc, (struct sockaddr *) &my_addr, sizeof(my_addr)) < 0) {
        perror("bind failed");
        exit(1);
    }

    /* set the socket for listening (queue backlog of 5) */
    if (listen(svc, 5) < 0) {
        perror("listen failed");
        exit(1);
    }

    while (true) {
        while ((rqst = accept(svc,
                              (struct sockaddr *) &client_addr, &alen)) < 0) {
            /* we may break out of accept if the system call */
            /* was interrupted. In this case, loop back and */
            /* try again */
            if ((errno != ECHILD) && (errno != ERESTART) && (errno != EINTR)) {
                perror("accept failed");
                exit(1);
            }
        }
        *client_number = *client_number + 1;

        printf("received a connection from: %s port %d\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        if (!strcmp(argv[1], "fork")) {
            if (!fork())
                do_work(rqst);
        } else if (!strcmp(argv[1], "pthread")) {
            pthread_t thread;
            pthread_create(&thread, NULL, &thread_function, (void *) &rqst);
            pthread_join(thread, NULL);
        }
    }
}
