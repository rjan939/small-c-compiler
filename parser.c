// This file is a recursive descent parser for C
#include "token.h"

// Scope for local variables, global variables, typedefs
// or enum constants
typedef struct var_scope var_scope;
struct var_scope {
        var_scope *next;
        char *name;
        Obj *var;
        Type *type_def;
        Type *enum_type;
        int enum_val;
};

// Scope for struct, union, or enum tags
typedef struct tag_scope tag_scope;
struct tag_scope {
        tag_scope *next;
        char *name;
        Type *type;
};

// Block scope
typedef struct Scope Scope;
struct Scope {
        Scope *next;

        // C has two block scopes; one is for variables/typedefs and
        // the other is for struct/union/enum tags
        var_scope *vars;
        tag_scope *tags;
};

typedef struct {
        bool is_typedef;
        bool is_static;
} var_attribute;

// Local variables in the parser
static Obj *locals;

// Global variables in this list
static Obj *globals;

static Scope *scope = &(Scope){};

// Points to the function object the parser is currently parsing
static Obj *current_func;

static bool is_typename(Token *token);
static Type *declaration_specifier(Token **rest, Token *token, var_attribute *attribute);
static Type *enum_specifier(Token **rest, Token *token);
static Type *type_suffix(Token **rest, Token *token, Type *type);
static Type *declarator(Token **rest, Token *token, Type *type);
static Node *declaration(Token **rest, Token *token, Type *basetype);
static Node *compound_statement(Token **rest, Token *token);
static Node *statement(Token **rest, Token *token);
static Node *expr_statement(Token **rest, Token *token);
static Node *expr(Token **rest, Token *token);
static Node *assign(Token **rest, Token *token);
static Node *logor(Token **rest, Token *token);
static Node *logand(Token **rest, Token *token);
static Node *bitor(Token **rest, Token *token);
static Node *bitxor(Token **rest, Token *token);
static Node *bitand(Token **rest, Token *token);
static Node *equality(Token **rest, Token *token);
static Node *relational(Token **rest, Token *token);
static Node *add(Token **rest, Token *token);
static Node *new_add(Node *left, Node *right, Token *token);
static Node *new_sub(Node *left, Node *right, Token *token);
static Node *mul(Token **rest, Token *token);
static Node *cast(Token **rest, Token *token);
static Type *struct_declaration(Token **rest, Token *token);
static Type *union_declaration(Token **rest, Token *token);
static Node *postfix(Token **rest, Token *token);
static Node *unary(Token **rest, Token *token);
static Node *primary(Token **rest, Token *token);
static Token *parse_typedef(Token *token, Type *basetype);

// TODO: free scopes
static void enter_scope(void) {
        Scope *sc = calloc(1, sizeof(Scope));
        if (sc == NULL)
                error("not enough memory in system to allocate scope");
        sc->next = scope;
        scope = sc;
}

static void leave_scope(void) {
        scope = scope->next;
}

// Find a variable based on name
static var_scope *find_var(Token *token) {
        for (Scope *sc = scope; sc; sc = sc->next)
                for (var_scope *sc2 = sc->vars; sc2; sc2 = sc2->next)
                        if (equal(token, sc2->name))
                                return sc2;
        return NULL;
}

static Type *find_tag(Token *token) {
        for (Scope *sc = scope; sc; sc = sc->next)
                for (tag_scope *sc2 = sc->tags; sc2; sc2 = sc2->next)
                        if (equal(token, sc2->name))
                                return sc2->type;
        return NULL;
}

// TODO: free var_scopes
static var_scope *push_scope(char *name) {
        var_scope *sc = calloc(1, sizeof(var_scope));
        if (sc == NULL)
                error("not enough memory in system to allocate var scope");
        sc->name = name;
        sc->next = scope->vars;
        scope->vars = sc;
        return sc;
}

static Node *new_node(NodeType type, Token *token) {
        Node* node = calloc(1, sizeof(Node));
        if (node == NULL) {
                error("not enough memory in system");
        }
        node->node_type = type;
        node->token = token;
        return node;
}

void free_node(Node *node) {
        if (!node) return;
        free_node(node->left);
        free(node->type);
        free_node(node->right);
        free(node->type);
        free(node);
}


static Node *new_binary(NodeType type, Node *left, Node *right, Token *token) {
        Node *node = new_node(type, token);
        node->left = left;
        node->right = right;
        return node;
}

static Node *new_num(int64_t val, Token *token) {
        Node *node = new_node(ND_NUM, token);
        node->val = val;
        return node;
}

static Node *new_long(int64_t val, Token *token) {
        Node *node = new_node(ND_NUM, token);
        node->val = val;
        node->type = ty_long;
        return node;
}

static Node *new_var_node(Obj *var, Token *token) {
        Node *node = new_node(ND_VAR, token);
        node->var = var;
        return node;
}

