#include <token.h>


static File *current_file;

static char *read_file(char* path) {
    FILE *file_path;

    if (strcmp(path, "-") == 0) {
        // If a given file name is "-", read from stdin
        // Source: https://github.com/nektos/act/issues/998
        file_path = stdin;
    } else {
        file_path = fopen(path, "r");
        if (!file_path) return NULL;
    }

    char *buf;
    size_t buflen;
    FILE *out = open_memstream(&buf, &buflen);

    for (;;) {
        char buf2[4096];
        int num_full_items = fread(buf2, 1, sizeof(buf2), file_path);
        if (num_full_items == 0) 
            break;
        fwrite(buf2, 1, num_full_items, out);
        
    }
}

Token *tokenize(char *path) {
    char *p = read_file(path);
}
