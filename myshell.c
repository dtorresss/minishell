#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "parser.h"

int main(){
    char buf[1024];
    char bufferPipe[1024];
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
        
        if (line->ncommands == 1) {
            pid = fork();
            if (pid == 0){
                if (line->redirect_output != NULL) {
                        int fd_out = open(line->redirect_output, O_CREAT | O_WRONLY | O_TRUNC, 0644);
                        if (fd_out < 0) {
                            printf("Error al abrir el archivo de salida");
                            exit(1);
                        }
                        dup2(fd_out, STDOUT_FILENO);
                        close(fd_out);
                    }
                if (line->redirect_input != NULL) {
                        int fd_in = open(line->redirect_input, O_RDONLY);
                        if (fd_in < 0) {
                            printf("Error al abrir el archivo de entrada\n");
                            exit(1);
                        }
                        dup2(fd_in, STDIN_FILENO);
                        close(fd_in);
                    }
                execvp(line->commands->argv[0], line->commands->argv);
                fprintf(stderr,"Se ha producido un error.\n");
                exit(1);
            }
            else {
                wait(NULL);
            }

        }

        else {
            pid = fork();
            if (pid == 0) {
                pipe(pip_des);
                pid = fork();
                if (pid == 0){
                    if (line->redirect_input != NULL) {
                        int fd_in = open(line->redirect_input, O_RDONLY);
                        if (fd_in < 0) {
                            printf("Error al abrir el archivo de entrada\n");
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
                            printf("Error al abrir el archivo de salida");
                            exit(1);
                        }
                        dup2(fd_out, STDOUT_FILENO);
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
            else {
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