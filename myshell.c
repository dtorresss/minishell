#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "parser.h"
#define BUFFER_SIZE 512

typedef struct {
	char state[8];
	char command[512];
	int	pid;
	tcommand * commands;
} proc;

void	myjobs(proc *proccesesBG, int numProcs)
{
	for (int i = 0; i < numProcs; i++)
		if (strcmp(proccesesBG[i].state, "Running") == 0)
			printf("[1]+   %s			%s" , proccesesBG[i].state, proccesesBG[i].command);
}

void	myfg()
{
	printf("TODO: Fg\n");
}

void	mycd(char *path)
{
	char	*dir;
	char	buffer[BUFFER_SIZE];

	if (path == NULL)
	{
		dir = getenv("HOME");
		if (dir == NULL)
			fprintf(stderr, "No existe la variable $HOME\n");
	}
	else
		dir = path;
	if (chdir(dir) != 0)
		fprintf(stderr, "Error al cambiar de directorio: %s\n", strerror(errno));
	else
		printf("El directorio actual es: %s\n", getcwd(buffer, -1));
}

int	main(void)
{
	char	buf[BUFFER_SIZE];
	tline	*line;
	pid_t	pid;
	int		i;

	proc	proccesesBG[512];
	int		proccesesFG[512];
	int		currentBG = 0;

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	while (1)
	{
		printf("msh> ");
		fgets(buf, BUFFER_SIZE, stdin);
		if (buf[0] != '\n')
			line = tokenize(buf);
		else
			continue;
		
		if (strcmp(line->commands[0].argv[0], "cd") == 0)
			mycd(line->commands[0].argv[1]);
		else if (strcmp(line->commands[0].argv[0], "jobs") == 0)
			myjobs(proccesesBG, currentBG);
		else if (strcmp(line->commands[0].argv[0], "fg") == 0)
			myfg();
		else if (strcmp(line->commands[0].argv[0], "exit") == 0)
			exit(EXIT_SUCCESS);
		else 
		{
			int		pipefd[line->ncommands - 1][2];
			for (i = 0; i < line->ncommands - 1; i++)
			{
				if (pipe(pipefd[i]) == -1)
				{
					perror("pipe");
					return (EXIT_FAILURE);
				}
			}
			for (i = 0; i < line->ncommands; i++)
			{		
				pid = fork();
				if (pid < 0)
				{
					perror("fork");
					return (EXIT_FAILURE);
				}
				else if (pid == 0)
				{
					signal(SIGINT, SIG_DFL);
					signal(SIGQUIT, SIG_DFL);
					if (line->ncommands > 1)
					{
						if (i == 0)
						{
							if (dup2(pipefd[i][1], STDOUT_FILENO) == -1)
							{
								perror("dup2 output");
								exit(EXIT_FAILURE);
							}
						}
						if (i == line->ncommands - 1)
						{
							if (dup2(pipefd[i - 1][0], STDIN_FILENO) == -1)
							{
								perror("dup2 input");
								exit(EXIT_FAILURE);
							}
						}
						if (i > 0 && i < line->ncommands - 1)
						{
							if (dup2(pipefd[i - 1][0], STDIN_FILENO) == -1)
							{
								perror("dup2 input");
								exit(EXIT_FAILURE);
							}
							if (dup2(pipefd[i][1], STDOUT_FILENO) == -1)
							{
								perror("dup2 output");
								exit(EXIT_FAILURE);
							}
						}
					}
					for (int j = 0; j < line->ncommands - 1; j++)
					{
						close(pipefd[j][0]);
						close(pipefd[j][1]);
					}
					if (i == 0 && line->redirect_input)
					{
						int fd_in = open(line->redirect_input, O_RDONLY);
						if (fd_in < 0)
						{
							printf("%s: Error. Archivo de entrada no válido.\n", line->redirect_input);
							exit(EXIT_FAILURE);
						}
						dup2(fd_in, STDIN_FILENO);
						close(fd_in);
					}
					if (i == line->ncommands - 1)
					{
						if (line->redirect_output)
						{
							int fd_out = open(line->redirect_output, O_CREAT | O_WRONLY | O_TRUNC, 0644);
							if (fd_out < 0)
							{
								printf("%s: Error. Archivo de salida no válido.\n", line->redirect_output);
								exit(EXIT_FAILURE);
							}
							dup2(fd_out, STDOUT_FILENO);
							close(fd_out);
						}
						if (line->redirect_error)
						{
							int fd_out = open(line->redirect_error, O_CREAT | O_WRONLY | O_TRUNC, 0644);
							if (fd_out < 0)
							{
								printf("%s: Error. Archivo de salida de error no válido.\n", line->redirect_error);
								exit(EXIT_FAILURE);
							}
							dup2(fd_out, STDERR_FILENO);
							close(fd_out);
						}
					}
					execvp(line->commands[i].argv[0], line->commands[i].argv);
					fprintf(stderr, "%s: No se encuentra el mandato\n", line->commands[i].argv[0]);
					exit(EXIT_FAILURE);
				}
				else if (pid > 0)
				{
					if (line->background == 1)
					{
						proc p;
						strcpy(p.command, buf);
						strcpy(p.state, "Running");
						p.commands = line->commands;
						p.pid = pid;
						proccesesBG[currentBG] = p;
						currentBG = currentBG + 1;
						printf("[%d] %d\n", currentBG, pid);
					}					
					else
						proccesesFG[i] = pid;
				}
			}
			for (int i = 0; i < line->ncommands - 1; i++)
			{
				close(pipefd[i][0]);
				close(pipefd[i][1]);
			}
			if (line->background==0)
			{
				for (int i = 0; i < line->ncommands; i++)
						waitpid(proccesesFG[i], NULL, 0);
			}
			int doneJobs = 0;
			for (int i = 0; i < currentBG; i++)
			{
				//printf("%s %d\n", proccesesBG[i].state, proccesesBG[i].pid);
				if (strcmp(proccesesBG[i].state, "Running") == 0)
				{
					int status = waitpid(proccesesBG[i].pid, NULL, WNOHANG);
					//printf("Status %d\n", status);
					if (status > 0)
					{
						//printf("Done\n");
						strcpy(proccesesBG[i].state, "Done");
						currentBG -= 1;
						doneJobs += 1;
					}
				}
			}
			if (doneJobs == line->ncommands)
				printf("[1]+   done			%s", buf);
		}
	}
	return (EXIT_SUCCESS);
}
