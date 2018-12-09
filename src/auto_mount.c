#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MONITOR (char* const[]){"udevadm",\
	                        "monitor",\
	                        "--udev",\
	                        "-s",\
                          	"block",\
        	                NULL}
#define ADD "add"
#define ADD_NUM 3
#define STDIN 0
#define STDOUT 1
#define LINE_LENGTH 250
#define PATH_LENGTH 150
#define IN 0
#define OUT 1

void
exit_msg(char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

void
run_monitor(int fd)
{
	/* duplicate stdout with the read end of pipe.*/
	if (dup2(fd, STDOUT) == -1)
		exit_msg("dup2");

	/* Close the unused fd.*/
	if (close(fd) == -1)
		exit_msg("close");

	execvp(*MONITOR, MONITOR);
	exit_msg("execvp");
}

int parse_line(char line[], int line_length, char *devpath);
void run_udisksctl(char *devpath);

int
main(int argc, char **argv)
{
	int fds[2];

	/* Open the pipe.*/
	if (pipe(fds) == -1)
		exit_msg("Pipe.");

	int pid;
	if ((pid = fork()) == -1)
		exit_msg("Fork");
	else if (!pid)
	{
		/* Close read end of the pipe.*/
		if (close(fds[0]) == -1)
			exit_msg("close");

		/* Call monitor function.*/
		run_monitor(fds[1]);
	}
	else
	{
		/* Close write end of the pipe.*/
		if (close(fds[1]) == -1)
			exit_msg("close");

		if (dup2(fds[0], STDIN) == -1)
			exit_msg("dup2");


		/* Buffer for storing every line.*/
		char line[LINE_LENGTH];
		memset(line, 0, LINE_LENGTH);

		char *devpath;
		if ((devpath = malloc(PATH_LENGTH * sizeof *devpath)) == NULL)
			exit_msg("malloc");

		int c, parse_code;
		int cur_length;
		while ((c = getchar()) != EOF)
		{
			printf("%c", c);

			/* Append a character to the line buffer.*/
			cur_length = strlen(line);
			if (cur_length == LINE_LENGTH)
			{
				printf("Buffer overfloat");
				exit(EXIT_FAILURE);
			}
			line[cur_length] = c;

			if (c == '\n')
			{
				parse_code = parse_line(line,
					        	cur_length + 1,
						       	devpath);
				printf("Code: %d, Devpath: %s\n", parse_code, devpath);

				if (!parse_code)
				{
					/* Fork a child to mount the partition.*/
					int tmp_pid;
					if ((tmp_pid = fork()) == -1)
						exit_msg("fork");
					else if (!tmp_pid)
						run_udisksctl(devpath);
					else
						if (wait(NULL) == -1)
							exit_msg("wait");
					/* Clear the path.*/
					memset(devpath, 0, PATH_LENGTH);
				}

				/* Clear the line.*/
				memset(line, 0, LINE_LENGTH);
			}
		}
	}
	
	return 0;
}

/* Parse a line and find if is a device addition. If it is return the device
 * path.
 *
 * Arguments:
 * line -- line to be parsed
 * line_length -- length of the line
 * devpath -- pointer to the returned string
 *
 * Returns:
 * 0 -- in success
 * 1 -- if it isn't an addition
 */
int
parse_line(char line[], int line_length, char *devpath)
{
	/* The running word.*/
	char word[PATH_LENGTH];
	int word_length;

	int exit_status = 1;
	int last_slash = -1;
	int words = 0;
	int state = OUT;
	for (int i = 0; i < line_length; ++i)
	{
		switch (line[i]) 
		{
			case ' ':
			case '\t':
			case '\n':
				if (state == IN)
				{
					state = OUT;
					words++;

					/* Check if the third word is add.*/
					if ((words == ADD_NUM) &&\
					    (!strcmp(word, ADD)))
						exit_status = 0;
					else if (words == ADD_NUM)
						return exit_status;
				}
				break;
			default:
				if (state == OUT)
				{
					/* Init state and word.*/
					state = IN;
					memset(word, 0, PATH_LENGTH);
				}

				word_length = strlen(word);
				word[word_length] = line[i];
				
				if (!exit_status && words == ADD_NUM)
				{
					if (line[i] == '/')
						last_slash = word_length;

					/* Append text to devpath.*/
					devpath[word_length] = line[i];
				}
		}
	}

	/* Modify devpath.*/
	if (!exit_status)
	{
		char *keep = malloc(10 * sizeof *keep);
		strcpy(keep, &devpath[last_slash]);

		/* Overide devpath.*/
		memset(devpath, 0, PATH_LENGTH);
		strcat(devpath, "/dev");
		strcat(devpath, keep);
	}

	return exit_status;
}

/* Run command udisksctl mount -b devpath --no-user-interaction.
 *
 * Arguments:
 * devpath -- the /dev/name of the device
 */
void
run_udisksctl(char *devpath)
{
	char **cmd;
	int args = 6;
	if ((cmd = malloc(args * sizeof *cmd)) == NULL)
		exit_msg("malloc");
	cmd[0] = "udisksctl";
	cmd[1] = "mount";
	cmd[2] = "-b";
	cmd[3] = devpath;
	cmd[4] = "--no-user-interaction";
	cmd[5] = NULL;

	/* Execute command.*/
	execvp(*cmd, cmd);
	exit_msg("execvp");
}
