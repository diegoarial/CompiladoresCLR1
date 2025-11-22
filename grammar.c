#include "grammar.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>

Symbol syms[MAXSYM];
int nsym = 0;

Production prods[MAXPROD];
int nprod = 0;

int start_sym = -1;

/* Tokens de teste */
char *test_tokens[MAXTOKS];
int ntest_tokens = 0;

/* Symbol functions */
int sym_index_by_name(const char *name) {
    for (int i=0;i<nsym;i++)
        if (strcmp(syms[i].name,name)==0) return i;
    return -1;
}

int add_symbol(const char *name, SymType type) {
    int i = sym_index_by_name(name);
    if (i>=0) return i;
    strcpy(syms[nsym].name, name);
    syms[nsym].type = type;
    syms[nsym].idx = nsym;
    return nsym++;
}

int add_prod(int lhs, int *rhs, int rhs_len) {
    prods[nprod].lhs = lhs;
    prods[nprod].rhs_len = rhs_len;
    for (int i=0;i<rhs_len;i++) prods[nprod].rhs[i]=rhs[i];
    return nprod++;
}

/* CHANGE YOUR GRAMMAR HERE!! */

void build_grammar() {
    nsym = 0; nprod = 0;

    int id = add_symbol("id", TERMINAL);
    int comma = add_symbol(",", TERMINAL);
    int dollar = add_symbol("$", TERMINAL);

    int Sprime = add_symbol("S'", NONTERMINAL);
    int L = add_symbol("L", NONTERMINAL);

    start_sym = Sprime;

    int rhs[10];
    rhs[0] = L; add_prod(Sprime, rhs, 1);         // S' -> L
    rhs[0] = L; rhs[1] = comma; rhs[2] = id; add_prod(L, rhs, 3); // L -> L , id
    rhs[0] = id; add_prod(L, rhs, 1);            // L -> id

    char *tokens[] = { "id", ",", "id", ",", "id", "$", NULL };
    for (int i=0; tokens[i]; i++) test_tokens[i] = tokens[i];
    ntest_tokens = 6;
}
