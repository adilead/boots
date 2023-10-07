#define BOOTS_IMPLEMENTATION
#include "boots.h"

#define PROJECT_NAME "Test Project"


int main(int argc, char **argv){
    REBUILD_BOOTS(argc, argv);
    PROJECT(PROJECT_NAME);
    printf("Created %s\n", PROJECT_NAME);
    ADD_TARGET("main", "examples/main.c");
    MAKE("main");
    return 0;
}
