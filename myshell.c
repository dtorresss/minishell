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

void myjobs() {
	printf("TODO: Jobs\n");
}

void myfg() {
	printf("TODO: Fg\n");
}

void mycd(char *path, int len) {
	char	*dir;
	char	buffer[BUFFER_SIZE];

	if (len == 0) {
		dir = getenv("HOME");
		if (dir == NULL)
			fprintf(stderr,"No existe la variable $HOME\n");
	}
	else
		dir = path;
	
	if (chdir(dir) != 0)
		fprintf(stderr,"Error al cambiar de directorio: %s\n", strerror(errno));
	else
		printf( "El directorio actual es: %s\n", getcwd(buffer,-1));
}

int main() {
	char	buf[BUFFER_SIZE];
	tline	*line;
	pid_t	pid;

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	printf("msh> ");	
	while (fgets(buf, BUFFER_SIZE, stdin))
	{
		line = tokenize(buf);

		if (line==NULL)
			continue;
		else if (line->ncommands == 1)
		{
			if (strcmp(line->commands[0].argv[0], "cd") == 0)
				mycd(line->commands[0].argv[1], line->commands[0].argc - 1);
			else if (strcmp(line->commands[0].argv[0], "jobs") == 0)
				myjobs();
			else if (strcmp(line->commands[0].argv[0], "fg") == 0)
				myfg();
			else if (strcmp(line->commands[0].argv[0], "exit") == 0)
				exit(0);
			else if (line->commands->filename == NULL)
				printf("%s: No se encuentra el mandato\n", line->commands[0].argv[0]);
			else
			{
				pid = fork();
				if (pid == 0)
				{
					signal(SIGINT, SIG_DFL);
					signal(SIGQUIT, SIG_DFL);
					if (line->redirect_input != NULL)
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
					if (line->redirect_output != NULL)
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
					if (line->redirect_error != NULL)
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
					execvp(line->commands[0].filename, line->commands[0].argv);
					fprintf(stderr,"Se ha producido un error.\n");
					exit(1);
				}
				else
					wait(NULL);
			}
		}
		/*
		else if (line->ncommands == 2)
		{
			int		pip_des[2];

			if (line->commands[0].filename == NULL)
				printf("%s: No se encuentra el mandato\n", line->commands[0].argv[0]);
			if (line->commands[1].filename == NULL)
				printf("%s: No se encuentra el mandato\n", line->commands[1].argv[0]);

			else if (line->commands[0].filename != NULL && line->commands[1].filename != NULL)
			{
				pid = fork();
				if (pid == 0)
				{
					signal(SIGINT, SIG_DFL);
					signal(SIGQUIT, SIG_DFL);
					pipe(pip_des);
					pid = fork();
					if (pid == 0){
						if (line->redirect_input != NULL)
						{
							int fd_in = open(line->redirect_input, O_RDONLY);
							if (fd_in < 0) {
								printf("%s: Error. Archivo de entrada no válido.\n", line->redirect_input);
								exit(1);
							}
							dup2(fd_in, STDIN_FILENO);
							close(fd_in);
						}
						close(pip_des[0]);
						dup2(pip_des[1], STDOUT_FILENO);
						close(pip_des[1]);
						execvp(line->commands[0].filename, line->commands[0].argv);
						fprintf(stderr,"Se ha producido un error.\n");
						exit(1);            
					}
					else {
						if (line->redirect_output != NULL) {
							int fd_out = open(line->redirect_output, O_CREAT | O_WRONLY | O_TRUNC, 0644);
							if (fd_out < 0) {
								printf("%s: Error. Archivo de salida no válido.\n", line->redirect_output);
								exit(1);
							}
							dup2(fd_out, STDOUT_FILENO);
							close(fd_out);
						}
						if (line->redirect_error != NULL) {
							int fd_out = open(line->redirect_error, O_CREAT | O_WRONLY | O_TRUNC, 0644);
							if (fd_out < 0) {
								printf("%s: Error. Archivo de salida de error no válido.\n", line->redirect_error);
								exit(1);
							}
							dup2(fd_out, STDERR_FILENO);
							close(fd_out);
						}
						close(pip_des[1]);
						dup2(pip_des[0], STDIN_FILENO);
						close(pip_des[0]);
						wait(NULL);
						execvp(line->commands[1].filename, line->commands[1].argv);
						fprintf(stderr,"Se ha producido un error.\n");
						exit(1);
					}
				}
				else
					wait(NULL);
			}
		}*/
		// Parte exotica
		else // if (line->ncommands > 2)
		{
			int		ultra_exotic_pipes[line->ncommands - 1][2];
			pid_t	hijos[line->ncommands];

			for (int i = 0; i < line->ncommands - 1; i++)
			{
				if (pipe(ultra_exotic_pipes[i]) == -1)
				{
					perror("pipe");
					return 1;
				}
			}

			if (line->redirect_input != NULL)
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

				if (line->redirect_output != NULL)
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

				if (line->redirect_error != NULL)
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

			// Crear un proceso para cada comando
			for (int i = 0; i < line->ncommands; i++)
			{
				pid = fork();

				if (pid < 0)
				{
					perror("fork");
					return 1;
				}
				if (pid == 0)
				{
					signal(SIGINT, SIG_DFL);
					signal(SIGQUIT, SIG_DFL);

					// Redirección de entrada para todos los procesos, excepto el primero
					if (i > 0)
					{
						// printf("i>0%d\n", i);
						close(ultra_exotic_pipes[i - 1][1]);
						dup2(ultra_exotic_pipes[i - 1][0], STDIN_FILENO);
						close(ultra_exotic_pipes[i - 1][0]);
					}

					// Redirección de salida para todos los procesos, excepto el último
					if (i < line->ncommands - 1)
					{
						// printf("i < line->ncommands%d\n", i);
						// close(ultra_exotic_pipes[i][0]);
						dup2(ultra_exotic_pipes[i][1], STDOUT_FILENO);
						close(ultra_exotic_pipes[i][1]);
					}
					// if (i == line->ncommands - 1)
					// {
					// 	close(ultra_exotic_pipes[i][0]);
					// 	dup2(ultra_exotic_pipes[i][1], STDOUT_FILENO);
					// 	close(ultra_exotic_pipes[i][1]);
					// }
					execvp(line->commands[i].filename, line->commands[i].argv);
					perror("execvp");
					exit(1);
				}
				else
					hijos[i] = pid;
			}
			for (int i = 0; i < line->ncommands - 1; i++)
			{
				close(ultra_exotic_pipes[i][0]);
				close(ultra_exotic_pipes[i][1]);
			}
			for (int i = 0; i < line->ncommands; i++)
				waitpid(hijos[i], NULL, 0);
			

		}
		printf("msh> ");
	}
	return 0;
}