// TODO: free cast nodes
Node *new_cast(Node *expr, Type *type) {
        add_type(expr);

        Node *node = calloc(1, sizeof(Node));
        if (node == NULL)
                error("not enough memory in system for cast");
        node->node_type = ND_CAST;
        node->token = expr->token;
        node->left = expr;
        node->type = copy_type(type);
        return node;
}

static Obj *new_var(char *name, Type *type) {
        Obj *var = calloc(1, sizeof(Obj));
        if (var == NULL)
                error("not enough memory in system for var");
        var->name = name;
        var->type = type;
        push_scope(name)->var = var;
        return var;
}

static Obj *new_lvar(char *name, Type *type) {
        Obj* var = new_var(name, type);
        var->is_local = true;
        var->next = locals;

        locals = var;
        return var;
}

static Obj *new_gvar(char *name, Type *type) {
        Obj *var = new_var(name, type);
        var->next = globals;
        globals = var;
        return var;
}

void free_lvar(Obj *locals) {
        if (!locals) return;
        free_lvar(locals->next);
        free(locals);
}

static Node *new_unary(NodeType type, Node *expr, Token *token) {
        Node *node = new_node(type, token);
        node->left = expr;
        return node;
}

static char *new_unique_name(void) {
        static int id = 0;
        return format(".L..%d", id++);
}

static Obj *new_anon_gvar(Type *type) {
        return new_gvar(new_unique_name(), type);
}

static Obj *new_string_literal(char *p, Type *type) {
        Obj *var = new_anon_gvar(type);
        var->init_data = p;
        return var;
}

static char *get_ident(Token *token) {
        if (token->token_type != T_IDENT)
                error_tok(token, "expected an identifier");
        return strndup(token->loc, token->len);
}

static Type *find_typedef(Token *token) {
        if (token->token_type == T_IDENT) {
                var_scope *sc = find_var(token);
                if (sc)
                        return sc->type_def;
        }
        return NULL;
}

static long get_number(Token *token) {
        if (token->token_type != T_NUM)
                error_tok(token, "expected a number");
        return token->val;
}

static void push_tag_scope(Token *token, Type *type) {
        tag_scope *sc = calloc(1, sizeof(tag_scope));
        if (sc == NULL)
                error("not enough memory in system to allocate struct tag");
        sc->name = strndup(token->loc, token->len);
        sc->type = type;
        sc->next = scope->tags;
        scope->tags = sc;
}

// declaration-specifier = ("void" | "_Bool" | "char" | "short" | "int" | "long"
//                          | "typedef" | "static"
//                          | struct-declaration | union-declaration | typedef-name
//                          | enum-specifier)+

// Order of typenames doesnt matter, int long static means same as static int long
// Count number of occurences of each typename while keeping the "current" type object that the typenames up
// until that point represent. Return only when there is a non-typename token
static Type *declaration_specifier(Token **rest, Token *token, var_attribute *attribute) {

        enum {
                VOID = 1 << 0,
                BOOL = 1 << 2,
                CHAR = 1 << 4,
                SHORT = 1 << 6,
                INT = 1 << 8,
                LONG = 1 << 10,
                OTHER = 1 << 12,
        };
        Type *type = ty_int;
        int counter = 0;

        while (is_typename(token)) {
                // Handle storage class specifiers
                if (equal(token, "typedef") || equal(token, "static")) {
                        if (!attribute) 
                                error_tok(token, "storage class specifier is not allowed in this context");
                        if (equal(token, "typedef"))
                                attribute->is_typedef = true;
                        else
                                attribute->is_static = true;

                        if (attribute->is_typedef + attribute->is_static > 1)
                                error_tok(token, "typedef and static may not be used together");
                        token = token->next;

                        continue;
                }
                Type *type2 = find_typedef(token);
                if (equal(token, "struct") || equal(token, "union") || equal(token, "enum") || type2) {
                        if (counter)
                                break;
                        if (equal(token, "struct"))
                                type = struct_declaration(&token, token->next);
                        else if (equal(token, "union")) 
                                type = union_declaration(&token, token->next);
                        else if (equal(token, "enum")) {
                                type = enum_specifier(&token, token->next);
                        } else {
                                type = type2;
                                token = token->next;
                        }

                        counter += OTHER;
                        continue;
                }

                if (equal(token, "void"))
                        counter += VOID;
                else if (equal(token, "_Bool"))
                        counter += BOOL;
                else if (equal(token, "char"))
                        counter += CHAR;
                else if (equal (token, "short"))
                        counter += SHORT;
                else if (equal(token, "int"))
                        counter += INT;
                else if (equal(token, "long"))
                        counter += LONG;
                else
                        unreachable();

                switch (counter) {
                        case VOID:
                                type = ty_void;
                                break;
                        case BOOL:
                                type = ty_bool;
                                break;
                        case CHAR:
                                type = ty_char;
                                break;
                        case SHORT:
                        case SHORT + INT:
                                type = ty_short;
                                break;
                        case INT:
                                type = ty_int;
                                break;
                        case LONG:
                        case LONG + INT:
                        case LONG + LONG:
                        case LONG + LONG + INT:
                                type = ty_long;
                                break;
                        default:
                                error_tok(token, "invalid type");
                }
                token = token->next;
        }

        *rest = token;
        return type;
}

