#include "token.h"


static File *current_file;

static char *current_filename;

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

// Reports error message in following format and exits
//
// main.c:10: x = y + 1;
//                ^ <error message here>
void verror_at(int line_num, char *location, char *fmt, va_list argument_pointer) {
  // Find a line containing `location`
  char *line = location;
  while (current_input < line && line[-1] != '\n')
    line--;
  
  char *end = location;
  while (*end != '\n')
    end++;
  
  // Print out the line
  int indent = fprintf(stderr, "%s:%d: ", current_filename, line_num);
  fprintf(stderr, "%.*s\n", (int) (end - line), line);

  // Show error message
  int position = location - line + indent;
  fprintf(stderr, "%*s", position, "");
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, argument_pointer);
  fprintf(stderr, "\n");
  va_end(argument_pointer);
  exit(1);
}

void error_at(char *location, char *fmt, ...) {
  int line_num = 1;
  for (char *p = current_input; p < location; p++)
    if (*p == '\n')
      line_num++;
  va_list argument_pointer;
  va_start(argument_pointer, fmt);
  verror_at(line_num, location, fmt, argument_pointer);
}

void error_tok(Token *token, char *fmt, ...) {
  va_list argument_pointer;
  va_start(argument_pointer, fmt);
  verror_at(token->line_num, token->loc, fmt, argument_pointer);
}

static void add_line_numbers(Token *token) {
  char *p = current_input;
  int n = 1;
  do {
    if (p == token->loc) {
      token->line_num = n;
      token = token->next;
    }
    if (*p == '\n')
      n++;
  } while(*p++);
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

bool consume(Token **rest, Token *token, char *str) {
  if (equal(token, str)) {
    *rest = token->next;
    return true;
  }
  *rest = token;
  return false;
}

static Token *new_token(TokenType type, char *start, char *end) {
  Token *token = calloc(1, sizeof(Token));
  if (token == NULL) {
    error("not enough memory in system");
  }

  token->token_type = type;
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

static int from_hex(char c) {
  if ('0' <= c && c <= '9')
    return c - '0';
  if ('a' <= c && c <= 'f')
    return c - 'a' + 10;
  return c - 'A' + 10;
}

// Returns length of punctuator token from p
static int get_punct_length(char *p) {
  static char *puncts[] = {"==", "!=", "<=", ">=", "->"};

  for (int i = 0; i < sizeof(puncts) / sizeof(*puncts); i++)
    if (startswith(p, puncts[i]))
      return strlen(puncts[i]);

  return ispunct(*p) ? 1 : 0;
}

static bool is_keyword(Token *token) {
  static char *keywords[] = {
    "return", "if", "else", "for", "while", "int", "sizeof", "char",
    "struct", "union",
  };
  
  for (int i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
    if (equal(token, keywords[i]))
      return true;
  }
  return false;
}

static int read_escaped_char(char **new_pos, char *p) {
  if ('0' <= *p && *p <= '7') {
    // octal num
    int c = *p++ - '0';
    if ('0' <= *p && *p <= '7') {
      c = (c << 3) + (*p++ - '0');
      if ('0' <= *p && *p <= '7')
        c = (c << 3) + (*p++ - '0');
    }
    *new_pos = p;
    return c;
  }

  if (*p == 'x') {
    // hex num
    p++;
    if (!isxdigit(*p))
      error_at(p, "invalid hex escape sequence");

    int c = 0;
    for (; isxdigit(*p); p++)
      c = (c << 4) + from_hex(*p);
    *new_pos = p;
    return c;
  }

  *new_pos = p + 1;

  // Escape sequences are defined using themselves here
  // Ex. '\n' is implemented using '\n'. This works
  // because the compiler that compiles our compiler knows
  // what '\n' is. Basically we inherit the ASCII code of
  // '\n' from the compiler that compiles our compiler
  // so we don't have to implement the actual code here

  // This is for security reasons. 
  // For more info, read "Reflections on Trusting Trust" by Ken Thompson.
  // https://www.cs.cmu.edu/~rdriley/487/papers/Thompson_1984_ReflectionsonTrustingTrust.pdf
  switch (*p) {
    case 'a': return '\a';
    case 'b': return '\b';
    case 't': return '\t';
    case 'n': return '\n';
    case 'v': return '\v';
    case 'f': return '\f';
    case 'r': return '\r';
    // [GNU] \e for the ASCII escape character is a GNU C extension
    case 'e': return 27;
    default: return *p;
  }
}

// Find a closing double-quote.
static char *string_literal_end(char *p) {
  char *start = p;
  for (; *p != '"'; p++) {
    if (*p == '\n' || *p == '\0')
      error_at(start, "unclosed string literal");
    if (*p == '\\')
      p++;
  }
  return p;
}

static Token *read_string_literal(char *start) {
  char *end = string_literal_end(start + 1);
  char *buf = calloc(1, end - start);
  int len = 0;


  for (char *p = start + 1; p < end;) {
    if (*p == '\\') {
      buf[len++] = read_escaped_char(&p, p + 1);
    } else {
      buf[len++] = *p++;
    }
  }

  Token *token = new_token(T_STR, start, end + 1);
  token->type = array_of(ty_char, len + 1);
  token->str = buf;
  return token;
}

static void identify_keywords(Token *token) {
  for (Token *curr = token; curr->token_type != T_EOF; curr = curr->next) 
    if (is_keyword(curr)) 
      curr->token_type = T_KEYWORD;
}

// Tokenize 'current_file' and returns new tokens
static Token *tokenize(char *filename, char *p) {
  current_filename = filename;
  current_input = p;
  Token head = {};
  Token *cur = &head;

  while (*p) {
    // Skip line comments
    if (startswith(p, "//")) {
      p += 2;
      while (*p != '\n')
        p++;
      continue;
    }

    // Skip block comments
    if (startswith(p, "/*")) {
      char *substring = strstr(p + 2, "*/");
      if (!substring)
        error_at(p, "unclosed block comment");
      p = substring + 2;
      continue;
    }

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

    // String literal
    if (*p == '"') {
      cur = cur->next = read_string_literal(p);
      p += cur->len;
      continue;
    }

    // Identifier or keyword
    if (is_valid_ident_first_character(*p)) {
      char *start = p;
      do {
        p++;
      } while (is_valid_ident_character(*p));
      cur = cur->next = new_token(T_IDENT, start, p);
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
  add_line_numbers(head.next);
  identify_keywords(head.next);
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
  
  /*static int file_num;
  File* file = new_file(path, file_num + 1, p);

   DONT USE THIS CODE RN ITS BROKEN
  input_files = realloc(input_files, sizeof(char *) * (file_num + 2));
  input_files[file_num] = file;
  input_files[file_num + 1] = NULL;
  file_num++;*/

  return tokenize(path, p);
}
