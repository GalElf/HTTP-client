#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

/*======= declaration of define =======*/

#define BUF_SIZE 1024
#define USAGE_ERROR "Usage: client [–p <text>] [–r n <pr1=value1 pr2=value2 ...>] <URL>\n"

/*======= main struct data =======*/

struct clientData
{
    char *request;
    int port;
    char *url;
    char *host;
    int urlIndex;

    int rIndex;
    int numOfValue;
    char *allValue;

    char *path;

    int pIndex;
    char *pText;
    int contentLength;

    int postRequest; // if -p exists then postRequest = 1 and getRequest = -1
    int getRequest;  // to write post or get
};
typedef struct clientData clientData;

/*======= declaration of function =======*/

void initializeData(clientData **data);
void findUrl(int argc, char *argv[], clientData **data);
void findR(int argc, char *argv[], clientData **data);
int findRValue(int argc, char *argv[], clientData **data);
void createHostAndPort(clientData **data);
void createPath(clientData **data);
void findP(int argc, char *argv[], clientData **data);
void buildRequest(char *argv[], clientData **data);

char *removeSubString(char *string, char *sub);
int countHowManyUrlAreInAllValue(char *argv[], clientData **data);
void checkValidDataIsInsert(int argc, clientData **data);
char *IntToString(int num);
void sendRequest(clientData **data);
void freeAllDataWithError(char *msg, clientData **data);

/*======= Main =======*/

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        return 0;
    }
    clientData *data;
    initializeData(&data);
    findP(argc, argv, &data);
    findR(argc, argv, &data);
    findUrl(argc, argv, &data);
    createHostAndPort(&data);
    createPath(&data);
    checkValidDataIsInsert(argc, &data);
    // the required print:
    buildRequest(argv, &data);
    printf("HTTP request =\n%s\nLEN = %ld\n", data->request, strlen(data->request));
    sendRequest(&data);

    free(data->path);
    free(data->allValue);
    free(data->host);
    free(data->request);
    free(data);
    return 0;
}

void checkValidDataIsInsert(int argc, clientData **data)
{
    int countParameter = 1; // url already check
    if ((*data)->pIndex != -1)
    {
        countParameter += 2;
    }
    if ((*data)->rIndex != -1)
    {
        countParameter += 2;
        countParameter += (*data)->numOfValue;
    }
    if (argc - 1 != countParameter)
    {
        freeAllDataWithError(USAGE_ERROR, data);
    }
}

// initialize all the data paramenters
void initializeData(clientData **data)
{
    (*data) = (clientData *)malloc(sizeof(clientData));
    assert(data);
    (*data)->request = NULL;
    (*data)->port = 80;

    (*data)->url = NULL;
    (*data)->host = NULL;
    (*data)->urlIndex = -1;

    (*data)->rIndex = -1;
    (*data)->numOfValue = 0;
    (*data)->allValue = NULL;

    (*data)->path = NULL;

    (*data)->pIndex = -1;
    (*data)->pText = NULL;
    (*data)->contentLength = 0;

    (*data)->postRequest = -1;
    (*data)->getRequest = 1;
}

// this function find the url
void findUrl(int argc, char *argv[], clientData **data)
{
    int countHowManyUrl = 0;
    int urlIndex = -1;
    int checkP = 0;
    int checkR = 0;
    for (int i = 0; i < argc; i++)
    {
        if (strstr(argv[i], "http"))
        {
            if (checkP == 0 && (*data)->pIndex != -1 && strcmp((*data)->pText, argv[i]) == 0)
            { // this if check if there is url int test of the -p
                checkP = 1;
            }
            else if (checkR < countHowManyUrlAreInAllValue(argv, data) && (*data)->rIndex != -1 && strstr((*data)->allValue, argv[i]))
            {
                checkR++;
            }
            else
            {
                urlIndex = i; // save the location in the argv
                countHowManyUrl++;
                break;
            }
        }
    }
    if (countHowManyUrl != 1)
    { // check if there are 2 url or there isn't any url in the input
        free((*data)->allValue);
        free(*data);
        fprintf(stderr, USAGE_ERROR);
        exit(EXIT_FAILURE);
    }
    (*data)->url = argv[urlIndex];
    (*data)->urlIndex = urlIndex;
}

