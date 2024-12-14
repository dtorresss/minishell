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

void	myjobs()
{
	printf("TODO: Jobs\n");
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

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	printf("msh> ");
	while (fgets(buf, BUFFER_SIZE, stdin))
	{
		line = tokenize(buf);

		if (line == NULL)
			continue;
		else
		{
			if (strcmp(line->commands[0].argv[0], "cd") == 0)
				mycd(line->commands[0].argv[1]);
			else if (strcmp(line->commands[0].argv[0], "jobs") == 0)
				myjobs();
			else if (strcmp(line->commands[0].argv[0], "fg") == 0)
				myfg();
			else if (strcmp(line->commands[0].argv[0], "exit") == 0)
				exit(0);
			else if (line->commands->filename == NULL)
				printf("%s: No se encuentra el mandato\n", line->commands[0].argv[0]);
			
			else {
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
						if (i > 0)
						{
							if (dup2(pipefd[i - 1][0], STDIN_FILENO) == -1)
							{
								perror("dup2 input");
								exit(EXIT_FAILURE);
							}
						}
						if (i < line->ncommands - 1)
						{
							if (dup2(pipefd[i][1], STDOUT_FILENO) == -1)
							{
								perror("dup2 output");
								exit(EXIT_FAILURE);
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
								exit(1);
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
									exit(1);
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
									exit(1);
								}
								dup2(fd_out, STDERR_FILENO);
								close(fd_out);
							}
						}
						execvp(line->commands[i].filename, line->commands[i].argv);
						perror("execvp");
						exit(EXIT_FAILURE);
					}
				}
				for (int i = 0; i < line->ncommands - 1; i++)
				{
					close(pipefd[i][0]);
					close(pipefd[i][1]);
				}
				for (int i = 0; i < line->ncommands; i++)
					wait(NULL);
			}
		}
		printf("msh> ");
	}
	return (EXIT_SUCCESS);
}