// func-params = (param ("," param)*)? ")"
// param       = declaration-specifier declarator
static Type *func_params(Token **rest, Token *token, Type *type) {
        Type head = {};
        Type *cur = &head;

        while (!equal(token, ")")) {
                if (cur != &head)
                        token = skip(token, ",");
                Type *type2 = declaration_specifier(&token, token, NULL);
                type2 = declarator(&token, token, type2);

                // Array of "T" is converted to pointer of "T" only in the parameter
                // context. For example, *argv[] is converted to **argv by this.
                if (type2->kind == TY_ARRAY) {
                        Token *name = type2->name;
                        type2 = pointer_to(type2->base);
                        type2->name = name;
                }

                cur = cur->next = copy_type(type2);
        }

        type = func_type(type);
        type->params = head.next;
        *rest = token->next;
        return type;
}

// array-dimensions = num? "]" type-suffix
static Type *array_dimensions(Token **rest, Token *token, Type *type) {
        if (equal(token, "]")) {
                type = type_suffix(rest, token->next, type);
                return array_of(type, -1);
        }
        int sz = get_number(token);
        token = skip(token->next, "]");
        type = type_suffix(rest, token, type);
        return array_of(type, sz);
}

// type-suffix = "(" func-params
//             | "[" array-dimensions
//             | ε
static Type *type_suffix(Token **rest, Token *token, Type *type) {
        if (equal(token, "(")) 
                return func_params(rest, token->next, type);

        if (equal(token, "[")) {
                return array_dimensions(rest, token->next, type);
        }

        *rest = token;
        return type;
}

// declarator = "*"* ("(" ident ")" | "(" declarator ")" | ident) type-suffix
static Type *declarator(Token **rest, Token *token, Type *type) {
        while (consume(&token, token, "*"))
                type = pointer_to(type);

        if (equal(token, "(")) {
                Token *start = token;
                Type dummy = {};
                declarator(&token, start->next, &dummy);
                token = skip(token, ")");
                type = type_suffix(rest, token, type);
                return declarator(&token, start->next, type);
        }
        if (token->token_type != T_IDENT)
                error_tok(token, "expected a variable name");

        type = type_suffix(rest, token->next, type);
        type->name = token;
        return type;
}

// abstract-declarator = "*" ("(" abstract-declarator ")")? type-suffix
static Type *abstract_declarator(Token **rest, Token *token, Type *type) {
        while (equal(token, "*")) {
                type = pointer_to(type);
                token = token->next;
        }

        if (equal(token, "(")) {
                Token *start = token;
                Type dummy = {};
                abstract_declarator(&token, start->next, &dummy);
                token = skip(token, ")");
                type = type_suffix(rest, token, type);
                return abstract_declarator(&token, start->next, type);
        }

        return type_suffix(rest, token, type);
}

// type-name = declaration-specifier abstract-declarator
static Type *typename(Token **rest, Token *token) {
        Type *type = declaration_specifier(&token, token, NULL);
        return abstract_declarator(rest, token, type);
}

// enum-specifier = ident? "{" enum-list? "}"
//                | ident ("{" enum-list? "}")?
// 
// enum-list = ident ("=" num)? ("," ident ("=" num)?)*
static Type *enum_specifier(Token **rest, Token *token) {
        Type *type = enum_type();
        Token *tag = NULL;
        if (token->token_type == T_IDENT) {
                tag = token;
                token = token->next;
        }

        if (tag && !equal(token, "{")) {
                Type *type = find_tag(tag);
                if (!type)
                        error_tok(tag, "unknown enum type");
                if (type->kind != TY_ENUM)
                        error_tok(tag, "not an enum tag");
                *rest = token;
                return type;
        }

        token = skip(token, "{");

        // Read an enum-list
        int i = 0;
        int val = 0;
        while (!equal(token, "}")) {
                if (i++ > 0)
                        token = skip(token, ",");
                char *name = get_ident(token);
                token = token->next;

                if (equal(token, "=")) {
                        val = get_number(token->next);
                        token = token->next->next;
                }

                var_scope *sc = push_scope(name);
                sc->enum_type = type;
                sc->enum_val = val++;
        }

        *rest = token->next;

        if (tag)
                push_tag_scope(tag, type);
        return type;
}

