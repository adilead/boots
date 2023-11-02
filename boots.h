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

#define ASCII_ART ""\
    "                   _                 _       \n" \
    "        ____      | |               | |      \n" \
    "        \\  _|__   | |__   ___   ___ | |_ ___ \n" \
    "       __)|   /   | '_ \\ / _ \\ / _ \\| __/ __|\n" \
    "      (___|  (__  | |_) | (_) | (_) | |_\\__ \\\n" \
    "          (_-___) |_.__/ \\___/ \\___/ \\__|___/\n"

/* #define BOOTS_IMPLEMENTATION //TODO rm when committing */
#define BOOTS_CMD_MAX_NUM_ARGS 100
#define BOOTS_MAX_NUM_TARGETS 100

typedef const char* cstr;

//TODO: This will be needed for simpler path handling
typedef struct {
    cstr *elements;
    int size;
} cstr_array;

cstr_array new_cstr_array(cstr el, ...);
void boots_cstr_array_append(cstr_array *array, cstr_array *const addendum);

typedef struct {
    cstr_array arr;
    char delimiter;
} boots_path;

boots_path boots_create_path(cstr_array arr);
bool boots_path_exists2(boots_path *path);
void boots_path_concat(boots_path *pre_path, boots_path *post_path);
cstr boots_path_get_full_path(boots_path *pre_path);

int boots_collect_args(cstr_array *out_args, ...);

enum boots_target_type {
    LIBRARY,
    EXECUTABLE
};

typedef struct {
    enum boots_target_type target_type;
    cstr name;
    cstr_array sources;
    cstr_array include_dirs;
} boots_target;

typedef struct {
    cstr name;
    boots_target targets[BOOTS_MAX_NUM_TARGETS];
    int num_targets;
} boots_project;

void boots_set_project(cstr name);
#define PROJECT(name) boots_set_project(name);

cstr boots_convert_path(int is_relative, ...);
# define RPATH(...) boots_convert_path(true, __VA_ARGS__, (char* ) NULL)
# define APATH(...) boots_convert_path(false, __VA_ARGS__, (char* ) NULL)

cstr boots_convert_path(int is_relative, ...){
    va_list ptr;
    va_start(ptr, is_relative);    
    int num_args = 0;
    int path_len = 0;

    char* arg = va_arg(ptr, char*);
    while(arg){
        num_args++;
        arg = va_arg(ptr, char*);
        if(arg) path_len += strlen(arg);
    }
    va_end(ptr);
    if(num_args == 0) return NULL;

    //TODO implement absolute paths
    char *path = (char*) malloc((path_len+1+num_args-1) * sizeof(char));
    if(path == NULL) return NULL;

    va_start(ptr, is_relative);    
    int i = 0;
    int arg_i = 0;

    arg = va_arg(ptr, char*);
    while(arg){
        memcpy(&path[i], arg, strlen(arg));
        i += strlen(arg);
        arg_i++;
        if(arg_i < num_args) path[i++] = '/';
        arg = va_arg(ptr, char*);
    }
    va_end(ptr);
    path[i] = '\0';
    return path;
}

int boots_project_add_target(cstr name, bool is_lib, cstr_array *sources);
#define ADD_TARGET(name, is_lib, ...)  \
    do {                                                                            \
        cstr_array sources;                                                           \
        int ret = boots_collect_args(&sources, __VA_ARGS__, (char*) NULL);             \
        if(ret) break;                                                              \
        boots_project_add_target(name, is_lib, &sources);                                       \
    } while(0);         

#define ADD_EXECUTABLE(name, ...) ADD_TARGET(name, false, __VA_ARGS__)
#define ADD_LIBRARY(name, ...) ADD_TARGET(name, true, __VA_ARGS__)
int boots_project_get_target(cstr target_name, boots_target **out_target);

void boots_target_add_include_dir(cstr name, cstr_array *dirs);
#define TARGET_ADD_INCLUDE_DIR(name, ...)\
    do {\
        cstr_array dirs;\
        int ret = boots_collect_args(&dirs, __VA_ARGS__, (char*) NULL);      \
        if(ret) break;                                                          \
        boots_target_add_include_dir(name, &dirs);                       \
    } while(0);         

