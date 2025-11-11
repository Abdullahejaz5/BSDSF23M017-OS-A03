#include "shell.h"

/* Simple linked list of variables */
typedef struct Var {
    char *name;
    char *value;
    struct Var *next;
} Var;

static Var *var_head = NULL;

void set_var(const char *name, const char *value) {
    if (!name) return;
    Var *cur = var_head;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            free(cur->value);
            cur->value = strdup(value ? value : "");
            return;
        }
        cur = cur->next;
    }
    Var *n = malloc(sizeof(Var));
    n->name = strdup(name);
    n->value = strdup(value ? value : "");
    n->next = var_head;
    var_head = n;
}

char *get_var(const char *name) {
    Var *cur = var_head;
    while (cur) {
        if (strcmp(cur->name, name) == 0) return cur->value;
        cur = cur->next;
    }
    return NULL;
}

void print_vars(void) {
    Var *cur = var_head;
    while (cur) {
        printf("%s=%s\n", cur->name, cur->value);
        cur = cur->next;
    }
}

void free_all_vars(void) {
    Var *cur = var_head;
    while (cur) {
        Var *tmp = cur;
        cur = cur->next;
        free(tmp->name);
        free(tmp->value);
        free(tmp);
    }
    var_head = NULL;
}

/* Detect simple assignment VARNAME=value with no spaces around '=' */
int handle_variable_assignment(const char *line) {
    if (!line) return 0;
    const char *eq = strchr(line, '=');
    if (!eq) return 0;
    if (eq == line) return 0; /* no name */

    /* ensure there are no spaces or tabs around '=' (very simple rule) */
    for (const char *p = line; p < eq; ++p) {
        if (!(isalnum((unsigned char)*p) || *p == '_')) return 0;
    }
    for (const char *p = eq + 1; *p; ++p) {
        /* allow anything in value, but we'll accept quotes later */
    }

    size_t nlen = eq - line;
    char *name = strndup(line, nlen);
    char *value = strdup(eq + 1);

    /* strip surrounding quotes if present */
    size_t vlen = strlen(value);
    if (vlen >= 2 && ((value[0] == '"' && value[vlen-1] == '"') || (value[0] == '\'' && value[vlen-1] == '\''))) {
        char *tmp = strndup(value + 1, vlen - 2);
        free(value);
        value = tmp;
    }

    set_var(name, value);
    free(name);
    free(value);
    return 1;
}

/* Expand tokens: $NAME and ${NAME} and embedded $NAME inside token */
static char *expand_token(const char *tok) {
    if (!tok) return NULL;
    if (!strchr(tok, '$')) return strdup(tok);

    char out[MAX_LEN];
    out[0] = '\0';
    const char *p = tok;
    while (*p) {
        if (*p != '$') {
            size_t l = strlen(out);
            if (l + 1 < sizeof(out)) {
                out[l] = *p;
                out[l+1] = '\0';
            }
            p++;
            continue;
        }
        /* found $ */
        p++;
        if (*p == '{') {
            p++;
            char name[256]; int i=0;
            while (*p && *p != '}' && i < 250) name[i++] = *p++;
            if (*p == '}') p++;
            name[i] = '\0';
            char *val = get_var(name);
            if (val) strncat(out, val, sizeof(out)-strlen(out)-1);
        } else {
            char name[256]; int i=0;
            while (*p && (isalnum((unsigned char)*p) || *p == '_') && i < 250) name[i++] = *p++;
            name[i] = '\0';
            if (i == 0) continue;
            char *val = get_var(name);
            if (val) strncat(out, val, sizeof(out)-strlen(out)-1);
        }
    }
    return strdup(out);
}

void expand_command_tokens(Command *cmd) {
    if (!cmd) return;
    for (int i = 0; i < MAXARGS && cmd->argv[i]; ++i) {
        char *exp = expand_token(cmd->argv[i]);
        free(cmd->argv[i]);
        cmd->argv[i] = exp;
    }
    for (int i = 0; i < MAXARGS && cmd->pipe_argv[i]; ++i) {
        char *exp = expand_token(cmd->pipe_argv[i]);
        free(cmd->pipe_argv[i]);
        cmd->pipe_argv[i] = exp;
    }
    if (cmd->infile) {
        char *exp = expand_token(cmd->infile);
        free(cmd->infile); cmd->infile = exp;
    }
    if (cmd->outfile) {
        char *exp = expand_token(cmd->outfile);
        free(cmd->outfile); cmd->outfile = exp;
    }
}
