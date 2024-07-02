#include "token.h"


static File *current_file;

// Compilers have to handle multiple input files at the same time
static File **input_files;

Token *tokenize(File *file) {
    current_file = file;
    char *content = file->contents;
    
    while (*content) {
        // Check if this starts with a "//" and skip it
        // if yes, do another while loop to keep incrementing content until it hits a new line and then start at the top of the loop again
        // Check if this starts with a "/*"
        // if yes, find the first occurence of "*/" after incrementing content by 2
        // if its not found, throw an error cause they didnt close the block comment and continue
        // Check if this starts with "\n"

        // List of all cases: //, /*, \n, , 1-9, "", u8\"", u\"", "L\"", "U\"", \, u', L', U', identifiers, keywords, punctuators, 
    }
}

File *new_file(char* name, int file_num, char *contents) {
    File *file = calloc(1, sizeof(File));
    file->name = name;
    file->unique_id = file_num;
    file->display_name = name;
    file->contents = contents;
    return file;
}

static char *read_file(char* path) {
    FILE *file_path;

    if (strcmp(path, "-") == 0) {
        // If a given file name is "-", read from stdin
        // Source: https://github.com/nektos/act/issues/998
        file_path = stdin;
    } else {
        file_path = fopen(path, "r");
        if (file_path == NULL) return NULL;
    }

    char *buf;
    size_t buflen;
    FILE *out = open_memstream(&buf, &buflen);

    for (;;) {
        // https://stackoverflow.com/questions/3033771/file-i-o-with-streams-best-memory-buffer-size
        char buf2[4096];
        int num_full_items = fread(buf2, sizeof(char), sizeof(buf2), file_path);
        if (num_full_items == 0) 
            break;
        fwrite(buf2, sizeof(char), num_full_items, out);
    }

    if (file_path != stdin) 
        fclose(file_path);

    // Last line should end with '\n'
    fflush(out);
    if (buflen == 0 || buf[buflen - 1] != '\n') {
        fputc('\n', out);
    }
    fputc('\0', out);
    fclose(out);
    return buf;

}

Token *tokenize_file(char *path) {
    char *p = read_file(path);
    if (!p) return NULL;

    // UTF-8 text might have a 3-byte long BOM: https://en.wikipedia.org/wiki/Byte_order_mark#Byte-order_marks_by_encoding
    // This statement skips it
    if (!memcmp(p, "\xef\xbb\xbf", 3))
        p += 3;

    static int file_num;

    File* file = new_file(path, file_num + 1, p);

    input_files = realloc(input_files, sizeof(char *) * (file_num + 2));
    input_files[file_num] = file;
    input_files[file_num + 1] = NULL;
    file_num++;

    return NULL;
}
