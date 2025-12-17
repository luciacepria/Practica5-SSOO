#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>

/*
 * JUSTIFICACIÓN DE DISEÑO:
 * 1. exec: Se usa 'execlp' porque conocemos los argumentos (lista) y 
 * queremos usar el PATH (p).
 * 2. Procesos: Se generan secuencialmente hijos para 'cp' y 'diff'.
 * 3. Redirección: Se usa close(1) + dup() para cumplir la restricción de no usar dup2.
 */

void copiar_fichero(const char *origen, const char *destino) {
    pid_t pid = fork();

    if (pid == -1) {
        perror("Error fork cp");
        return -1;
    }
    if (pid == 0) {
        // --- HIJO (CP) ---
        execlp("cp", "cp", origen, destino, NULL);
        perror("Error ejecutando cp");
        exit(-1); 
    } 
    // --- PADRE ---
    wait(NULL); // Esperamos a que termine la copia
    
}

void comparar_fichero(const char *origen, const char *destino, int log_fd) {
    int pipe_fd[2];
    
    
    pid_t pid = fork();
    if (pid == -1) {
        perror("Error fork diff");
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        return ;
    }
    if (pid == 0) {
        // --- HIJO (DIFF) ---
        close(pipe_fd[0]); // Cerramos lectura

        // 1. Cerramos salida estándar (fd 1)
        close(1); 
        // 2. Duplicamos el pipe de escritura. 
        if (dup(pipe_fd[1]) == -1) {
            perror("dup");
            exit(-1);
        }
        // 3. Cerramos el fd original del pipe 
        close(pipe_fd[1]);

        execlp("diff", "diff", origen, destino, NULL);
        
        perror("Error ejecutando diff");
        exit(-1);
    } 
    // --- PADRE ---
    close(pipe_fd[1]); // Cerramos escritura
    char c; // <--- Variable simple, no un array
    
    // Leemos 1 byte cada vez (el último parámetro es 1)
    while (read(pipe_fd[0], &c, 1) > 0) {
        // Escribimos ese byte en pantalla
        write(1, &c, 1);
        // Escribimos ese byte en el log
        write(log_fd, &c, 1);
    }

    close(pipe_fd[0]);
    wait(NULL);
}

void procesar_directorio(char *dir_origen, char *dir_destino, int log_fd) {
    DIR *dir;
    struct dirent *dp;
    struct stat datos;
    char *ruta_origen, *ruta_destino;

    if ((dir = opendir(dir_origen)) == NULL) {
        perror("Error opendir");
        return;
    }

    while ((dp = readdir(dir)) != NULL) {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue;

        ruta_origen = malloc(strlen(dir_origen) + strlen(dp->d_name) + 2);
        sprintf(ruta_origen, "%s/%s", dir_origen, dp->d_name);

        if (lstat(ruta_origen, &datos) != 0) {
            perror("lstat");
            free(ruta_origen);
            continue;
        }

        if (S_ISREG(datos.st_mode)) {
            ruta_destino = malloc(strlen(dir_destino) + strlen(dp->d_name) + 2);
            sprintf(ruta_destino, "%s/%s", dir_destino, dp->d_name);

            // Copiar
            copiar_fichero(ruta_origen, ruta_destino);
            // Comparar
            comparar_fichero(ruta_origen, ruta_destino, log_fd);
            
            free(ruta_destino);
        }
        free(ruta_origen);
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    struct stat datos;
    int log_fd;

    if (argc != 4) {
        char msg[] = "Uso incorrecto argumentos\n";
        write(2, msg, strlen(msg));
        exit(-1);
    }

    if (lstat(argv[1], &datos) == -1 || !S_ISDIR(datos.st_mode)) {
        perror("Error origen");
        exit(-1);
    }
    if (lstat(argv[2], &datos) != -1) {
        perror("Error: Destino ya existe");
        exit(-1);
    }
    if (mkdir(argv[2], 0777) == -1) {
        perror("mkdir");
        exit(-1);
    }
    log_fd = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (log_fd == -1) {
        perror("open log");
        exit(-1);
    }
    procesar_directorio(argv[1], argv[2], log_fd);
    close(log_fd);
    return 0;
}