#ifndef BOOTS_INCLUDE
#define BOOTS_INCLUDE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

#define BOOTS_IMPLEMENTATION //TODO rm when committing
#define BOOTS_CMD_MAX_NUM_ARGS 100

typedef const char* cstr;

//TODO: This will be needed for simpler path handling
typedef struct {
    cstr *elements;
    int size;
} cstr_array;

cstr_array new_cstr_array(cstr el, ...);

typedef struct {
    cstr_array arr;
    char delimiter;
} boots_path;

boots_path boots_create_path(cstr_array arr);
bool boots_path_exists2(boots_path *path);
void boots_path_concat(boots_path *pre_path, boots_path *post_path);
cstr boots_path_get_full_path(boots_path *pre_path);

int boots_collect_args(cstr_array *out_args, ...);

typedef struct {
    cstr name;
    boots_path* sources;
    int num_sources;
} boots_target;

typedef struct {
    cstr name;
    boots_target* targets;
    int num_targets;
} boots_project;

void boots_set_project(cstr name);
#define PROJECT(name) boots_set_project(name);

int boots_project_add_target(cstr name, cstr_array *sources);
#define ADD_TARGET(name, ...)  \
    do {                                                                            \
        cstr_array sources;                                                           \
        int ret = boots_collect_args(&sources, __VA_ARGS__, (char*) NULL);             \
        if(ret) break;                                                              \
        boots_project_add_target(name, &sources);                                       \
    } while(0);         

int boots_project_get_target(cstr target_name, boots_target **out_target);

//IMPORTANT: Must be the last call in the build recipe
void boots_make(cstr target);
#define MAKE(target) boots_make(target);

//path ops
bool boots_path_exists(cstr path);
#define EXISTS(path) boots_path_exists(path);

void boots_rebuild(int argc, char **argv, cstr src_file);
#define REBUILD_BOOTS(argc, argv)                                       \
    do {                                                                \
        boots_rebuild(argc, argv, __FILE__);                            \
    } while(0)

#define CC(...) boots_cmd("cc", __VA_ARGS__)

void boots_cmd(cstr prog, cstr_array *cmd_args);
#define CMD(prog, ...)\
    do {                                                                            \
        cstr_array cmd_args;                                                           \
        int ret = boots_collect_args(&cmd_args, prog, __VA_ARGS__, (char*) NULL);             \
        if(ret) break;                                                              \
        boots_cmd(prog, &cmd_args);                                       \
    } while(0);         


#ifdef BOOTS_IMPLEMENTATION
boots_project project;
//TODO add implementation here

/* #define BOOTS_COLLECT_ARGS(...) collect_args(0, __VA_ARGS__) */

int boots_collect_args(cstr_array *out_args, ...){
    va_list ptr;
    va_start(ptr, out_args);    
    int num_args = 0;

    char* arg = va_arg(ptr, char*);
    while(arg){
        num_args++;
        arg = va_arg(ptr, char*);
    }
    va_end(ptr);
    if(num_args == 0) return -2;

    char **args = (char**) malloc((num_args+1) * sizeof(char*));
    if(args == NULL) return -1;

    va_start(ptr, out_args);    
    int i = 0;

    arg = va_arg(ptr, char*);
    while(arg){
        if(i >= BOOTS_CMD_MAX_NUM_ARGS - 1) break;
        args[i] = arg;
        i++;
        arg = va_arg(ptr, char*);
    }
    va_end(ptr);

    args[num_args] = (char*) 0; //important for command functions; num_args DOES NOT account for that
    
    cstr_array args_arr = {.elements= (cstr*) args, .size=num_args};
    *out_args = args_arr;

    return 0;
}

void boots_set_project(cstr name) {
    project.name = name;
}

void boots_rebuild(int argc, char **argv, cstr src_file){
    if(argc < 1){
        printf("ERROR: argc < 1\n");
    }
    char *prog = argv[0];
    if(!boots_path_exists(prog)){
        printf("ERROR: prog does not exist\n");
    }

    bool rebuild = false;
    
    struct stat file_stat;
    long last_bin_mod, last_src_mod;
    if(stat(prog, &file_stat) == 0){
        last_bin_mod = file_stat.st_mtim.tv_sec;
    } else {
        printf("ERROR: Can't read file %s", prog);
    }

    if(stat(src_file, &file_stat) == 0){
        last_src_mod = file_stat.st_mtim.tv_sec;
    } else {
        printf("ERROR: Can't read file %s\n", src_file);
    }

    if(last_bin_mod >= last_src_mod) return;

    printf("INFO: Changes in %s: Rebuilding build script\n", src_file);
    
    CMD("gcc", "boots.c", "-o", "boots");
    //replace this process image
    execvp(prog, argv);

}

bool boots_path_exists(cstr path){
    struct stat info;
    
    if (stat(path, &info) != 0) {
        return false;
    }

    if (S_ISREG(info.st_mode) || S_ISDIR(info.st_mode)) {
        return true;
    }
    return false;
}

boots_path boots_create_path(cstr_array array) {
    //TODO
    boots_path path = {.arr=array, .delimiter='/'};
    return path;
}

void boots_cmd(cstr prog, cstr_array *args){
    if(fork() == 0){
        if(execvp(prog, (char * const *) args->elements)){
            printf("some error occured");
        }
    } else {
        wait(NULL);
    }
}

void boots_make(cstr target){
    CMD("gcc", "examples/main.c", "-o", "examples/main")
    //TODO build specific target
}

int boots_project_add_target(cstr name, cstr_array *sources){
    return 0;
}

int boots_project_get_target(cstr target_name, boots_target **out_target){
    int idx = -1;
    for(int i = 0; i <project.num_targets; i++){
        if(strcmp(project.targets[i].name, target_name)){
            idx = i;
        }
    }
    if(idx == -1){
        printf("ERROR: Target %s does not exist", target_name);
        return -1;
    };
    *out_target = &project.targets[idx];
    return 0;
}


#endif //BOOTS_IMPLEMENTATION
#endif //BOOTS_INCLUDE
