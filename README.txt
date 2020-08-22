
HTTP client

Description:

Program files:
HTTP_client.c - implement http/1.0 protocol to get or post request to a server.
the user insert data in the next format:
[–p <text>] [–r n <pr1=value1 pr2=value2 ...>] <URL>
while the URl is required and must be in the input to get the request


funcation:
    void initializeData - initialize all the data paramenters of the struct
    void findUrl - find the url in the argv input
    void findR - find -r in the argv input
    int findRValue - find all the value from the [–r n <pr1=value1 pr2=value2 ...>]
    void createHostAndPort - create host and port
    void createPath - create the path
    void findP - find -r in the argv input
    void buildRequest - build the request needed to sent to the server

    char *removeSubString - remove sub string from another string
    int countHowManyUrlAreInAllValue - count how mant value there is in the [–r n <pr1=value1 pr2=value2 ...>]
    void checkValidDataIsInsert - check if the input the user insert are valid
    char *IntToString - change int into a string
    void sendRequest - send the request to the server
    void freeAllDataWithError - free all memeory allocate and return error