// declaration = declaration_specifier (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
static Node *declaration(Token **rest, Token *token, Type *basetype) {
        Node head = {};
        Node *cur = &head;
        int i = 0;

        while (!equal(token, ";")) {
                if (i++ > 0)
                        token = skip(token, ",");

                Type *type = declarator(&token, token, basetype);
                if (type->size < 0)
                        error_tok(token, "variable has incomplete type");
                if (type->kind == TY_VOID)
                        error_tok(token, "variable declared void");
                Obj *var = new_lvar(get_ident(type->name), type);

                if (!equal(token, "="))
                        continue;

                Node *left = new_var_node(var, type->name);
                Node *right = assign(&token, token->next);
                Node *node = new_binary(ND_ASSIGN, left, right, token);
                cur = cur->next = new_unary(ND_STATEMENT, node, token);
        }

        Node *node = new_node(ND_BLOCK, token);
        node->body = head.next;
        *rest = token->next;
        return node;
}

// Returns true if token represents a type
static bool is_typename(Token *token) {
        static char *typenames[] = { 
                "void", "_Bool", "char", "short", "int", "long", "struct", "union", 
                "typedef", "enum", "static",
        };

        for (int i = 0; i < sizeof(typenames) / sizeof(*typenames); i++)
                if (equal(token, typenames[i]))
                        return true;
        return find_typedef(token);
}

// stmt = "return" expr ";" 
//        | "if" "(" expr ")" stmt ("else" stmt)?
//        | "for" "(" expr-stmt expr? ";" expr? ")" statement
//        | "while" "(" expr ")" stmt
//        | "{" compound-stmt
//        | expr->stmt
static Node *statement(Token **rest, Token *token) {
        if (equal(token, "return")) {
                Node *node = new_node(ND_RETURN, token);
                Node *exp = expr(&token, token->next);
                *rest = skip(token, ";");

                add_type(exp);
                node->left = new_cast(exp, current_func->type->return_type);
                return node;
        }

        if (equal(token, "if")) {
                Node *node = new_node(ND_IF, token);
                token = skip(token->next, "(");
                node->cond = expr(&token, token);
                token = skip(token, ")");
                node->then = statement(&token, token);

                if (equal(token, "else"))
                        node->els = statement(&token, token->next);
                *rest = token;
                return node;
        }

        if (equal(token, "for")) {
                Node *node = new_node(ND_FOR, token);
                token = skip(token->next, "(");

                enter_scope();

                if (is_typename(token)) {
                        Type *basetype = declaration_specifier(&token, token, NULL);
                        node->init = declaration(&token, token, basetype);
                } else {
                        node->init = expr_statement(&token, token);
                }

                if (!equal(token, ";"))
                        node->cond = expr(&token, token);
                token = skip(token, ";");

                if (!equal(token, ")"))
                        node->inc = expr(&token, token);
                token = skip(token, ")");

                node->then = statement(rest, token);
                leave_scope();
                return node;
        }

        if (equal(token, "while")) {
                Node *node = new_node(ND_FOR, token);
                token = skip(token->next, "(");
                node->cond = expr(&token, token);
                token = skip(token, ")");
                node->then = statement(rest, token);
                return node;
        }

        if (equal(token, "{"))
                return compound_statement(rest, token->next);

        return expr_statement(rest, token);
}

// compound-stmt = (typedef | declaration | stmt)* "}"
static Node *compound_statement(Token **rest, Token* token) {
        Node *node = new_node(ND_BLOCK, token);
        Node head = {};
        Node *cur = &head;

        enter_scope();

        while (!equal(token, "}")) {
                if (is_typename(token)) {
                        var_attribute attribute = {};
                        Type *basetype = declaration_specifier(&token, token, &attribute);

                        if (attribute.is_typedef) {
                                token = parse_typedef(token, basetype);
                                continue;
                        }
                        cur = cur->next = declaration(&token, token, basetype);
                } else {
                        cur = cur->next = statement(&token, token);
                }
                add_type(cur);
        }

        leave_scope();
        node->body = head.next;
        *rest = token->next;
        return node;
}

// expr-statement = expr? ";"
static Node *expr_statement(Token **rest, Token *token) {
        if (equal(token, ";")) {
                *rest = token->next;
                return new_node(ND_NULL_STATEMENT, token);
        }
        Node *node = new_node(ND_STATEMENT, token);
        node->left = expr(&token, token);
        *rest = skip(token, ";");
        return node;
}

// expr = assign ("," expr)?
static Node *expr(Token **rest, Token *token) {
        Node *node = assign(&token, token);
        if (equal(token, ","))
                return new_binary(ND_COMMA, node, expr(rest, token->next), token);
        *rest = token;
        return node;
}

