#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>

int socketFd = 0;

void closeSocket()
{
    printf("Closing socket.\n");
    shutdown(socketFd, SHUT_RDWR);
    close(socketFd);
    socketFd = 0;
}

void exitFunction()
{
    // do stuff to make exit graceful

    // closing bs socket if open
    if (socketFd != 0)
    {
        closeSocket();
    }

    printf("****************SMTP Client Closed*****************\n");
}

void signalHandler(int sig)
{
    switch (sig)
    {
    case SIGINT:
        printf("Received SIGINT signal.\n");
        exit(EXIT_SUCCESS);
        break;
    case SIGILL:
        printf("Received SIGILL signal.\n");
        break;
    case SIGABRT:
        printf("Received SIGABRT signal.\n");
        exit(EXIT_SUCCESS);
        return;
    case SIGFPE:
        printf("Received SIGFPE signal.\n");
        break;
    case SIGSEGV:
        printf("Received SIGSEGV signal.\n");
        break;
    case SIGTERM:
        printf("Received SIGTERM signal.\n");
        break;

    default:
        printf("Received unknown signal.\n");
        break;
    }

    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    // connect function to handle exits
    atexit(exitFunction);

    // close sockets on segmentation fault
    signal(SIGSEGV, signalHandler);

    // close sockets on abort signal
    signal(SIGABRT, signalHandler);

    // close sockets on interactive attention
    signal(SIGINT, signalHandler);

    printf("********************SMTP Client********************\n");
}