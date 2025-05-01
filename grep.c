#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <libgen.h>
#include <regex.h>
#include <stdbool.h>
#define MAX_PATH 1024

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

bool regex_flag = 0;
typedef struct {
    char filepath[MAX_PATH];
    char pattern[256];
} thread_arg;

void* search_file_with_regex(void* arg) {
    regex_t regex;
    thread_arg* data = (thread_arg*)arg;
    FILE* file = fopen(data->filepath, "r");
    if (!file) return NULL;

    char line[1024];
    int line_num = 1;

    if (regcomp(&regex, data->pattern, REG_EXTENDED)) {
        fprintf(stderr, "Could not compile regex\n");
    }

    while (fgets(line, sizeof(line), file)) {
	        int result = regexec(&regex, line, 0, NULL, 0);
    if (!result) {
            pthread_mutex_lock(&print_mutex);
            printf("%s:%d: %s", data->filepath, line_num, line);
            pthread_mutex_unlock(&print_mutex);
        }
        line_num++;
    }
    fclose(file);
    free(arg);
    return NULL;
}


void* search_file(void* arg) {
    thread_arg* data = (thread_arg*)arg;
    FILE* file = fopen(data->filepath, "r");
    if (!file) return NULL;

    char line[1024];
    int line_num = 1;
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, data->pattern)) {
            pthread_mutex_lock(&print_mutex);
            printf("%s:%d: %s", data->filepath, line_num, line);
            pthread_mutex_unlock(&print_mutex);
        }
        line_num++;
    }
    fclose(file);
    free(arg);
    return NULL;
}


void search_dir(const char* dirname, const char* pattern) {
    DIR* dir = opendir(dirname);
    if (!dir) return;

    struct dirent* entry;
    struct stat statbuf;
    char path[MAX_PATH];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(path, MAX_PATH, "%s/%s", dirname, entry->d_name);
        if (stat(path, &statbuf) == -1) continue;

        if (S_ISDIR(statbuf.st_mode)) {
            search_dir(path, pattern);
        } else if (S_ISREG(statbuf.st_mode)) {
            pthread_t tid;
            thread_arg* arg = malloc(sizeof(thread_arg));
            strncpy(arg->filepath, path, MAX_PATH);
            strncpy(arg->pattern, pattern, 256);
	    if(regex_flag==1)
	    {
	    	pthread_create(&tid, NULL, search_file_with_regex, arg);
	    }
	    else
	    {
            	pthread_create(&tid, NULL, search_file, arg);
	    }
            pthread_detach(tid);  // Fire-and-forget
        }
    }
    closedir(dir);
}

int main(int argc, char* argv[]) {
	char cwd[PATH_MAX];
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <directory> <pattern>\n", argv[0]);
        return 1;
	}
    	
    	char *str = argv[2];
	size_t len = strlen(str);
	if(str[len - 1] == '*')
	{
		regex_flag = 1;
    		if (len > 0) 
        	str[len - 1] = '\0';  // Remove the last '*'
		strcpy(argv[2],str);
    	}
	
	if((strcmp(argv[1],"-r")==0))
	{	
		search_dir(".", argv[2]);
	}
	else
	{
    		search_dir(argv[1], argv[2]);
	}
    sleep(2); // Give threads time to finish for demo
    return 0;
}

