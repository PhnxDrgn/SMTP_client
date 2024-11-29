#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdbool.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#define SMTP_PORT 25
#define SMTP_SSL_PORT 465
#define GMAIL_SMTP_NAME "smtp.gmail.com"
#define MAX_LINE_LEN 100
#define MAX_USER_LEN 50
#define MAX_PASS_LEN 50
#define BUFFER_SIZE 1024

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

/**
 * Function to send command to server
 * socket: socket descriptor for server
 * buffer: data to send to server
 * hideCommand: flag to hide print out of failed command
 * return: 0 for success or -1 for failure
 */
int sendCommand(SSL *ssl, char *cmd, bool hideCommand, int expectedResponse)
{
    SSL_write(ssl, cmd, strlen(cmd));

    // read response
    char buffer[BUFFER_SIZE];
    int len = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    buffer[len] = '\0';
    if (len <= 0)
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
        ERR_print_errors_fp(stdout);
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

    // Initialize OpenSSL
    printf("Initializing OpenSSL\n");
    SSL_CTX *ctx;
    SSL *ssl;
    BIO *bio;
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    const SSL_METHOD *method = SSLv23_client_method();
    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        ERR_print_errors_fp(stdout);
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

    // get host ip
    struct hostent *host = gethostbyname(GMAIL_SMTP_NAME);

    if (host == NULL)
    {
        printf("Failed to get host from %s\n", GMAIL_SMTP_NAME);
        exit(EXIT_FAILURE);
    }

    // creating server address struct
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SMTP_SSL_PORT);
    memcpy(&serverAddress.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

    // connect to server
    if (connect(socketFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        printf("Failed to connect to server.\n");
        exit(EXIT_FAILURE);
    }
    printf("Successfully connected to server\n");

    // Create SSL object
    printf("Creating SSL object\n");
    bio = BIO_new_socket(socketFd, BIO_NOCLOSE);
    ssl = SSL_new(ctx);
    SSL_set_bio(ssl, bio, bio);

    // Perform the handshake
    if (SSL_connect(ssl) <= 0)
    {
        ERR_print_errors_fp(stdout);
        exit(EXIT_FAILURE);
    }

    // read server response. Expecting 220 for server ready
    if (sendCommand(ssl, "", false, 220) < 0)
        exit(EXIT_FAILURE);

    // send EHLO to introduce and set domain. Expecting 250 for auth request accepted
    if (sendCommand(ssl, "EHLO umd.edu\r\n", false, 250) < 0)
        exit(EXIT_FAILURE);

    // request auth login. Expecting 334 for auth request accepted
    if (sendCommand(ssl, "AUTH LOGIN\r\n", false, 334) < 0)
        exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);
}