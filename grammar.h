#ifndef GRAMMAR_H
#define GRAMMAR_H

#define MAXSYM   200
#define MAXPROD  500
#define MAXRHS   50
#define MAXSTR   64
#define MAXTOKS  100

typedef enum { TERMINAL=0, NONTERMINAL=1 } SymType;

typedef struct {
    char name[MAXSTR];
    SymType type;
    int idx;
} Symbol;

typedef struct {
    int lhs;
    int rhs_len;
    int rhs[MAXRHS];
} Production;

extern Symbol syms[MAXSYM];
extern int nsym;

extern Production prods[MAXPROD];
extern int nprod;

int sym_index_by_name(const char *name);
int add_symbol(const char *name, SymType type);
int add_prod(int lhs, int *rhs, int rhs_len);

extern int start_sym;

void build_grammar();

extern char *test_tokens[MAXTOKS];
extern int ntest_tokens;

#endif
