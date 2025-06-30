 #include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include "parser.h"
#include <signal.h>

#define READ_END    0
#define WRITE_END   1

char cwd[1024];
int bg_commands[100]; // Array para almacenar los comandos en segundo plano
char comandos_ejecutandose[100][1024]; //Array que almacena los comandos ejecutnadose en segundo plano
char buf[1024];
int num_hijos = 0;
pid_t pid;
tline *line;

void cd_command();
void jobs_command();
void fg_command();
void execute();

int main(void) {
    while (1) {
        printf("msh > ");
        fgets(buf, 1024, stdin);

        if (strcmp(buf, "\n") == 0) {
            continue;
        }
        line = tokenize(buf);

        if (strcmp(line->commands[0].argv[0], "cd") == 0) {
            cd_command();
            continue;
        } else if (strcmp(line->commands[0].argv[0], "salir") == 0) {
            exit(0);
        } else if (strcmp(line->commands[0].argv[0], "jobs") == 0) {
            jobs_command();
        } else if(strcmp(line->commands[0].argv[0], "fg")==0){

            fg_command();
        }
        else{
          execute();
        }
    }
    return 0;
}

void cd_command(){

  if(line->commands[0].argc > 1) {
      if (chdir(line->commands[0].argv[1]) == 0) {
          if (getcwd(cwd, sizeof(cwd)) != NULL) {
              printf("%s\n",cwd);
          } else {
             printf("No se puede encontrar el directorio actual de trabajo %s\n", strerror(errno));
          }
      } else {
          printf("No es posible cambiar de directorio o el directorio no existe\n");
      }
  } else {
      char* home = getenv("HOME");
      if (chdir(home) == 0) {
          if (getcwd(cwd, sizeof(cwd)) != NULL) {
              printf("%s\n",cwd);
          } else {
              printf("No se puede encontrar el directorio actual de trabajo %s\n", strerror(errno));
          }
      } else {
            printf("No es posible cambiar de directorio o el directorio no existe\n");
      }
  }

}
void jobs_command(){

  int pid_finalizado;
  int status;

  for (int i = 0; i < num_hijos; i++) {
      pid_finalizado = waitpid(-1, &status, WNOHANG);
      if(pid_finalizado > 0) {
          printf("[%d]+ Hecho  %s", i + 1, comandos_ejecutandose[i]);
          //Iteramos sobre los hijos si algun hijo coincide con ese pid entonces se elimina o se pone un numero negativo por ejemplo -1
          for (int j = 0; j < num_hijos; j++) {
              if(bg_commands[j] == pid_finalizado) {
                  bg_commands[j] = -1;
              }
          }
      }
  }

  for (int i = 0; i < num_hijos; i++) {
      if (bg_commands[i] != -1) {
          //Para poder ejecutar el comando corrrespondiente asociamos con la posicion del hijo correspondiente
          printf("[%d]+ Ejecutando  %s", i + 1, comandos_ejecutandose[i]);
      }
  }
}
void fg_command(){
  //Si el numero de argumentos es dos

  if(line->commands[0].argc<=2){
      if(line->commands[0].argc==1){
          if(num_hijos>0){
              //Para poder restaurar el trabajo primero debemos comprobar el ultimo mandato introducido es decir el mandato que corresponde al hijos
              //Precondicion: dicha posicion debe ser distinta de -1 pues quiere decir que el hijo todavia no ha finalizado , caso contrario ya no esta ejecutnadose
              //en primer plano
              //Ahora esperamos que el hijo termine
              waitpid(bg_commands[num_hijos-1], NULL, 0);
              //Tenemos ahora que poner el valor -1 en dicho pid pues el hijo ya no se encuentra en segundo plano
              bg_commands[num_hijos-1] = -1;



          } else {
              printf("No existe ese trabajo\n");
          }
      }
      else if(line->commands[0].argc==2){
          if(num_hijos>0){
              //Para poder restaurar el trabajo primero debemos comprobar el ultimo mandato introducido es decir el mandato que corresponde al hijos
              //Precondicion: en dicha posicion el pid debe ser distinta de -1 pues quiere decir que el hijo todavia no ha finalizado , caso contrario ya no esta ejecutnadose
              //en primer plano
              //Ahora esperamos que el hijo termine

              int posicion_proceso = atoi(line->commands[0].argv[1]);
              if (posicion_proceso > 0) {
                  if (bg_commands[posicion_proceso - 1] == -1) {
                      printf("El trabajo especificado no existe\n");
                  } else {
                      printf("Restaurando el proceso %d al primer plano\n",posicion_proceso);
                      waitpid(bg_commands[posicion_proceso - 1], NULL, 0);
                      //Tenemos ahora que poner el valor -1 en dicho pid pues el hijo ya no se encuentra en segundo plano
                      bg_commands[posicion_proceso-1] = -1;


                  }
              } else {
                  printf("Número de trabajo inválido\n");
              }

          }
      }
  }


}
void execute(){
  int fdinput, fdoutput, fderror;
  int pipes[line->ncommands -1][2];

  // Crear todas las tuberías necesarias
  for (int i = 0; i < line->ncommands - 1; i++) {
      if (pipe(pipes[i]) == -1) {
          fprintf(stderr, "Error al crear el pipes\n");
      }
  }

  for (int i = 0; i < line->ncommands; i++) {
      pid = fork();
      if (pid < 0) {
          fprintf(stderr, "Error en el fork\n");
      }

      if (pid == 0) { //Si nos encontramos en el proceso hijo
          /*SI lo que se lee en el buffer contiene una redirección de entrada hay que controlar que solo se puead hacer
          si quien lo ejecuta es el primer hijo(condición impuesta en la practica)*/

          if (line->redirect_input != NULL && i == 0) {
              if ((fdinput = open(line->redirect_input, O_RDONLY)) < 0) {
                  printf("Error al abrir el fichero\n");
                  exit(1);
              }

              if(dup2(fdinput,STDIN_FILENO)<0){
                  printf("Error duplicar el descriptor\n");
              }
              close(fdinput);
          }

          /*Si lo que se lee en el buffer contiene una redirecion de salida o de error hay que controlar que solo se pueda hacer
          si quien lo ejecuta es el ultimo hijo(condicion impuesta en la practica)*/
          if (line->redirect_output != NULL && i == line->ncommands - 1) {
              if ((fdoutput = open(line->redirect_output, O_CREAT | O_TRUNC | O_WRONLY, 0666)) < 0) {
                  printf("Error al abrir el fichero\n");
                  exit(1);
              }

              if(dup2(fdoutput, STDOUT_FILENO)<0){
                  printf("Error duplicar el descriptor\n");
              }
              close(fdoutput);
          };

          if (line->redirect_error != NULL && i == 0) {
              if ((fderror = open(line->redirect_error, O_CREAT | O_TRUNC | O_WRONLY, 0666)) < 0) {
                  printf("Error al abrir el fichero\n");
                  exit(1);
              }

              if(dup2(fderror,STDERR_FILENO)<0){
                  printf("Error duplicar el descriptor\n");
              }
              close(fderror);
          }

          if (i != 0) {
              dup2(pipes[i - 1][0], STDIN_FILENO);
              close(pipes[i - 1][0]);
          }

          if (i != line->ncommands - 1) {
              dup2(pipes[i][1], STDOUT_FILENO);
              close(pipes[i][1]);
          }

          for (int j = 0; j < line->ncommands - 1; j++) {
              close(pipes[j][0]);
              close(pipes[j][1]);
          }

          if (line->commands[i].filename != NULL) {
              execvp(line->commands[i].filename, line->commands[i].argv);
          } else {
              fprintf(stderr, "No se puede ejecutar el mandato porque no existe\n");
              exit(1);
          }
      }
  }

  // Cerrar todos los descriptores de archivo en el proceso padre
  for (int i = 0; i < line->ncommands - 1; i++) {
      close(pipes[i][0]);
      close(pipes[i][1]);
  }

  if(line->background != 1) {
      // Esperar a que todos los hijos terminen
      waitpid(pid, NULL, 0);
  } else {
      printf("[%d]\t%d \n",num_hijos+1,pid);
      bg_commands[num_hijos] = pid;
      strcpy(comandos_ejecutandose[num_hijos], buf);
      num_hijos++;
  }
}
