#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define SMTP_PORT 25
#define GMAIL_SMTP_IP "smtp.gmail.com"
#define MAX_LINE_LEN 100
#define MAX_USER_LEN 50
#define MAX_PASS_LEN 50

int socketFd = 0;
char user[MAX_USER_LEN] = "\0";
char pass[MAX_PASS_LEN] = "\0";

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
 * Function to remove leading and trailing spaces from a string
 * str: pointer to char buffer with string to be trimmed
 */
void trim(char *str)
{
    int start = 0, end = strlen(str) - 1;

    // remove leading spaces
    while (str[start] == ' ')
    {
        start++;
    }

    // remove trailing spaces
    while (end >= start && (str[end] == ' ' || str[end] == '\n' || str[end] == '\r'))
    {
        end--;
    }

    // calculate string len
    int len = end - start + 1;

    // shift the string to the left to remove leading spaces
    for (int ii = 0; ii < len; ii++)
    {
        str[ii] = str[start + ii];
    }

    // null-terminate the string
    str[len] = '\0';
}

/**
 * Function to read in the config file
 * filePath: pointer to string name for file path
 * return: 0 if successful, -1 if failed
 */
int readCredentials(char *filePath)
{
    char line[MAX_LINE_LEN] = "\0";
    FILE *file = fopen(filePath, "r");

    if (file == NULL)
    {
        printf("Failed to read config file %s", filePath);
        return -1;
    }

    // reset user and pass vars
    user[0] = '\0';
    pass[0] = '\0';

    // read configs line by line
    while (fgets(line, MAX_LINE_LEN, file))
    {
        char *key;
        char *value;

        // trim leading an trailing spaces
        trim(line);

        // ignore comments
        if (line[0] == '#')
            continue;

        // valid key values will have : between. Find splitter
        char *splitPtr = strchr(line, ':');

        if (splitPtr != NULL)
        {
            // null terminate at the split
            *splitPtr = '\0';

            // set key and value
            key = line;
            value = splitPtr + 1;

            // trim key and value
            trim(key);
            trim(value);

            // check for user and pass
            if (strcmp(key, "user") == 0)
            {
                strncpy(user, value, sizeof(user));
            }
            else if (strcmp(key, "pass") == 0)
            {
                strncpy(pass, value, sizeof(pass));
            }
        }
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

    // read in credentials
    printf("Reading credentials.\n");
    if (readCredentials("credentials.config") != 0)
    {
        exit(EXIT_FAILURE);
    }

    // check if user and pass are read
    if (strlen(user) == 0)
    {
        printf("Failed to read user.\n");
        exit(EXIT_FAILURE);
    }
    if (strlen(pass) == 0)
    {
        printf("Failed to read pass.\n");
        exit(EXIT_FAILURE);
    }

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

    // get binary network format from server ip string
    if (inet_pton(AF_INET, GMAIL_SMTP_IP, &serverAddress.sin_addr) <= 0)
    {
        printf("Server address is invalid.\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}