// Convert `A op=B` to `tmp = &A, *tmp = *tmp op B`
// where tmp is a fresh pointer variable.
static Node *to_assign(Node *binary) {
        add_type(binary->left);
        add_type(binary->right);
        Token *token = binary->token;

        Obj *var = new_lvar("", pointer_to(binary->left->type));

        Node *expr1 = new_binary(ND_ASSIGN, new_var_node(var, token),
                        new_unary(ND_ADDRESS, binary->left, token), token);

        Node *expr2 =
                new_binary(ND_ASSIGN,
                                new_unary(ND_DEREF, new_var_node(var, token), token),
                                new_binary(binary->node_type,
                                        new_unary(ND_DEREF, new_var_node(var, token), token),
                                        binary->right,
                                        token),
                                token);
        return new_binary(ND_COMMA, expr1, expr2, token);
}

// assign = logor (assign-op assign)?
// assign-op = "=" | "+=" | "-=" | "*=" | "/=" | "%=" | "&=" | "|=" | "^="
static Node *assign(Token **rest, Token *token) {
        Node *node = logor(&token, token);
        if (equal(token, "=")) {
                node = new_binary(ND_ASSIGN, node, assign(&token, token->next), token);
        }

        if (equal(token, "+="))
                return to_assign(new_add(node, assign(rest, token->next), token));

        if (equal(token, "-="))
                return to_assign(new_sub(node, assign(rest, token->next), token));

        if (equal(token, "*="))
                return to_assign(new_binary(ND_MUL, node, assign(rest, token->next), token));

        if (equal(token, "/=")) 
                return to_assign(new_binary(ND_DIV, node, assign(rest, token->next), token));

        if (equal(token, "%="))
                return to_assign(new_binary(ND_MOD, node, assign(rest, token->next), token));

        if (equal(token, "&="))
                return to_assign(new_binary(ND_BITAND, node, assign(rest, token->next), token)); 

        if (equal(token, "|="))
                return to_assign(new_binary(ND_BITOR, node, assign(rest, token->next), token));

        if (equal(token, "^="))
                return to_assign(new_binary(ND_BITXOR, node, assign(rest, token->next), token));

        *rest = token;
        return node;
}

// logor = logand ("||" logand)*
static Node *logor(Token **rest, Token *token) {
        Node *node = logand(&token, token);
        while (equal(token, "||")) {
                Token *start = token;
                node = new_binary(ND_LOGOR, node, logand(&token, token->next), start);
        }
        *rest = token;
        return node;
}

// logand = bitor ("&&" bitor)*
static Node *logand(Token **rest, Token *token) {
        Node *node = bitor(&token, token);
        while (equal(token, "&&")) {
                Token *start = token;
                node = new_binary(ND_LOGAND, node, bitor(&token, token->next), start);
        }
        *rest = token;
        return node;
}

// bitor = bitxor ("|" bitxor)*
static Node *bitor(Token **rest, Token *token) {
        Node *node = bitxor(&token, token);
        while (equal(token, "|")) {
                Token *start = token;
                node = new_binary(ND_BITOR, node, bitxor(&token, token->next), start);
        }
        *rest = token;
        return node;
}

// bitxor = bitand ("^" bitand)*
static Node *bitxor(Token **rest, Token *token) {
        Node *node = bitand(&token, token);
        while (equal(token, "^")) {
                Token *start = token;
                node = new_binary(ND_BITXOR, node, bitand(&token, token->next), start);
        }
        *rest = token;
        return node;
}

// bitand = equality ("&" equality)*
static Node *bitand(Token **rest, Token *token) {
        Node *node = equality(&token, token);
        while (equal(token, "&")) {
                Token *start = token;
                node = new_binary(ND_BITAND, node, equality(&token, token->next), start);
        }
        *rest = token;
        return node;
}


