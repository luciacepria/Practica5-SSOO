#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

void print_file_type(mode_t mode)
{
    if (S_ISREG(mode))
        printf("TYPE: Regular file\n");
    else if (S_ISDIR(mode))
        printf("TYPE: Directory\n");
    else if (S_ISLNK(mode))
        printf("TYPE: Symbolic link\n");
    else
        printf("TYPE: Other\n");
}

void print_attributes(struct stat *st)
{
    print_file_type(st->st_mode);

    printf("PERMISSIONS: %04o\n", st->st_mode & 0777);
    printf("HARD LINKS: %ld\n", (long)st->st_nlink);
    printf("SIZE: %ld\n", (long)st->st_size);
    printf("INODE: %ld\n", (long)st->st_ino);
    printf("LAST MODIFICATION: %s", ctime(&st->st_mtime));
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Da el nombre de un documento\n");
        exit(EXIT_FAILURE);
    }

    struct stat st;

    // Primero lstat para saber si es un enlace
    if (lstat(argv[1], &st) == -1)
    {
        perror("lstat");
        exit(EXIT_FAILURE);
    }

    printf("FILE: %s\n", argv[1]);
    print_attributes(&st);

    // Si es un ENLACE:
    if (S_ISLNK(st.st_mode))
    {
        printf("\nFILE WHERE %s LINKS: ", argv[1]);

        // Leer el contenido del enlace (ruta)
        char *buffer = malloc(st.st_size + 1);
        if (buffer == NULL)
        {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        ssize_t len = readlink(argv[1], buffer, st.st_size + 1);
        if (len == -1)
        {
            perror("readlink");
            free(buffer);
            exit(EXIT_FAILURE);
        }

        buffer[len] = '\0';
        printf("%s\n", buffer);

        // Obtener atributos del archivo destino
        printf("TYPE AND ATTRIBUTES OF LINKED FILE:\n");

        struct stat st2;
        if (stat(buffer, &st2) == -1)
        {
            perror("stat");
            free(buffer);
            exit(EXIT_FAILURE);
        }

        print_attributes(&st2);
        free(buffer);
    }

    return 0;
}