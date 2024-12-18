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
#define FG_SIZE 512
#define BG_SIZE 512

// Estructura que almacena información sobre procesos en segundo plano
typedef struct {
	char	state[8];		// Estado actual del proceso: "Running", "Done"
	char	command[BG_SIZE];	// Comandos en la línea
	int		ncommands;		// Número de comandos en la línea
	int		pid[BG_SIZE];		// PIDs de los procesos hijos
	int		finishedC;		 // Número de comandos completados
}	proc;

// Función que muestra los trabajos en segundo plano activos
void	myjobs(proc bgProcs[BG_SIZE], int contProcs)
{
	int cont = 0;
	for (int i = 0; cont < contProcs; i++)	// Itera sobre los procesos activos
	{
		if (bgProcs[i].finishedC < bgProcs[i].ncommands)	// Solo muestra procesos no finalizados
		{
			printf("[%d]+	%s			%s", i+1, bgProcs[i].state, bgProcs[i].command);
			cont += 1;
		}
	}
}

// Función para traer un trabajo de segundo plano al primer plano
int myfg(proc *bgProcs, int contProcs, char **proc, int numProc)
{
	int		proccess;
	
	if (contProcs > 0)
	{
		if (numProc == 1)	// Si no se especifica un identificador, se toma el último proceso
		{
			printf("[%d]+ %s		%s", contProcs, bgProcs[contProcs-1].state, bgProcs[contProcs-1].command);
			waitpid(bgProcs[contProcs-1].pid[bgProcs[contProcs-1].ncommands - 1], NULL, 0);
			bgProcs[contProcs-1].finishedC = bgProcs[contProcs-1].ncommands;
			strcpy(bgProcs[contProcs-1].state, "Done");
		}
		else	// Si se especifica un número de proceso
		{
			proccess = atoi(proc[1]);
			if (proccess <= 0 || proccess > contProcs)	// Validación del proceso
			{
				fprintf(stderr, "Error: Mandato con identificador [%d] no encontrado.\n", proccess);
				return 0;
			}
			else	// Mueve el proceso al primer plano
			{
				printf("[%d]+ %s		%s", proccess, bgProcs[proccess - 1].state, bgProcs[proccess - 1].command);
				waitpid(bgProcs[proccess - 1].pid[bgProcs[proccess - 1].ncommands - 1], NULL, 0);
				bgProcs[proccess-1].finishedC = bgProcs[proccess-1].ncommands;
				strcpy(bgProcs[proccess - 1].state, "Done");
			}
		}
		return 1;
	}
	fprintf(stderr, "No hay ningún mandato en background.\n");
	return 0;
}

// Implementación del comando cd
void	mycd(char *path)
{
	char	*dir;
	char	buffer[BUFFER_SIZE];

	if (path == NULL)	// Si no se especifica un directorio usa $HOME
	{
		dir = getenv("HOME");
		if (dir == NULL)
			fprintf(stderr, "No existe la variable $HOME\n");
	}
	else
		dir = path;
	if (chdir(dir) != 0)	// Cambio de directorio
		fprintf(stderr, "Error al cambiar de directorio: %s\n", strerror(errno));
	else
		printf("El directorio actual es: %s\n", getcwd(buffer, -1));
}

// Actualiza los procesos en segundo plano y devuelve cuántos terminaron
int updateBG(int contBGLines, proc *bgProcs)
{
	int numFinished = 0;

	for (int i = 0; i < contBGLines; i++)	// Recorre todos los procesos en segundo plano
	{
		if (bgProcs[i].finishedC != bgProcs[i].ncommands)	// Solo verifica procesos no finalizados
		{
			int j = bgProcs[i].finishedC;
			while (j < bgProcs[i].ncommands)
			{
				pid_t res = waitpid(bgProcs[i].pid[j], NULL, WNOHANG); // Se comprueba cada comando de cada línea haya acabado
				if (res > 0)
				{
					bgProcs[i].finishedC += 1;	// Incrementa contador de procesos finalizados
				}
				j += 1;
			}
			if (bgProcs[i].finishedC == bgProcs[i].ncommands)	// Todos los procesos han finalizado
			{
				strcpy(bgProcs[i].state, "Done");
				printf("[%d]+ %s		%s", contBGLines, bgProcs[i].state, bgProcs[i].command);
				numFinished += 1;
			}
		}
	}
	return (numFinished);
}

