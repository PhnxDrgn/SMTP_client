#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdbool.h>
#include <termio.h>
#include <base64.h>

#define SMTP_PORT 25
#define SMTP_SERVER_IP "192.168.0.3"
#define BUFFER_SIZE (1024 * 512) // 512 kB of buffer space

typedef struct
{
    char from[256];
    char to[256];
    char subject[256];
    char body[BUFFER_SIZE];
    char attachmentName[256];
    char encodedAttachment[BUFFER_SIZE];
} mailData_t;

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
 * expectedResponse: int value expected. If 0, no response expected.
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

    if (expectedResponse > 0)
    {
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
    }

    return 0;
}

void initMailData(mailData_t *data)
{
    data->from[0] = '\0';
    data->to[0] = '\0';
    data->subject[0] = '\0';
    memset(data->body, '\0', sizeof(data->body));
    memset(data->attachmentName, '\0', sizeof(data->attachmentName));
    memset(data->encodedAttachment, '\0', sizeof(data->encodedAttachment));
}

void printMailData(mailData_t data)
{
    printf("\nFrom: %s\n", data.from);
    printf("To: %s\n", data.to);
    printf("Subject: %s\n", data.subject);
    if (strlen(data.attachmentName) > 0)
    {
        printf("Attachment: %s\n", data.attachmentName);
    }
    if (strlen(data.body) > 0)
        printf("\n%s\n", data.body);
    printf("\n");
}

