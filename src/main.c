#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdbool.h>

#define SMTP_PORT 25
#define SMTP_SERVER_IP "192.168.0.3"
#define BUFFER_SIZE 1024

int socketFd = 0;

/**
 * Function to close the socket
 */
void closeSocket()
{
    printf("Closing socket.\n");
    shutdown(socketFd, SHUT_RDWR);
    close(socketFd);
    socketFd = 0;
}

/**
 * Function to close app gracefully
 */
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

/**
 * Function to report signal
 * sig: signal flag received
 */
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

/**
 * Function to send command to server
 * socket: socket descriptor for server
 * buffer: data to send to server
 * hideCommand: flag to hide print out of failed command
 * return: 0 for success or -1 for failure
 */
int sendCommand(int socket, char *cmd, bool hideCommand, int expectedResponse)
{
    if (!hideCommand)
    {
        printf("Sending command: %s\n", cmd);
    }

    if (write(socket, cmd, strlen(cmd)) < 0)
    {
        printf("Failed to send command.\n");
        return -1;
    }

    // read response
    char buffer[BUFFER_SIZE];
    int len = read(socket, buffer, sizeof(buffer) - 1);
    buffer[len] = '\0';
    if (len < 0)
    {
        printf("Failed to read reply to command.\n");
        return -1;
    }

    printf("Server response: %s\n", buffer);

    char responseCpy[4];
    strncpy(responseCpy, buffer, sizeof(responseCpy) - 1);
    responseCpy[sizeof(responseCpy) - 1] = '\0';
    int responseCode = atoi(responseCpy);

    if (responseCode != expectedResponse)
    {
        printf("Response does not match expected response of %d.\n", expectedResponse);
        return -1;
    }

    return 0;
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

    // creating socket
    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0)
    {
        printf("Failed to create socket.\n");
        exit(EXIT_FAILURE);
    }
    printf("Socket created successfully.\n");

    // creating server address struct
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SMTP_PORT);

    // Convert IP address from text to binary
    if (inet_pton(AF_INET, SMTP_SERVER_IP, &serverAddress.sin_addr) <= 0)
    {
        printf("%s is an invalid address\n", SMTP_SERVER_IP);
        exit(EXIT_FAILURE);
    }

    // connect to server
    if (connect(socketFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        printf("Failed to connect to server.\n");
        exit(EXIT_FAILURE);
    }
    printf("Successfully connected to server\n");

    // read server response. Expecting 220 for server ready
    if (sendCommand(socketFd, "", false, 220) < 0)
        exit(EXIT_FAILURE);

    // send EHLO to introduce and set domain. Expecting 250 command completed
    if (sendCommand(socketFd, "EHLO umd.edu\r\n", false, 250) < 0)
        exit(EXIT_FAILURE);

    // send QUIT to gracefully close connection. Expecting 221 close connection confirm
    if (sendCommand(socketFd, "QUIT\r\n", false, 221) < 0)
        exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);
}