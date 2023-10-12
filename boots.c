#define BOOTS_IMPLEMENTATION
#include "boots.h"

#define PROJECT_NAME "Test Project"

// other functions I'd like to have
// target_include_directories
// target_link_libraries
// find_library
int main(int argc, char **argv){
    REBUILD_BOOTS(argc, argv);
    PROJECT(PROJECT_NAME);
    ADD_EXECUTABLE("main3", RPATH("examples", "main.c"), RPATH("examples", "lib.c"));
    TARGET_ADD_INCLUDE_DIR("main3", "examples");
    MAKE("main3");
    return 0;
}