void clearInputBuffer()
{
    char c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

mailData_t getMailDataFromUser()
{
    mailData_t data;
    initMailData(&data);
    FILE *file; // file for body text
    int br = 0; // number of bytes read

    char options[] = {"\n1) set from email\n2) set to email\n3) set subject\n4) select body txt file\n5) select png attachment\n6) done\n\n"};

    bool done = false;

    char bodyBuffer[2] = {'\0', '\0'};

    while (!done)
    {
        // create buffers
        char inputBuffer[BUFFER_SIZE];
        memset(inputBuffer, '\0', BUFFER_SIZE);
        unsigned char rawAttachmentBuffer[BUFFER_SIZE];
        memset(rawAttachmentBuffer, '\0', BUFFER_SIZE);

        // attachment encoded size
        size_t encodedAttachmentSize;

        // encoded attachment pointer
        char *encodedAttachment;

        // show options
        printf("%s", options);

        // read in first char (input should only be 1 char)
        scanf("%1s", inputBuffer);
        inputBuffer[1] = '\0';
        clearInputBuffer();

        // add new line to make terminal look better
        printf("\n");

        // get option as int
        int selected = atoi(inputBuffer);

        switch (selected)
        {
        case 1: // get from email
            printf("from: ");
            scanf("%63s", inputBuffer);
            clearInputBuffer();
            strncpy(data.from, inputBuffer, sizeof(data.from));
            printf("\n");
            printMailData(data);
            break;
        case 2: // get to email
            printf("to: ");
            scanf("%63s", inputBuffer);
            clearInputBuffer();
            strncpy(data.to, inputBuffer, sizeof(data.to));
            printf("\n");
            printMailData(data);
            break;
        case 3: // get subject
            printf("subject: ");
            fgets(inputBuffer, sizeof(inputBuffer), stdin);
            inputBuffer[strcspn(inputBuffer, "\n")] = '\0'; // remove newline char
            strncpy(data.subject, inputBuffer, sizeof(data.subject));
            printf("\n");
            printMailData(data);
            break;
        case 4: // get body
            // reset body text
            memset(data.body, '\0', sizeof(data.body));

            printf("path to body text: ");

            // get file path from user
            scanf("%255s", inputBuffer);
            clearInputBuffer();

            file = fopen(inputBuffer, "r");

            if (file == NULL)
            {
                printf("Failed to open body text file.\n");
                break;
            }

            br = fread(data.body, sizeof(char), sizeof(data.body) - 1, file);

            if (br < 0)
            {
                printf("Failed to read contexts of file.\n");
            }

            fclose(file);

            printMailData(data);
            break;
        case 5: // get attachment
            // reset attachment path text
            memset(data.attachmentName, '\0', sizeof(data.attachmentName));

            // get file path from user
            printf("file name: ");
            scanf("%255s", data.attachmentName);
            clearInputBuffer();

            // get file path from user
            printf("path to png: ");
            scanf("%255s", inputBuffer);
            clearInputBuffer();

            file = fopen(inputBuffer, "rb");

            if (file == NULL)
            {
                printf("Failed to open attachment.\n");
                break;
            }

            br = fread(rawAttachmentBuffer, sizeof(char), sizeof(rawAttachmentBuffer) - 1, file);
            fclose(file);

            if (br < 0)
            {
                printf("Failed to read contexts of attachment.\n");
            }

            if (br > sizeof(data.encodedAttachment))
            {
                printf("Attachment too large, max size is %ld bytes.", sizeof(data.encodedAttachment));

                // reset attachment name
                memset(data.attachmentName, '\0', sizeof(data.attachmentName));

                break;
            }

            // encode attachment
            encodedAttachment = base64_encode(rawAttachmentBuffer, br, &encodedAttachmentSize);

            // copy encoded attachment
            strncpy(data.encodedAttachment, encodedAttachment, sizeof(data.encodedAttachment));

            // free encoded attachment memory
            free(encodedAttachment);

            printMailData(data);
            break;
        case 6:
            done = true;
            break;
        default:
            printf("Invalid option selected: %s\n", inputBuffer);
        }
    }

    return data;
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

    mailData_t mailData = getMailDataFromUser();
    char cmdBuffer[BUFFER_SIZE + 3]; // extra bytes to add \r\n

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

    // send MAIL FROM to set sender email. Expecting 250 command completed
    snprintf(cmdBuffer, sizeof(cmdBuffer), "MAIL FROM:<%s>\r\n", mailData.from);
    if (sendCommand(socketFd, cmdBuffer, false, 250) < 0)
        exit(EXIT_FAILURE);

    // send RCPT TO to set receiver email. Expecting 250 command completed
    snprintf(cmdBuffer, sizeof(cmdBuffer), "RCPT TO:<%s>\r\n", mailData.to);
    if (sendCommand(socketFd, cmdBuffer, false, 250) < 0)
        exit(EXIT_FAILURE);

    // send DATA to set receiver email. Expecting 354 server confirm email addresses
    if (sendCommand(socketFd, "DATA\r\n", false, 354) < 0)
        exit(EXIT_FAILURE);

    // set subject segment. no response expected.
    snprintf(cmdBuffer, sizeof(cmdBuffer), "Subject: %s\r\n", mailData.subject);
    if (sendCommand(socketFd, cmdBuffer, false, 0) < 0)
        exit(EXIT_FAILURE);

    // set from email. no response expected.
    snprintf(cmdBuffer, sizeof(cmdBuffer), "From: %s\r\n", mailData.from);
    if (sendCommand(socketFd, cmdBuffer, false, 0) < 0)
        exit(EXIT_FAILURE);

    // set to email. no response expected.
    snprintf(cmdBuffer, sizeof(cmdBuffer), "To: %s\r\n", mailData.to);
    if (sendCommand(socketFd, cmdBuffer, false, 0) < 0)
        exit(EXIT_FAILURE);

    // set MIME version. no response expected.
    if (sendCommand(socketFd, "MIME-Version: 1.0\r\n", false, 0) < 0)
        exit(EXIT_FAILURE);

    // set MIME version. no response expected.
    if (sendCommand(socketFd, "Content-Type: multipart/mixed; boundary=\"dataBoundary\"\r\n", false, 0) < 0)
        exit(EXIT_FAILURE);

    // end header section. no response expected.
    if (sendCommand(socketFd, "\r\n", false, 0) < 0)
        exit(EXIT_FAILURE);

    // set boundary for text portion of email. no response expected.
    if (sendCommand(socketFd, "--dataBoundary\r\n", false, 0) < 0)
        exit(EXIT_FAILURE);

    // defining type for this bounded section
    if (sendCommand(socketFd, "Content-Type: text/plain; charset=\"utf-8\"\r\n", false, 0) < 0)
        exit(EXIT_FAILURE);

    // defining encoding for this bounded section
    if (sendCommand(socketFd, "Content-Transfer-Encoding: 7bit\r\n", false, 0) < 0)
        exit(EXIT_FAILURE);

    // end boundary description for this text section. no response expected.
    if (sendCommand(socketFd, "\r\n", false, 0) < 0)
        exit(EXIT_FAILURE);

    // set body. no response expected.
    snprintf(cmdBuffer, sizeof(cmdBuffer), "%s\r\n", mailData.body);
    if (sendCommand(socketFd, cmdBuffer, false, 0) < 0)
        exit(EXIT_FAILURE);

    // end text section. no response expected.
    if (sendCommand(socketFd, "\r\n", false, 0) < 0)
        exit(EXIT_FAILURE);

    // set boundary for png attachment
    if (sendCommand(socketFd, "--dataBoundary\r\n", false, 0) < 0)
        exit(EXIT_FAILURE);

    // set content type and name. no response expected.
    snprintf(cmdBuffer, sizeof(cmdBuffer), "Content-Type: image/png; name=\"%s\"", mailData.attachmentName);
    if (sendCommand(socketFd, cmdBuffer, false, 0) < 0)
        exit(EXIT_FAILURE);

    // defining encoding for this bounded section
    if (sendCommand(socketFd, "Content-Transfer-Encoding: base64\r\n", false, 0) < 0)
        exit(EXIT_FAILURE);

    // defining disposition for this bounded section
    snprintf(cmdBuffer, sizeof(cmdBuffer), "Content-Disposition: attachment; filename=\"%s\"\r\n", mailData.attachmentName);
    if (sendCommand(socketFd, cmdBuffer, false, 0) < 0)
        exit(EXIT_FAILURE);

    // end boundary description for this text section. no response expected.
    if (sendCommand(socketFd, "\r\n", false, 0) < 0)
        exit(EXIT_FAILURE);

    // send encoded attachment
    if (sendCommand(socketFd, mailData.encodedAttachment, false, 0) < 0)
        exit(EXIT_FAILURE);

    // end png section. no response expected.
    if (sendCommand(socketFd, "\r\n", false, 0) < 0)
        exit(EXIT_FAILURE);

    // // set final boundary. no response expected.
    // if (sendCommand(socketFd, "--dataBoundary--\r\n", false, 0) < 0)
    //     exit(EXIT_FAILURE);

    // finish data section. Expect 250 as command complete.
    if (sendCommand(socketFd, ".\r\n", false, 250) < 0)
        exit(EXIT_FAILURE);

    // send QUIT to gracefully close connection. Expecting 221 close connection confirm
    if (sendCommand(socketFd, "QUIT\r\n", false, 221) < 0)
        exit(EXIT_FAILURE);

    exit(EXIT_SUCCESS);
}