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
	char	state[8];
	char	command[512];
	int		ncommands;
	int		pid[512];
	int		finishedC;
} proc;

void	myjobs(proc bgProcs[512], int contProcs)
{
	int cont = 0;
	for (int i = 0; cont < contProcs; i++)
	{
		if (bgProcs[i].finishedC < bgProcs[i].ncommands)
		{
			printf("[%d]+	%s			%s", i+1, bgProcs[i].state, bgProcs[i].command);
			cont += 1;
		}
	}
}

void myfg(proc bgProcs[512], int *contProcs, char *buf)
{
    char	*job_id_str;
	int		job_id;
	proc	*bgProc;
	int		status;
	
	job_id_str = strtok(buf, " ");
    job_id_str = strtok(NULL, " ");
    if (job_id_str == NULL)
	{
        fprintf(stderr, "Uso: fg <job_id>\n");
        return (EXIT_FAILURE);
    }
	job_id = atoi(job_id_str);
    if (job_id <= 0 || job_id > *contProcs)
	{
        fprintf(stderr, "Error: Job [%d] no encontrado\n", job_id);
        return (EXIT_FAILURE);
    }
    bgProc = &bgProcs[job_id - 1];
    printf("Llevando al primer plano el trabajo [%d] con PID %d\n", job_id, bgProc->pid[0]);
    strcpy(bgProc->state, "Foreground");
    waitpid(bgProc->pid[0], &status, 0);
    if (WIFEXITED(status))
        strcpy(bgProc->state, "Done");
    else if (WIFSIGNALED(status))
        strcpy(bgProc->state, "Terminado por señal");
    printf("[%d]+ %s   %s\n", job_id, bgProc->state, bgProc->command);
    (*contProcs)--;
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

int updateBG(int contBGLines, proc *bgProcs)
{
	int numFinished = 0;
	for (int i = 0; i < contBGLines; i++)
	{
		if (bgProcs[i].finishedC != bgProcs[i].ncommands)
		{
			int j = bgProcs[i].finishedC;
			while (j < bgProcs[i].ncommands)
			{
				pid_t res = waitpid(bgProcs[i].pid[j], NULL, WNOHANG);
				if (res > 0)
				{
					bgProcs[i].finishedC += 1;
				}
				j += 1;
			}
			if (bgProcs[i].finishedC == bgProcs[i].ncommands)
			{
				strcpy(bgProcs[i].state, "Done");
				printf("[%d]+ %s		%s", contBGLines, bgProcs[i].state, bgProcs[i].command);
				numFinished += 1;
			}
		}
	}
	return numFinished;
}

int	main(void)
{
	char	buf[BUFFER_SIZE];
	tline	*line;
	pid_t	pid;
	int		i;

	int pidFG[512];
	int pidBG[512];
	int contBGLines = 0;
	proc	bgProcs[512];

	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	while (1)
	{
		contBGLines -= updateBG(contBGLines, bgProcs);
		printf("msh> ");
		fgets(buf, BUFFER_SIZE, stdin);
		
		if (buf[0] == '\n')
			continue;
		
		line = tokenize(buf);

		if (line == NULL)
			continue;
		
		if (strcmp(line->commands[0].argv[0], "cd") == 0)
			mycd(line->commands[0].argv[1]);
		else if (strcmp(line->commands[0].argv[0], "jobs") == 0)
			myjobs(bgProcs, contBGLines);
		else if (strcmp(line->commands[0].argv[0], "fg") == 0)
			myfg(bgProcs, &contBGLines, buf);
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
				else if (pid > 0)
				{
					if (line->background == 0)
						pidFG[i] = pid;
					else
					{
						pidBG[i] = pid;
						if (i == line->ncommands - 1)
						{
							proc p;
							p.ncommands = line->ncommands;
							strcpy(p.state, "Running");
							strcpy(p.command, buf);
							memcpy(p.pid, pidBG, line->ncommands*sizeof(pidBG[0]));
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
							fprintf(stderr, "%s: Error. Archivo de entrada no válido.\n", line->redirect_input);
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
								fprintf(stderr, "%s: Error. Archivo de salida no válido.\n", line->redirect_output);
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
								fprintf(stderr, "%s: Error. Archivo de salida de error no válido.\n", line->redirect_error);
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
			}
			for (int i = 0; i < line->ncommands - 1; i++)
			{
				close(pipefd[i][0]);
				close(pipefd[i][1]);
			}
			if (line->background == 0)
			{
				for (int i = 0; i < line->ncommands; i++)
						waitpid(pidFG[i], NULL, 0);
			}
		}
	}
	return (EXIT_SUCCESS);
}
