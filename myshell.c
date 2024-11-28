#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "parser.h"

void mycd(char *path, int len) {
    char *dir;
	char buffer[512];

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

int main(){
    char buf[1024];
	tline * line;
	int i,j;

	printf("msh> ");	
	while (fgets(buf, 1024, stdin)) {
		
		line = tokenize(buf);
        pid_t pid;
		int pip_des[2];

        if (line==NULL) {
			continue;
		}
        
        else if (line->ncommands == 1) {
            if (strcmp(line->commands[0].argv[0], "cd") == 0)
                mycd(line->commands[0].argv[1], line->commands[0].argc - 1);

            else if (line->commands->filename == NULL)
                printf("%s: No se encuentra el mandato\n", line->commands[0].argv[0]);

            else {
                pid = fork();
                if (pid == 0) {
                    if (line->redirect_input != NULL) {
                        int fd_in = open(line->redirect_input, O_RDONLY);
                        if (fd_in < 0) {
                            printf("%s: Error. Archivo de entrada no válido.\n", line->redirect_input);
                            exit(1);
                        }
                        dup2(fd_in, STDIN_FILENO);
                        close(fd_in);
                    }
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
                    execvp(line->commands[0].filename, line->commands[0].argv);
                    fprintf(stderr,"Se ha producido un error.\n");
                    exit(1);
                }
                else {
                    wait(NULL);
                }
            }

        }

        else if (line->ncommands == 2){
            if (line->commands[0].filename == NULL)
                printf("%s: No se encuentra el mandato\n", line->commands[0].argv[0]);
            if (line->commands[1].filename == NULL)
                printf("%s: No se encuentra el mandato\n", line->commands[1].argv[0]);

            else if (line->commands[0].filename != NULL && line->commands[1].filename != NULL) {
                pid = fork();
                if (pid == 0) {
                    pipe(pip_des);
                    pid = fork();
                    if (pid == 0){
                        if (line->redirect_input != NULL) {
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
        }
		printf("msh> ");	
	}
	return 0;
}


/*for (i=0; i<line->ncommands; i++) {
                    printf("orden %d (%s):\n", i, line->commands[i].filename);
                    for (j=0; j<line->commands[i].argc; j++) {
                        printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
                    }
                }*/
                //execvp(line->commands->argv[0], line->commands->argv);
                //fprintf(stderr,"Se ha producido un error.\n");