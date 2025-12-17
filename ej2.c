#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

int total_subdirs = 0;


void recorre_dir( char *ruta, int level)
{
    char *fichero;
    char *link_target;
    struct stat datos;
    DIR *dir;
    struct dirent *dp;
    int estado;

    
    if ((dir=opendir(ruta)) == NULL)
    { 
        perror("opendir"); 
        return;
    }

    for (int i = 0; i < level; i++) printf("   ");
    printf("Directory %s:\n", ruta);

    while ((dp=readdir(dir)) != NULL)
    {   
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue;
            
        fichero= malloc(strlen(ruta)+strlen(dp->d_name)+2);
        sprintf(fichero,"%s/%s",ruta,dp->d_name);
        if ((lstat(fichero,&datos)) != 0)
        { 
            perror("stat"); 
            continue; 
        }
        
        switch (datos.st_mode & S_IFMT) {
            case S_IFDIR:
                total_subdirs++;
                recorre_dir(fichero, level + 1);
                break;

            case S_IFLNK:
                for (int i = 0; i <= level; i++) printf("   ");
                link_target = malloc(datos.st_size + 1);
                ssize_t len = readlink(fichero, link_target, datos.st_size + 1);
                if (len != -1) {
                    link_target[len] = '\0';
                    printf("%s -> %s\n", fichero, link_target);
                } else {
                    perror("readlink");
                }
                free(link_target);
                break;

            case S_IFREG:
                for (int i = 0; i <= level; i++) printf("   ");
                printf("%s\n", fichero);
                break;
        }
        free (fichero);
    }
    closedir(dir);

}

int main(int argc, char *argv[])
{
    struct stat datos;

    if(argc != 2){
        printf("Debes introducir el directorio de inicio.\n");
        exit(-1);
    }
    if (lstat(argv[1], &datos) == -1) {
        if (errno == ENOENT) {
            if (mkdir(argv[1], 0755) == -1) {
                perror("Error creando directorio");
                exit(-1);
            }
            printf("Directorio %s creado.\n", argv[1]);
            exit(0);
        } else {
            perror("Error lstat");
            exit(-1);
        }
    }

    // Verificar que sea un directorio
    if (!S_ISDIR(datos.st_mode)) {
        printf("%s no es un directorio\n", argv[1]);
        exit(-1);
    }

    //Llamada a la función recursiva
    printf("Tree of directory %s:\n", argv[1]);
    recorre_dir(argv[1], 0);
    printf("\nNumber of directories in %s: %d\n", argv[1], total_subdirs);

    
}