// this function find the -r and check is parameter then create the path value for the path
void findR(int argc, char *argv[], clientData **data)
{
    if (strcmp("-r", argv[argc-1]) == 0)
    {
        fprintf(stderr, USAGE_ERROR);
        free(*data);
        exit(EXIT_FAILURE);
    }
    int numOfValue = 0;
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-r") == 0)
        {
            if ((*data)->pIndex != -1 && (1 + (*data)->pIndex) != i)
            {
                (*data)->rIndex = i;
                (*data)->numOfValue = atoi(argv[i + 1]);
                break;
            }
            if ((*data)->pIndex == -1)
            {
                (*data)->rIndex = i;
                (*data)->numOfValue = atoi(argv[i + 1]);
                break;
            }
        }
    }
    if ((*data)->rIndex != -1)
    {
        if ((*data)->numOfValue > 0)
        {
            numOfValue = findRValue(argc, argv, data);
            if (numOfValue != (*data)->numOfValue)
            {
                free((*data)->allValue);
                free(*data);
                fprintf(stderr, USAGE_ERROR);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (numOfValue == 0)
            {
                if (argv[1 + (*data)->rIndex][0] != '0')
                {
                    free((*data)->allValue);
                    free(*data);
                    fprintf(stderr, USAGE_ERROR);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
}

// this function take the url and make it as the host www.---.com
void createHostAndPort(clientData **data)
{
    char *url = NULL;
    url = (char *)malloc(1 + strlen((*data)->url));
    assert(url);
    strncpy(url, (*data)->url, 1 + strlen((*data)->url));

    char *tempHost = strstr(url, "http://");
    if (tempHost != NULL)
    {
        url = removeSubString(url, "http://");
    }

    tempHost = strchr(url, '/');
    if (tempHost != NULL)
    {
        for (int i = strlen(url); i >= strlen(url) - strlen(tempHost); i--)
        {
            url[i] = '\0';
        }
    }

    (*data)->host = (char *)malloc(1 + strlen(url));
    assert((*data)->host);
    strncpy((*data)->host, url, 1 + strlen(url));

    if (strchr((*data)->host, ':'))
    { // check if port exist
        char *tempPort = (char *)malloc(1 + strlen(strchr((*data)->host, ':')));
        assert(tempPort);
        strncpy(tempPort, strchr((*data)->host, ':'), 1 + strlen(strchr((*data)->host, ':')));

        ((*data)->host)[strlen((*data)->host) - strlen(tempPort)] = '\0';

        removeSubString(tempPort, ":");
        if (strcmp(tempPort, "") == 0 || atoi(tempPort) == 0)
        {                       // check if after the : there is an empty string or ilegal port
            (*data)->port = -1; // change to -1 to fail the connection in the server
        }
        else
        {
            (*data)->port = atoi(tempPort);
        }
        free(tempPort);
    }
    free(url);
}

// this function create the full path the part of the url and the values from the -r data
void createPath(clientData **data)
{
    char *url = NULL;
    url = (char *)malloc(1 + strlen((*data)->url));
    assert(url);
    strncpy(url, (*data)->url, 1 + strlen((*data)->url));

    url = removeSubString(url, "http://");
    char *tempHost = strchr(url, '/');
    if (tempHost == NULL)
    {
        if ((*data)->allValue == NULL)
        {
            char *tempPath = "/ HTTP/1.0";
            (*data)->path = (char *)malloc(strlen(tempPath) + 1);
            assert((*data)->path);
            strncpy((*data)->path, tempPath, strlen(tempPath) + 1);
        }
        else
        {
            char *tempPath = " HTTP/1.0";
            int len = strlen("/?") + strlen((*data)->allValue) + strlen(tempPath);
            (*data)->path = (char *)malloc(len + 1);
            assert((*data)->path);
            strncpy((*data)->path, "/?", strlen(tempPath) + 1);
            strcat((*data)->path, (*data)->allValue);
            strcat((*data)->path, tempPath);
        }
    }
    else
    {
        if ((*data)->allValue == NULL)
        {
            char *tempPath = " HTTP/1.0";
            int len = strlen(tempHost) + strlen(tempPath);
            (*data)->path = (char *)malloc(len + 1);
            assert((*data)->path);
            strncpy((*data)->path, tempHost, strlen(tempHost) + 1);
            strcat((*data)->path, tempPath);
        }
        else
        {
            char *tempPath = " HTTP/1.0";
            int len = strlen(tempHost) + strlen("?") + strlen((*data)->allValue) + strlen(tempPath);
            (*data)->path = (char *)malloc(len + 1);
            assert((*data)->path);
            strncpy((*data)->path, tempHost, strlen(tempHost) + 1);
            strcat((*data)->path, "?");
            strcat((*data)->path, (*data)->allValue);
            strcat((*data)->path, tempPath);
        }
    }
    free(url);
}

// take all <pr1=value1 pr2=value2 ...> and change it into a pr1=value1&pr2=value2&... for the path
int findRValue(int argc, char *argv[], clientData **data)
{
    int memorySize = 1000;
    int numOfValue = 0;
    int countFreeMemory = 0;
    (*data)->allValue = (char *)malloc(memorySize * sizeof(char));
    assert((*data)->allValue);
    strncpy((*data)->allValue, "", 1);
    for (int i = 2 + (*data)->rIndex; i < argc; i++)
    {
        if (strchr(argv[i], '=') != NULL)
        {
            while (1)
            {
                if ((countFreeMemory + strlen(argv[i])) >= memorySize)
                { // if the allocation memory is over or not enough it will realloc
                    memorySize += memorySize;
                    (*data)->allValue = (char *)realloc((*data)->allValue, memorySize);
                    assert((*data)->allValue);
                }
                else
                { // if there are still not enough memory it will relloc again
                    break;
                }
            }
            numOfValue++;
            char *c = strcat((*data)->allValue, argv[i]);
            assert(c);
            countFreeMemory += strlen(argv[i]);
            if (i < argc - 1)
            {
                if (i < argc && strchr(argv[i + 1], '=') != NULL)
                {
                    c = strcat((*data)->allValue, "&");
                    assert(c);
                    countFreeMemory++;
                }
            }
        }
        else
        {
            break;
        }
    }
    (*data)->allValue = (char *)realloc((*data)->allValue, strlen((*data)->allValue) + 1);
    assert((*data)->allValue);
    return numOfValue;
}

// this fucntiob check if -p is exist and initialize all the parameter that needed.
void findP(int argc, char *argv[], clientData **data)
{
    if (strcmp("-p", argv[argc-1]) == 0)
    {
        free(*data);
        fprintf(stderr, USAGE_ERROR);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
        {
            (*data)->pIndex = i;
            (*data)->getRequest = -1;
            (*data)->postRequest = 1;
            (*data)->pText = argv[i + 1];
            (*data)->contentLength = strlen(argv[i + 1]);
            break;
        }
    }
}

// remove a sub String from another String
char *removeSubString(char *str, char *subStr)
{
    int len = strlen(subStr);
    if (len > 0)
    {
        char *p = str;
        while ((p = strstr(p, subStr)) != NULL)
        {
            memmove(p, p + len, strlen(p + len) + 1);
        }
    }
    return str;
}

// this function count how many url are inside [–r n <pr1=value1 pr2=value2 ...>]
int countHowManyUrlAreInAllValue(char *argv[], clientData **data)
{
    int count = 0;
    int i = 2 + (*data)->rIndex;
    int lastValue = i + ((*data)->numOfValue);
    while (i < lastValue)
    {
        if (strstr(argv[i], "http://"))
        {
            count++;
        }
        i++;
    }
    return count;
}

// create one string that will contain all the data POST/GET, HOST and so to sent the request
void buildRequest(char *argv[], clientData **data)
{
    //Request:
    size_t len = 0;
    const char *host = "HOST: ";
    const char *post = "POST ";
    const char *get = "GET ";
    const char *contentLength = "Content-length:";
    if ((*data)->postRequest == 1)
    { // create POST Request
        char *tempContentLength = IntToString((*data)->contentLength);
        len = strlen(post) + strlen((*data)->path) + 2 + strlen(host) + strlen((*data)->host) + 2 + strlen(contentLength) + strlen(tempContentLength) + 4 + strlen((*data)->pText);
        (*data)->request = (char *)malloc(len + 1);
        assert((*data)->request);
        strncpy((*data)->request, post, strlen(post) + 1);
        strcat((*data)->request, (*data)->path);
        strcat((*data)->request, "\r\n");
        strcat((*data)->request, host);
        strcat((*data)->request, (*data)->host);
        strcat((*data)->request, "\r\n");
        strcat((*data)->request, contentLength);
        strcat((*data)->request, tempContentLength);
        strcat((*data)->request, "\r\n\r\n");
        strcat((*data)->request, (*data)->pText);
        free(tempContentLength);
    }
    if ((*data)->getRequest == 1)
    { // create GET Request
        len = strlen(get) + strlen((*data)->path) + 2 + strlen(host) + strlen((*data)->host) + 4;
        (*data)->request = (char *)malloc(len + 1);
        assert((*data)->request);
        strncpy((*data)->request, get, strlen(get) + 1);
        strcat((*data)->request, (*data)->path);
        strcat((*data)->request, "\r\n");
        strcat((*data)->request, host);
        strcat((*data)->request, (*data)->host);
        strcat((*data)->request, "\r\n\r\n");
    }
}

// change int into string
char *IntToString(int num)
{
    int memorySize = 10; // int can be at max 10 "char" - 2147483647 then the max memory to create it as string will be 10
    char *tempContentLength = (char *)malloc(memorySize + 1);
    assert(tempContentLength);
    sprintf(tempContentLength, "%d", num);
    tempContentLength = (char *)realloc(tempContentLength, strlen(tempContentLength) + 1);
    assert(tempContentLength);
    return tempContentLength;
}

void sendRequest(clientData **data)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    { // create socket
        close(sockfd);
        freeAllDataWithError("ERROR, opening socket\n", data);
    }

    server = gethostbyname((*data)->host);
    if (server == NULL)
    {
        close(sockfd);
        herror("ERROR, no such host\n");
        freeAllDataWithError("", data);
    }

    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0], (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons((*data)->port);

    if (connect(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        close(sockfd);
        freeAllDataWithError("ERROR, commecting\n", data);
    }

    int sentAndGetRequest = write(sockfd, (*data)->request, strlen((*data)->request) + 1);
    if (sentAndGetRequest < 0)
    { // send to the server the request
        close(sockfd);
        freeAllDataWithError("ERROR, write request has failed\n", data);
    }

    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE * sizeof(char));
    int countSizeOfByte = 0;
    while (1)
    {
        sentAndGetRequest = read(sockfd, buffer, BUF_SIZE - 1);
        if (sentAndGetRequest > 0)
        { // geting the data from the server
            if (write(STDOUT_FILENO, buffer, sentAndGetRequest) < 0)
            {
                close(sockfd);
                freeAllDataWithError("ERROR, write data has failed", data);
            }
            countSizeOfByte += strlen(buffer);
            memset(buffer, 0, BUF_SIZE * sizeof(char));
        }
        else if (sentAndGetRequest == 0)
        { // if we read all the data then exit
            break;
        }
        else
        {
            close(sockfd);
            freeAllDataWithError("ERROR, read data has failed", data);
        }
    }
    printf("\n Total received response bytes: %d\n",countSizeOfByte);
    close(sockfd);
}

// this function call if there is problem and free all the allocate memory and exit the program
void freeAllDataWithError(char *msg, clientData **data)
{
    free((*data)->path);
    free((*data)->allValue);
    free((*data)->host);
    free((*data)->request);
    free((*data));
    if (strcmp(msg, "") != 0)
    {
        perror(msg);
    }
    exit(EXIT_FAILURE);
}