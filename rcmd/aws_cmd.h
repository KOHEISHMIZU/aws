#include <fstream>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
using namespace std;
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "../command.h"

#define SOCKET int

#ifndef SOCKET_ERROR 
#define SOCKET_ERROR (-1)
#endif

int aws_cmd(int argc, char ** argv, const char * cmd);
