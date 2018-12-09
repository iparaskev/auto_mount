#ifndef CONSTANTS_H
#define CONSTANTS_H

#define MONITOR (char* const[]){"udevadm",\
	                        "monitor",\
	                        "--udev",\
	                        "-s",\
                          	"block",\
        	                NULL}
#define MAX_FD 8192
#define STDIN 0
#define STDOUT 1
#define STDERR 2
#define LINE_LENGTH 250
#define PATH_LENGTH 150
#define ADD "add"
#define ADD_NUM 3
#define IN 0
#define OUT 1

#endif
