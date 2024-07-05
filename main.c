#include "token.h"
#include <stdarg.h>

static void error(char *fmt, ...) {
    va_list argument_pointer;
    va_start(argument_pointer, fmt);
    vfprintf(stderr, fmt, argument_pointer);
    fprintf(stderr, "\n");
    exit(1);
}

static bool equal(Token *token, char *op) {
    return memcmp(token->loc, op, token->len) == 0 && op[token->len] == '\0';
}

static Token *skip(Token *token, char *s) {
    if (!equal(token, s)) error("expected '%s'", s);
    return token->next;
}

static int get_number(Token *token) {
    if (token->type != T_NUM) error("expected a number");
    return token->val;
}

static Token *new_token(TokenType type, char *start, char *end) {
    Token *token = calloc(1, sizeof(Token));
    token->type = type;
    token->loc = start;
    token->len = end - start;
    return token;
}

static Token *tokenize(char *p) {
    Token head = {};
    Token *cur = &head;

    while (*p) {

        // Skip whitespace characters
        if (is_space(*p)) {
            p++;
            continue;
        }

        
        // Numeric literal
        if (isdigit(*p)) {
            cur = cur->next = new_token(T_NUM, p, p);
            char *q = p;
            cur->val = strtoul(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        // Punctuator
        if (*p == '+' || *p == '-') {
            cur = cur->next = new_token(T_PUNCT, p, p + 1);
            p++;
            continue;
        }

        error("invalid token");
    }

    cur = cur->next = new_token(T_EOF, p, p);
    return head.next;
}

int main(int argc, char **argv) {
    if (argc != 2) error("%s: invalid number of arguments", argv[0]);

    Token *token = tokenize(argv[1]);
    
    printf("  .globl main\n");
    printf("main:\n");

    // First token has to be a number
    printf("  mov $%d, %%rax\n, ", get_number(token));
    token = token->next;

    while (token->type != T_EOF) {
        if (equal(token, "+")) {
            printf("  add$%d, %%rax\n", get_number(token->next));
            token = token->next->next;
            continue;
        }

        token = skip(token, "-");
        printf("  sub $%d, %%rax\n", get_number(token));
        token = token->next;
    }
    printf("  ret\n");
    return 0;
}