#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include "constants.h"


/* Helper functions.*/
void exit_msg(char *msg);
int parse_line(char line[], int line_length, char *devpath);
void run_udisksctl(char *devpath);
void run_monitor(int fd);
int become_daemon(void);

int
main(int argc, char **argv)
{
	become_daemon();

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
		int wstatus;
		while ((c = getchar()) != EOF)
		{
			/* Append a character to the line buffer.*/
			cur_length = strlen(line);
			if (cur_length == LINE_LENGTH)
			{
				printf("Buffer overflow");
				exit(EXIT_FAILURE);
			}
			line[cur_length] = c;

			if (c == '\n')
			{
				parse_code = parse_line(line,
					        	cur_length + 1,
						       	devpath);

				if (!parse_code)
				{
					/* Fork a child to mount the partition.*/
					int tmp_pid;
					if ((tmp_pid = fork()) == -1)
						exit_msg("fork");
					else if (!tmp_pid)
						run_udisksctl(devpath);
					else
					{
						
						if (wait(&wstatus) == -1)
							exit_msg("wait");

						if (!wstatus)
							syslog(LOG_INFO,
							       "Device %s mounted\n",
							       devpath);
					}

					/* Clear the path.*/
					memset(devpath, 0, PATH_LENGTH);
				}

				/* Clear the line.*/
				memset(line, 0, LINE_LENGTH);
			}
		}
	}
	
	/* Get error code of child*/
	int wstatus;
	if (wait(&wstatus) == -1)
		exit_msg("wait");

	syslog(LOG_ERR, "udevadm return with non zero value %d", wstatus);

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
			   	   if ((words == ADD_NUM) && (!strcmp(word, ADD)))
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

/* Run the command udevadm monitor --udev -s block. To monitor the udev behaviour
 * for the block module.
 *
 * Arguments:
 * fd -- write end of a pipe.
 */
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

/* Make the program a deamon to run on the background.*/
int 
become_daemon(void)
{
	/* Fork to run on background.*/
	switch (fork())
	{
		case -1:
		   exit_msg("fork");
		case 0:
		   break;
		default:
		   exit(EXIT_SUCCESS);
	}

	/* Call setsid to start a new session.*/
	if (setsid() == -1)
		exit_msg("setsid");

	/* Clear process umask.*/
	umask(0);

	/* Change current working directory to /.*/
	if (chdir("/") == -1)
		exit_msg("chdir");

	/* Close all open file discriptors.*/
	int maxfd = sysconf(_SC_OPEN_MAX);
	if (maxfd == -1)
		maxfd = MAX_FD;

	for (int fd = 0; fd < MAX_FD; ++fd)
		close(fd);

	/* Reopen stdin, stdout and stderror and redirect them to /dev/null.*/
	int fd = open("/dev/null", O_RDWR);
	if (fd != STDIN)
		exit_msg("open");
	
	dup2(fd, STDOUT);
	dup2(fd, STDERR);

	return 0;
}

/* Helper for visualization.*/
void
exit_msg(char *msg)
{
	syslog(LOG_ERR, "%s %m", msg);
	exit(EXIT_FAILURE);
}
