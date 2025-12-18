#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>

void process_subdirectory(const char *dirpath);

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Da un directorio\n");
        return 1;
    }

    process_subdirectory(argv[1]);
    return 0;
}

void process_subdirectory(const char *dirpath)
{
    DIR *dir = opendir(dirpath);
    if (!dir)
    {
        perror("opendir");
        return;
    }
    int pipe1[2], pipe2[2];
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1)
    {
        perror("pipe");
        closedir(dir);
        return;
    }

    pid_t pidB = fork();
    if (pidB == 0)
    {
        dup2(pipe1[1], STDOUT_FILENO);
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        execlp("ls", "ls", "-l", dirpath, NULL);
        perror("execlp ls");
        exit(1);
    }

    pid_t pidC = fork();
    if (pidC == 0)
    {
        dup2(pipe1[0], STDIN_FILENO);
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        execlp("tail", "tail", "-n", "+2", NULL);
        perror("execlp tail");
        exit(1);
    }

    pid_t pidD = fork();
    if (pidD == 0)
    {
        dup2(pipe2[0], STDIN_FILENO);
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        execlp("wc", "wc", "-l", NULL);
        perror("execlp wc");
        exit(1);
    }

    // Padre: cerrar extremos no usados
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[1]);

    // Esperar a que al menos un hijo termine (normalmente wc)
    wait(NULL);

    // Leer resultado de wc
    char buffer[128];
    int n = read(pipe2[0], buffer, sizeof(buffer) - 1);
    if (n > 0)
    {
        buffer[n] = '\0';
        printf("Files and directories of %s: %d\n", dirpath, atoi(buffer));
    }
    close(pipe2[0]);

    // Esperar a los otros hijos restantes
    wait(NULL);
    wait(NULL);

    // Recursión para subdirectorios
    struct dirent *entry;
    rewinddir(dir);
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);

        struct stat st;
        if (lstat(fullpath, &st) == -1)
            continue;

        if (S_ISDIR(st.st_mode))
        {
            process_subdirectory(fullpath);
        }
    }

    closedir(dir);
}