//IMPORTANT: Must be the last call in the build recipe
void boots_make(cstr target);
#define MAKE(target) boots_make(target);

//path ops
bool boots_path_exists(cstr path);
#define EXISTS(path) boots_path_exists(path);

void boots_rebuild(int argc, char **argv, cstr src_file);
#define REBUILD_BOOTS(argc, argv) boots_rebuild(argc, argv, __FILE__);

#define CC(...) boots_cmd("cc", __VA_ARGS__)

void boots_cmd(cstr prog, cstr_array *cmd_args);
#define CMD(prog, ...)\
    do {                                                                            \
        cstr_array cmd_args;                                                           \
        int ret = boots_collect_args(&cmd_args, prog, __VA_ARGS__, (char*) NULL);             \
        if(ret) break;                                                              \
        boots_cmd(prog, &cmd_args);                                       \
    } while(0);         

int boots_check_last_cmd_status();

#ifdef BOOTS_IMPLEMENTATION
void boots_cstr_array_append(cstr_array *array, cstr_array *const addendum){
    size_t cstr_size = sizeof(*(array->elements));
    if(array->size == 0){
        array->elements = (cstr*) malloc(cstr_size*addendum->size);
        memcpy(array->elements, addendum->elements, cstr_size*addendum->size);
        array->size = addendum->size;
    } else {
        array->elements = (cstr*) realloc(array->elements, cstr_size*(array->size + addendum->size));
        memcpy(array->elements + array->size, addendum->elements, cstr_size*addendum->size);
        array->size += addendum->size;
    }
}

boots_project project;
int boots_cmd_status = 0;

int boots_check_last_cmd_status(){
    return boots_cmd_status;
}
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

    if(last_bin_mod >= last_src_mod){
        cstr art = ASCII_ART;
        printf("%s\n", art);
        return;
    } 

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
            printf("ERROR: some error occured executing the command %s\n", prog);
        }
    } else {
        wait(&boots_cmd_status);
    }
}

void boots_make(cstr target_name){
    boots_target *out_target;
    boots_project_get_target(target_name, &out_target);
    int num_sources = out_target->sources.size;
    int num_include_dirs = out_target->include_dirs.size;
    int num_args = num_sources + (num_include_dirs+1) + 3;
    cstr* args = (cstr*) malloc((num_args+1) * sizeof(char*));
    uint idx = 0;
    args[idx++] = "gcc";
    memcpy(&args[idx], out_target->sources.elements, num_sources*sizeof(char*));idx+=num_sources;
    args[idx++] = "-o";
    args[idx++] = out_target->name;
    if(num_include_dirs > 0){
        args[idx++] = "-I";
        memcpy(&args[idx], out_target->include_dirs.elements, num_include_dirs*sizeof(char*));idx+=num_include_dirs;
    }
    args[idx] = NULL;

    printf("INFO: Building %s\n", out_target->name);
    cstr_array cmd_args = {.elements=args, .size=num_args};
    boots_cmd("gcc", &cmd_args);
    free(args);
}

int boots_project_add_target(cstr name, bool is_lib, cstr_array *sources){
    for(int i=0; i<sources->size; i++){
        printf("INFO: Adding %s to %s\n", sources->elements[i], name);
    }
    if (project.num_targets >= BOOTS_MAX_NUM_TARGETS) {
        return -1;
    }
    boots_target target = {.target_type= is_lib ? LIBRARY : EXECUTABLE, .name=name, .sources=*sources, .include_dirs={.elements=NULL, .size=0}};
    project.targets[project.num_targets++] = target;
    return 0;
}

int boots_project_get_target(cstr target_name, boots_target **out_target){
    int idx = -1;
    for(int i = 0; i <project.num_targets; i++){
        if(strcmp(project.targets[i].name, target_name) == 0){
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

void boots_target_add_include_dir(cstr target_name, cstr_array *dirs){
    boots_target *out_target;
    if(boots_project_get_target(target_name, &out_target)) return;
    boots_cstr_array_append(&out_target->include_dirs, dirs);
    int n = out_target->include_dirs.size;

    for(int i = 0; i<n; i++){
        printf("INFO: Adding %s to %s\n", out_target->include_dirs.elements[i], out_target->name);
    }
}

#endif //BOOTS_IMPLEMENTATION
#endif //BOOTS_INCLUDE
