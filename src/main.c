#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define MAX_RESULTS 3

typedef struct {
    char path[1024];
    int line_num;
    char content[1024];
} SearchResult;

SearchResult last_results[MAX_RESULTS];
int result_count = 0;

void add_result(const char* path, int line_num, const char* content) {
    if (result_count < MAX_RESULTS) {
        strncpy(last_results[result_count].path, path, sizeof(last_results[0].path)-1);
        last_results[result_count].line_num = line_num;
        strncpy(last_results[result_count].content, content, sizeof(last_results[0].content)-1);
        result_count++;
    } else {
        for (int i = 0; i < MAX_RESULTS-1; i++) {
            last_results[i] = last_results[i+1];
        }
        strncpy(last_results[MAX_RESULTS-1].path, path, sizeof(last_results[0].path)-1);
        last_results[MAX_RESULTS-1].line_num = line_num;
        strncpy(last_results[MAX_RESULTS-1].content, content, sizeof(last_results[0].content)-1);
    }
}

void print_last_results() {
    for (int i = 0; i < result_count; i++) {
        printf("Путь: %s: %d: %s", 
               last_results[i].path, 
               last_results[i].line_num, 
               last_results[i].content);
    }
}

void search_in_file(const char *file_path, const char *word) {
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        perror("Ошибка открытия файла");
        return;
    }

    char line[1024];
    int line_num = 0;

    while (fgets(line, sizeof(line), file) != NULL) {
        line_num++;
        if (strstr(line, word) != NULL) {
            add_result(file_path, line_num, line);
        }
    }

    fclose(file);
}

void search_in_directory(const char *dir_path, const char *word) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("Ошибка открытия директории");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);

        struct stat statbuf;
        if (lstat(full_path, &statbuf) == -1) {
            perror("Ошибка получения информации о файле");
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            search_in_directory(full_path, word);
        } else if (S_ISREG(statbuf.st_mode)) {
            search_in_file(full_path, word);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Использование: %s <слово> [директория]\n", argv[0]);
        fprintf(stderr, "По умолчанию директория: ~/files\n");
        return EXIT_FAILURE;
    }

    const char *word = argv[1];
    const char *default_dir = "~/files";
    char *dir_path = (argc > 2) ? argv[2] : (char*)default_dir;

    if (dir_path[0] == '~') {
        char *home = getenv("HOME");
        if (home == NULL) {
            fprintf(stderr, "Ошибка: не удалось определить домашнюю директорию\n");
            return EXIT_FAILURE;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s%s", home, dir_path + 1);
        dir_path = full_path;
    }

    struct stat statbuf;
    if (stat(dir_path, &statbuf) == -1) {
        perror("Ошибка доступа к директории");
        return EXIT_FAILURE;
    }

    if (!S_ISDIR(statbuf.st_mode)) {
        fprintf(stderr, "Ошибка: указанный путь не является директорией\n");
        return EXIT_FAILURE;
    }

    printf("Поиск слова '%s' в директории %s...\n", word, dir_path);
    search_in_directory(dir_path, word);
    print_last_results();

    return EXIT_SUCCESS;
}
