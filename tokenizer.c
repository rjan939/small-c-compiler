#include "token.h"


static File *current_file;
// temporary in order to test easier
static char *current_input;

// Compilers have to handle multiple input files at the same time
static File **input_files;

void error(char *fmt, ...) {
  va_list argument_pointer;
  va_start(argument_pointer, fmt);
  vfprintf(stderr, fmt, argument_pointer);
  fprintf(stderr, "\n");
  va_end(argument_pointer);
  exit(1);
}

// Reports error location and then exits
void verror_at(char *location, char *fmt, va_list argument_pointer) {
  int position = location - current_input;
  fprintf(stderr, "%s\n", current_input);
  fprintf(stderr, "%*s", position, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, argument_pointer);
  fprintf(stderr, "\n");
  va_end(argument_pointer);
  exit(1);
}

void error_at(char *location, char *fmt, ...) {
  va_list argument_pointer;
  va_start(argument_pointer, fmt);
  verror_at(location, fmt, argument_pointer);
}

void error_tok(Token *token, char *fmt, ...) {
  va_list argument_pointer;
  va_start(argument_pointer, fmt);
  verror_at(token->loc, fmt, argument_pointer);
}


void free_token(Token *token) {
  if (!token) return;
  free_token(token->next);
  free(token);
}

bool equal(Token *token, char *op) {
  return memcmp(token->loc, op, token->len) == 0 && op[token->len] == '\0';
}

Token *skip(Token *token, char *s) {
  if (!equal(token, s))
    error_tok(token, "expected '%s'", s);
  return token->next;
}

static Token *new_token(TokenType type, char *start, char *end) {
  Token *token = calloc(1, sizeof(Token));
  if (token == NULL) {
    error("not enough memory in system");
  }

  token->type = type;
  token->loc = start;
  token->len = end - start;
  return token;
}

static bool startswith(char *p, char *q) {
  return strncmp(p, q, strlen(q)) == 0;
}

// Returns true if c is valid as the first character of an identifier 
static bool is_valid_ident_first_character(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

// Returns true if c is valid as a non-first character of an identifier
static bool is_valid_ident_character(char c) {
  return is_valid_ident_first_character(c) || ('0' <= c && c <= '9');
}

// Returns length of punctuator token from p
static int get_punct_length(char *p) {
  if (startswith(p, "==") || startswith(p, "!=") ||
      startswith(p, "<=") || startswith(p, ">="))
    return 2;

  return ispunct(*p) ? 1 : 0;
}


Token *tokenized(File *file) {
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
  return NULL;
}

static bool is_keyword(Token *token) {
  static char *keywords[] = {"return", "if", "else", "for"};
  
  for (int i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
    if (equal(token, keywords[i]))
      return true;
  }
  return false;
}

static void identify_keywords(Token *token) {
  for (Token *curr = token; curr->type != T_EOF; curr = curr->next) {
    if (equal(curr, "return")) {
      curr->type = T_KEYWORD;
    }
  }
}

// Tokenize 'current_file' and returns new tokens
Token *tokenize(char *p) {
  current_input = p;
  Token head = {};
  Token *cur = &head;

  while (*p) {

    // Skip whitespace characters
    if (isspace(*p)) {
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

    // Identifier 
    if (is_valid_ident_first_character(*p)) {
      char *start = p;
      do {
        p++;
      } while (is_valid_ident_character(*p));
      cur = cur->next = new_token(T_IDENT, start, p);
      if (equal(cur, "return")) {
        cur->type = T_KEYWORD;
      }
      continue;
    }

    // Punctuator
    int punct_len = get_punct_length(p);
    if (punct_len) {
      cur = cur->next = new_token(T_PUNCT, p, p + punct_len);
      p += punct_len;
      continue;
    }
    error_at(p, "invalid token");
  }
  cur = cur->next = new_token(T_EOF, p, p);
  //identify_keywords(head.next);
  return head.next;
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