int	main(void)
{
	char	buf[BUFFER_SIZE];
	tline	*line;
	pid_t	pid;
	int		i;
	int		pidFG[FG_SIZE];	// PIDs para procesos en primer plano
	int		pidBG[BG_SIZE];	// PIDs para procesos en segundo plano
	int		contBGLines = 0;
	proc	bgProcs[BG_SIZE];	// Almacena información de procesos en segundo plano

    // Ignora señales para evitar interrupciones no deseadas
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	while (1)
	{
		contBGLines -= updateBG(contBGLines, bgProcs);	// Actualiza procesos en segundo plano
		printf("msh> ");
		fgets(buf, BUFFER_SIZE, stdin);	// Lee la entrada del usuario
		
		if (buf[0] == '\n')
			continue;
		
		line = tokenize(buf);	// Tokeniza la línea de entrada

		if (line == NULL)
			continue;
		// Implementación de comandos internos
		if (strcmp(line->commands[0].argv[0], "cd") == 0)
			mycd(line->commands[0].argv[1]);
		else if (strcmp(line->commands[0].argv[0], "jobs") == 0)
			myjobs(bgProcs, contBGLines);
		else if (strcmp(line->commands[0].argv[0], "fg") == 0)
			contBGLines -= myfg(bgProcs, contBGLines, line->commands[0].argv, line->commands[0].argc);
		else if (strcmp(line->commands[0].argv[0], "exit") == 0)
			exit(EXIT_SUCCESS);
		else // Ejecución de comandos externos
		{
			int **pipefd = malloc((line->ncommands - 1) * sizeof(int*)); // n - 1 pipes
			if (pipefd == NULL)
			{
				perror("malloc");
				return (EXIT_FAILURE);
			}
			for (i = 0; i < line->ncommands - 1; i++)
			{
				pipefd[i] = malloc(2 * sizeof(int));
				if (pipefd[i] == NULL)
				{
					perror("malloc");
					for (int j = 0; j < i; j++)
						free(pipefd[j]);
					free(pipefd);
					return (EXIT_FAILURE);
				}
				if (pipe(pipefd[i]) == -1)
				{
					for (int j = 0; j <= i; j++)
						free(pipefd[j]);
            		free(pipefd);
					perror("pipe");
					return (EXIT_FAILURE);
				}
			}
			for (i = 0; i < line->ncommands; i++)
			{		
				pid = fork();	// Crea un proceso hijo para ejecutar cada comando
				if (pid < 0)
				{
					perror("fork");
					for (int j = 0; j < line->ncommands - 1; j++)
					{
						close(pipefd[j][0]);
						close(pipefd[j][1]);
						free(pipefd[j]);
					}
					free(pipefd);
					return (EXIT_FAILURE);
				}
				else if (pid > 0)	// Código del proceso padre
				{
					// Guarda los pids dependiendo de si la ejecución es en foreground o background
					if (line->background == 0)
						pidFG[i] = pid;	// Guarda los PIDs de procesos foreground
					else
					{
						pidBG[i] = pid;	// Guarda los PIDs de procesos background
						if (i == line->ncommands - 1)
						{
							proc p;
							p.ncommands = line->ncommands;
							strcpy(p.state, "Running");
							strcpy(p.command, buf);
							memcpy(p.pid, pidBG, line->ncommands * sizeof(pidBG[0]));
							p.finishedC = 0;
							int aux = 0;
							if (contBGLines > 0)
							{
								while (aux < contBGLines)
								{
									if (bgProcs[aux].finishedC == bgProcs[aux].ncommands)
										break;
									aux += 1;
								}
							}
							bgProcs[aux] = p;
							printf("[%d] %d\n", contBGLines + 1, pid);
							contBGLines+=1;
						}
					}
				}
				else if (pid == 0)	// Código del proceso hijo
				{
					if (line->background == 0) // Configura señales foreground
					{
						signal(SIGINT, SIG_DFL);
						signal(SIGQUIT, SIG_DFL);
					}
					else	 // Configura señales background
					{
						signal(SIGINT, SIG_IGN);
						signal(SIGQUIT, SIG_IGN);
					}
					if (line->ncommands > 1)	// Configura pipes para la entrada y salida estándar en el pipeline
					{
						if (i == 0)	// Primer comando, redirige salida estándar al pipe
						{
							if (dup2(pipefd[i][1], STDOUT_FILENO) == -1)
							{
								perror("dup2 output");
								exit(EXIT_FAILURE);
							}
						}
						if (i == line->ncommands - 1)	// Último comando, redirige entrada estándar desde el pipe anterior
						{
							if (dup2(pipefd[i - 1][0], STDIN_FILENO) == -1)
							{
								perror("dup2 input");
								exit(EXIT_FAILURE);
							}
						}
						if (i > 0 && i < line->ncommands - 1)	 // Comandos intermedios, redirige entrada y salida
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
					for (int j = 0; j < line->ncommands - 1; j++)	// Cierra los pipes en el proceso hijo
					{
						close(pipefd[j][0]);
						close(pipefd[j][1]);
					}
					if (i == 0 && line->redirect_input)	// Redirección de entrada para el primer comando
					{
						int fd_in = open(line->redirect_input, O_RDONLY);
						if (fd_in < 0)
						{
							fprintf(stderr, "%s: Error. Archivo de entrada no válido.\n", line->redirect_input);
							exit(EXIT_FAILURE);
						}
						dup2(fd_in, STDIN_FILENO);
						close(fd_in);
					}
					if (i == line->ncommands - 1)	// Redirección de salida y errores para el último comando
					{
						if (line->redirect_output)	// Redirección de salida
						{
							int fd_out = open(line->redirect_output, O_CREAT | O_WRONLY | O_TRUNC, 0644);
							if (fd_out < 0)
							{
								fprintf(stderr, "%s: Error. Archivo de salida no válido.\n", line->redirect_output);
								exit(EXIT_FAILURE);
							}
							dup2(fd_out, STDOUT_FILENO);
							close(fd_out);
						}
						if (line->redirect_error)	// Redirección de errores
						{
							int fd_out = open(line->redirect_error, O_CREAT | O_WRONLY | O_TRUNC, 0644);
							if (fd_out < 0)
							{
								fprintf(stderr, "%s: Error. Archivo de salida de error no válido.\n", line->redirect_error);
								exit(EXIT_FAILURE);
							}
							dup2(fd_out, STDERR_FILENO);
							close(fd_out);
						}
					}
					execvp(line->commands[i].argv[0], line->commands[i].argv);	// Ejecucion del comando
					fprintf(stderr, "%s: No se encuentra el mandato\n", line->commands[i].argv[0]);
					exit(EXIT_FAILURE);
				}
			}
			for (int i = 0; i < line->ncommands - 1; i++)	// Cierra los pipes en el proceso padre
			{
				close(pipefd[i][0]);
				close(pipefd[i][1]);
			}
			if (line->background == 0)	// Si es un proceso foreground espera a que todos los procesos hijos finalicen
			{
				for (int i = 0; i < line->ncommands; i++)
						waitpid(pidFG[i], NULL, 0);
			}
			for (int i = 0; i < line->ncommands - 1; i++)	// Libera la memoria reservada por los pipes
    		{
        		free(pipefd[i]);
    		}
    		free(pipefd);
		}
	}
	return (EXIT_SUCCESS);
}
