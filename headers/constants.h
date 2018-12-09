#ifndef CONSTANTS_H
#define CONSTANTS_H

/* The monotor command.*/
#define MONITOR (char* const[]){"udevadm",\
	                        "monitor",\
	                        "--udev",\
	                        "-s",\
                          	"block",\
        	                NULL}
#define MAX_FD 8192       /* Random number for max open fds.*/
#define STDIN 0           /* The stdin fd.*/ 
#define STDOUT 1          /* The stdout fd.*/ 
#define STDERR 2          /* The stderror fd.*/ 
#define LINE_LENGTH 250   /* Max length of monitor output.*/
#define PATH_LENGTH 150   /* Max length of device path.*/
#define ADD "add"         /* Add word for comparison.*/
#define ADD_NUM 3         /* The position of the add word in the output.*/
#define IN 0              /* Flag for declaring that the parser is in a word.*/
#define OUT 1             /* Flag for declaring that the parser out of a word.*/

#endif