// equality = relational ("==" relational | "!=" relational)* 
static Node *equality(Token **rest, Token *token) {
        Node *node = relational(&token, token);
        for (;;) {
                Token *start = token;
                if (equal(token, "==")) {
                        node = new_binary(ND_EQ, node, relational(&token, token->next), start);
                        continue;
                }

                if (equal(token, "!=")) {
                        node = new_binary(ND_NE, node, relational(&token, token->next), start);
                        continue;
                }

                *rest = token;
                return node;
        }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node *relational(Token **rest, Token *token) {
        Node *node = add(&token, token);

        for (;;) {
                Token *start = token;
                if (equal(token, "<")) {
                        node = new_binary(ND_LT, node, add(&token, token->next), start);
                        continue;
                }

                if (equal(token, "<=")) {
                        node = new_binary(ND_LE, node, add(&token, token->next), start);
                        continue;
                }

                if (equal(token, ">")) {
                        node = new_binary(ND_LT, add(&token, token->next), node, start);
                        continue;
                }

                if (equal(token, ">=")) {
                        node = new_binary(ND_LE, add(&token, token->next), node, start);
                        continue;
                }

                *rest = token;
                return node;
        }
}

// In C, '+' operator is overloaded to do pointer arithmetic
// if p is a pointer, p + n adds not n but sizeof(*p)*n to the value of p,
// so that p + n points to the location n elements ahead of p
// scale integer value before adding to a pointer value
// which is what this function does
static Node *new_add(Node *left, Node *right, Token *token) {
        add_type(left);
        add_type(right);

        // num + num
        if (is_integer(left->type) && is_integer(right->type))
                return new_binary(ND_ADD, left, right, token);

        if (left->type->base && right->type->base)
                error_tok(token, "invalid operands");

        // num + ptr
        if (!left->type->base && right->type->base) {
                Node *tmp = left;
                left = right;
                right = tmp;
        }

        // ptr + num
        right = new_binary(ND_MUL, right, new_long(left->type->base->size, token), token);
        return new_binary(ND_ADD, left, right, token);
}

static Node *new_sub(Node *left, Node *right, Token *token) {
        add_type(left);
        add_type(right);

        // num - num
        if (is_integer(left->type) && is_integer(right->type))
                return new_binary(ND_SUB, left, right, token);

        // ptr - num
        if (left->type->base && is_integer(right->type)) {
                right = new_binary(ND_MUL, right, new_long(left->type->base->size, token), token);
                add_type(right);
                Node *node = new_binary(ND_SUB, left, right, token);
                node->type = left->type;
                return node;
        }

        // ptr - ptr, which returns how many elements are between the two
        if (left->type->base && right->type->base) {
                Node *node = new_binary(ND_SUB, left, right, token);
                node->type = ty_int;
                return new_binary(ND_DIV, node, new_num(left->type->base->size, token), token);
        }

        error_tok(token, "invalid operands");
}

// add = mul ("+" mul | "-" mul)*
static Node *add(Token **rest, Token *token) {
        Node *node = mul(&token, token);
        for (;;) {
                Token *start = token;
                if (equal(token, "+")) {
                        node = new_add(node, mul(&token, token->next), start);
                        continue;
                }

                if (equal(token, "-")) {
                        node = new_sub(node, mul(&token, token->next), start);
                        continue;
                }
                *rest = token;
                return node;
        }
}

// mul = cast ("*" cast | "/" cast | "%" cast)*
static Node *mul(Token **rest, Token *token) {
        Node *node = cast(&token, token); 

        for (;;) {
                Token *start = token;

                if (equal(token, "*")) {
                        node = new_binary(ND_MUL, node, cast(&token, token->next), start);
                        continue;
                }

                if (equal(token, "/")) {
                        node = new_binary(ND_DIV, node, cast(&token, token->next), start);
                        continue;
                }

                if (equal(token, "%")) {
                        node = new_binary(ND_MOD, node, cast(&token, token->next), start);
                        continue;
                }

                *rest = token;
                return node;
        }
}

// cast = "(" type-name ")" cast | unary
static Node *cast(Token **rest, Token *token) {
        if (equal(token, "(") && is_typename(token->next)) {
                Token *start = token;
                Type *type = typename(&token, token->next);
                token = skip(token, ")");
                Node *node = new_cast(cast(rest, token), type);
                node->token = start;
                return node;
        }

        return unary(rest, token);
}

// unary = ("+" | "-" | "*" | "&" | "!" | "~") cast 
//       | ("++" | "--") unary
//       | postfix
static Node *unary(Token **rest, Token *token) {
        if (equal(token, "+")) 
                return cast(rest, token->next);
        if (equal(token, "-")) 
                return new_unary(ND_NEG, cast(rest, token->next), token);
        if (equal(token, "&")) 
                return new_unary(ND_ADDRESS, cast(rest, token->next), token);
        if (equal(token, "*")) 
                return new_unary(ND_DEREF, cast(rest, token->next), token);

        if (equal(token, "!"))
                return new_unary(ND_NOT, cast(rest, token->next), token);
        if (equal(token, "~"))
                return new_unary(ND_BITNOT, cast(rest, token->next), token);

        // Read ++i as i += 1
        if (equal(token, "++"))
                return to_assign(new_add(unary(rest, token->next), new_num(1, token), token));

        // Read --i as i -= 1
        if (equal(token, "--"))
                return to_assign(new_sub(unary(rest, token->next), new_num(1, token), token));

        return postfix(rest, token);
}


// TODO: free struct memory
// struct-members = (declaration-specifier declarator ("," declarator)* ";")*
static void struct_members(Token **rest, Token *token, Type *type) {
        Member head = {};
        Member *cur = &head;

        while (!equal(token, "}")) {
                Type *basetype = declaration_specifier(&token, token, NULL);
                int i = 0;

                while (!consume(&token, token, ";")) {
                        if (i++)
                                token = skip(token, ",");

                        Member *member = calloc(1, sizeof(Member));
                        if (member == NULL)
                                error("not enough memory to allocate for member");
                        member->type = declarator(&token, token, basetype);
                        member->name = member->type->name;
                        cur = cur->next = member;
                }
        }

        *rest = token->next;
        type->members = head.next;
}

// struct-union-declaration = ident? ("{" struct-members)?
static Type *struct_union_declaration(Token **rest, Token *token) {
        // Read tag
        Token *tag = NULL;
        if (token->token_type == T_IDENT) {
                tag = token;
                token = token->next;
        }

        if (tag && !equal(token, "{")) {
                Type *type = find_tag(tag);
                if (!type)
                        error_tok(tag, "unknown struct type");
                *rest = token;
                return type;
        }

        // Build struct object
        Type *type = calloc(1, sizeof(Type));
        if (type == NULL)
                error("not enough memory in system to allocate for struct object");
        type->kind = TY_STRUCT;
        struct_members(rest, token->next, type);
        type->align = 1;
        if (tag)
                push_tag_scope(tag, type);
        return type;
}


// struct-declaration = ident? "{" struct-members
static Type *struct_declaration(Token **rest, Token *token) {
        Type *type = struct_union_declaration(rest, token);
        type->kind = TY_STRUCT;

        // Assign offsets within structs to members
        int offset = 0;
        for (Member *member = type->members; member; member = member->next) {
                offset = align_to(offset, member->type->align);
                member->offset = offset;
                offset += member->type->size;

                if (type->align < member->type->align)
                        type->align = member->type->align;
        }
        type->size = align_to(offset, type->align);
        return type;
}

// union-declaration = struct-union-declaration
static Type *union_declaration(Token **rest, Token *token) {
        Type *type = struct_union_declaration(rest, token);
        type->kind = TY_UNION;

        // Unions dont need offsets since they are already initialized to zero, we still need to compute alignment and size though
        for (Member *member = type->members; member; member = member->next) {
                if (type->align < member->type->align)
                        type->align = member->type->align;
                if (type->size < member->type->size) 
                        type->size = member->type->size;
        }
        type->size = align_to(type->size, type->align);
        return type;
}

static Member *get_struct_member(Type *type, Token *token) {
        for (Member *member = type->members; member; member = member->next) 
                if (member->name->len == token->len &&
                                !strncmp(member->name->loc, token->loc, token->len))
                        return member;
        error_tok(token, "no such member");
}

static Node *struct_ref(Node *left, Token *token) {
        add_type(left);
        if (left->type->kind != TY_STRUCT && left->type->kind != TY_UNION)
                error_tok(left->token, "not a struct nor a union");

        Node *node = new_unary(ND_MEMBER, left, token);
        node->member = get_struct_member(left->type, token);
        return node;
}

// Convert A++ to `(typeof A)((A += 1) - 1)`
static Node *new_inc_dec(Node *node, Token *token, int addend) {
        add_type(node);
        return new_cast(new_add(to_assign(new_add(node, new_num(addend, token), token)),
                                new_num(-addend, token), token),
                        node->type);
}

// postfix = primary ("[" expr "]" | "." ident | "->" ident | "++" | "--")*
static Node *postfix(Token **rest, Token *token) {
        Node *node = primary(&token, token);

        for (;;) {
                if (equal(token, "[")) {
                        // x[y] is short for *(x + y)
                        Token *start = token;
                        Node *index = expr(&token, token->next);
                        token = skip(token, "]");
                        node = new_unary(ND_DEREF, new_add(node, index, start), start);
                        continue;
                }

                if (equal(token, ".")) {
                        node = struct_ref(node, token->next);
                        token = token->next->next;
                        continue;
                }

                if (equal(token, "->")) {
                        // x->y is short for (*x).y
                        node = new_unary(ND_DEREF, node, token);
                        node = struct_ref(node, token->next);
                        token = token->next->next;
                        continue;
                }

                if (equal(token, "++")) {
                        node = new_inc_dec(node, token, 1);
                        token = token->next;
                        continue;
                }

                if (equal(token, "--")) {
                        node = new_inc_dec(node, token, -1);
                        token = token->next;
                        continue;
                }

                *rest = token;
                return node;
        } 
}

// funcall = ident "(" (assign ("," assign)*)? ")"
static Node *funcall(Token **rest, Token *token) {
        Token *start = token;
        token = token->next->next;

        var_scope *sc = find_var(start);
        if (!sc)
                error_tok(start, "implicit declaration of a function");
        if (!sc->var || sc->var->type->kind != TY_FUNC)
                error_tok(start, "not a function");

        Type *type = sc->var->type;
        Type *param_type = type->params;

        Node head = {};
        Node *cur = &head;

        while (!equal(token, ")")) {
                if (cur != &head)
                        token = skip(token, ",");

                Node *arg = assign(&token, token);
                add_type(arg);

                if (param_type) {
                        if (param_type->kind == TY_STRUCT || param_type->kind == TY_UNION)
                                error_tok(arg->token, "passing struct or union is not supported yet");
                        arg = new_cast(arg, param_type);
                        param_type = param_type->next;
                }
                cur = cur->next = arg;
        }

        *rest = skip(token, ")");

        Node *node = new_node(ND_FUNCALL, start);
        node->funcname = strndup(start->loc, start->len);
        node->func_type = type;
        node->type = type->return_type;
        node->args = head.next;
        return node;
}

// primary = "(" "{" stmt+ "}" ")"
//         | "(" expr ")"
//         | "sizeof" "(" type-name ")"
//         | "sizeof" unary
//         | funcall
//         | str
//         | num
static Node *primary(Token **rest, Token* token) {
        Token *start = token;

        if (equal(token, "(") && equal(token->next, "{")) {
                Node *node = new_node(ND_STATEMENT_EXPRESSION, token);
                node->body = compound_statement(&token, token->next->next)->body;
                *rest = skip(token, ")");
                return node;
        }
        if (equal(token, "(")) {
                Node *node = expr(&token, token->next);
                *rest = skip(token, ")");
                return node;
        }

        if (equal(token, "sizeof") && equal(token->next, "(") && is_typename(token->next->next)) {
                Type *type = typename(&token, token->next->next);
                *rest = skip(token, ")");
                return new_num(type->size, start);
        }

        if (equal(token, "sizeof")) {
                Node *node = unary(rest, token->next);
                add_type(node);
                return new_num(node->type->size, token);
        }

        if (token->token_type == T_IDENT) {
                // Function call
                if (equal(token->next, "(")) {
                        return funcall(rest, token);
                }

                // Verify that there is not a variable already created with this name
                // or enum constant
                var_scope *sc = find_var(token);
                if (!sc || (!sc->var && !sc->enum_type))
                        error_tok(token, "undefined variable");

                Node *node;
                if (sc->var)
                        node = new_var_node(sc->var, token);
                else
                        node = new_num(sc->enum_val, token);

                *rest = token->next;
                return node;
        }

        if (token->token_type == T_STR) {
                Obj *var = new_string_literal(token->str, token->type);
                *rest = token->next;
                return new_var_node(var, token);
        }

        if (token->token_type == T_NUM) {
                Node *node = new_num(token->val, token);
                *rest = token->next;
                return node;
        } 
        error_tok(token, "expected an expression");
        return NULL; // It will exit before here
}

static Token *parse_typedef(Token *token, Type *basetype) {
        bool first = true;

        while (!consume(&token, token, ";")) {
                if (!first)
                        token = skip(token, ",");
                first = false;
                Type *type = declarator(&token, token, basetype);
                push_scope(get_ident(type->name))->type_def = type;
        }
        return token;
}

static void create_param_lvars(Type *param) {
        if (param) {
                create_param_lvars(param->next);
                new_lvar(get_ident(param->name), param);
        }
}

static Token *function(Token *token, Type *basetype, var_attribute *attribute) {
        Type *type = declarator(&token, token, basetype);

        Obj *func = new_gvar(get_ident(type->name), type);
        func->is_function = true;
        func->is_definition = !consume(&token, token, ";");
        func->is_static = attribute->is_static;

        if (!func->is_definition)
                return token;

        current_func = func;

        locals = NULL;
        enter_scope();

        create_param_lvars(type->params);
        func->params = locals;

        token = skip(token, "{");
        func->body = compound_statement(&token, token);
        func->locals = locals;
        leave_scope();
        return token;
}

static Token *global_variable(Token *token, Type *basetype) {
        bool first = true;

        while (!consume(&token, token, ";")) {
                if (!first)
                        token = skip(token, ",");
                first = false;

                Type *type = declarator(&token, token, basetype);
                new_gvar(get_ident(type->name), type);
        }
        return token;
}

// Lookahead tokens and returns true if a given token is start
// of a function definition or declaration
static bool is_function(Token *token) {
        if (equal(token->next, ";"))
                return false;

        Type dummy = {};
        Type *type = declarator(&token, token, &dummy);
        return type->kind == TY_FUNC;
}


// program = (typedef | function-definition | global-variable)*
Obj *parse(Token *token) {
        globals = NULL;

        while (token->token_type != T_EOF) {
                var_attribute attribute = {};
                Type *basetype = declaration_specifier(&token, token, &attribute);

                // Typedef
                if (attribute.is_typedef) {
                        token = parse_typedef(token, basetype);
                        continue;
                }

                // Function
                if (is_function(token)) {
                        token = function(token, basetype, &attribute);
                        continue;
                }

                // Global variable
                token = global_variable(token, basetype);
        }
        return globals;
}
