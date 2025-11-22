#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "grammar.h"

#define MAXITEMSETS 1000
#define MAXITEMS    2000
#define MAXRHS      50
#define MAX_TERMS   60

typedef struct {
    int prod;
    int dot;
    uint64_t lookahead;
} Item;

typedef struct {
    Item *items;
    int n;
} ItemSet;

uint64_t FIRST_sym[MAXSYM];
int nullable[MAXSYM];

int term_offset[MAXSYM];
int term_count = 0;

static inline void bs_set(uint64_t *b, int pos){ *b |= (1ULL<<pos); }
static inline int  bs_test(uint64_t b, int pos){ return (b>>pos)&1ULL; }
static inline void bs_or(uint64_t *a, uint64_t b){ *a |= b; }

void assign_terminal_offsets() {
    term_count = 0;
    for (int i=0;i<nsym;i++) {
        if (syms[i].type == TERMINAL) {
            term_offset[i] = term_count++;
        } else term_offset[i] = -1;
    }
    for (int i=0;i<nsym;i++) {
        FIRST_sym[i] = 0;
        nullable[i] = 0;
        if (syms[i].type == TERMINAL) {
            bs_set(&FIRST_sym[i], term_offset[i]);
        }
    }
}

uint64_t FIRST_of_seq(int *seq, int len);

void compute_nullable_and_first() {
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int p=0;p<nprod;p++) {
            int A = prods[p].lhs;
            int all_nullable = 1;
            for (int i=0;i<prods[p].rhs_len;i++) {
                int s = prods[p].rhs[i];
                if (!nullable[s]) { all_nullable = 0; break; }
            }
            if (all_nullable && !nullable[A]) {
                nullable[A] = 1;
                changed = 1;
            }

            uint64_t before = FIRST_sym[A];
            for (int i=0;i<prods[p].rhs_len;i++) {
                int s = prods[p].rhs[i];
                bs_or(&FIRST_sym[A], FIRST_sym[s]);
                if (!nullable[s]) break;
            }
            if (before != FIRST_sym[A]) changed = 1;
        }
    }
}

uint64_t FIRST_of_seq(int *seq, int len) {
    uint64_t res = 0;
    for (int i=0;i<len;i++) {
        int s = seq[i];
        bs_or(&res, FIRST_sym[s]);
        if (!nullable[s]) return res;
    }
    return res;
}

ItemSet itemsets[MAXITEMSETS];
int nitemsets = 0;

void itemset_add(ItemSet *is, Item it) {
    for (int i=0;i<is->n;i++)
        if (is->items[i].prod == it.prod && is->items[i].dot == it.dot
            && is->items[i].lookahead == it.lookahead) return;

    is->items = realloc(is->items, sizeof(Item)*(is->n+1));
    is->items[is->n++] = it;
}

int item_equal(const Item *a, const Item *b) {
    return a->prod==b->prod && a->dot==b->dot && a->lookahead==b->lookahead;
}

int itemset_equal(ItemSet *A, ItemSet *B) {
    if (A->n != B->n) return 0;
    for (int i=0;i<A->n;i++) {
        int f = 0;
        for (int j=0;j<B->n;j++)
            if (item_equal(&A->items[i], &B->items[j])) { f=1; break; }
        if (!f) return 0;
    }
    return 1;
}

void closure(ItemSet *I) {
    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i=0;i<I->n;i++) {
            Item it = I->items[i];
            Production *p = &prods[it.prod];

            if (it.dot >= p->rhs_len) continue;
            int B = p->rhs[it.dot];
            if (syms[B].type == TERMINAL) continue;

            int beta_len = p->rhs_len - (it.dot+1);
            int beta[MAXRHS];
            for (int k=0;k<beta_len;k++) beta[k] = p->rhs[it.dot+1+k];

            uint64_t first_beta = FIRST_of_seq(beta, beta_len);
            int beta_nullable = 1;
            for (int k=0;k<beta_len;k++) if (!nullable[beta[k]]) { beta_nullable=0; break; }

            for (int q=0;q<nprod;q++) if (prods[q].lhs == B) {
                Item newit = { q, 0, first_beta };
                if (beta_nullable) newit.lookahead |= it.lookahead;

                int merged = 0;
                for (int z=0; z<I->n; z++) {
                    Item *e = &I->items[z];
                    if (e->prod==newit.prod && e->dot==0) {
                        uint64_t before = e->lookahead;
                        e->lookahead |= newit.lookahead;
                        if (e->lookahead != before) changed = 1;
                        merged = 1;
                        break;
                    }
                }
                if (!merged) {
                    itemset_add(I, newit);
                    changed = 1;
                }
            }
        }
    }
}

ItemSet goto_op(ItemSet *I, int X) {
    ItemSet J = {NULL,0};
    for (int i=0;i<I->n;i++) {
        Item it = I->items[i];
        Production *p = &prods[it.prod];
        if (it.dot < p->rhs_len && p->rhs[it.dot]==X) {
            Item moved = it;
            moved.dot++;
            itemset_add(&J, moved);
        }
    }
    closure(&J);
    return J;
}

void build_canonical_collection() {
    nitemsets = 0;

    ItemSet I0={NULL,0};
    Item init = {0,0,0};

    int dollar = sym_index_by_name("$");
    bs_set(&init.lookahead, term_offset[dollar]);

    itemset_add(&I0, init);
    closure(&I0);

    itemsets[nitemsets++] = I0;

    int changed = 1;
    while (changed) {
        changed = 0;
        for (int i=0;i<nitemsets;i++)
            for (int X=0; X<nsym; X++) {
                ItemSet J = goto_op(&itemsets[i], X);
                if (J.n == 0) { if (J.items) free(J.items); continue; }

                int exists = -1;
                for (int k=0;k<nitemsets;k++)
                    if (itemset_equal(&itemsets[k], &J)) { exists = k; break; }

                if (exists == -1) {
                    itemsets[nitemsets++] = J;
                    changed = 1;
                } else if (J.items) free(J.items);
            }
    }
}

typedef enum { ACT_NONE=0, ACT_SHIFT=1, ACT_REDUCE=2, ACT_ACCEPT=3 } ActType;
typedef struct { ActType type; int val; } ActionCell;

ActionCell ACTION[MAXITEMSETS][MAX_TERMS];
int GOTOtab[MAXITEMSETS][MAXSYM];

void build_action_goto() {
    for (int i=0;i<nitemsets;i++){
        for (int t=0;t<term_count;t++) ACTION[i][t].type = ACT_NONE;
        for (int X=0;X<nsym;X++) GOTOtab[i][X] = -1;
    }

    for (int s=0;s<nitemsets;s++)
        for (int X=0;X<nsym;X++) {
            ItemSet J = goto_op(&itemsets[s], X);
            if (J.n > 0) {
                for (int k=0;k<nitemsets;k++)
                    if (itemset_equal(&itemsets[k], &J)) { GOTOtab[s][X] = k; break; }
            }
            if (J.items) free(J.items);
        }

    for (int s=0;s<nitemsets;s++) {
        ItemSet *I = &itemsets[s];

        for (int i=0;i<I->n;i++) {
            Item it = I->items[i];
            Production *p = &prods[it.prod];

            if (it.dot < p->rhs_len) {
                int a = p->rhs[it.dot];
                if (syms[a].type == TERMINAL) {
                    int t = term_offset[a];
                    int to = GOTOtab[s][a];
                    if (to==-1) continue;

                    if (ACTION[s][t].type == ACT_NONE) {
                        ACTION[s][t].type = ACT_SHIFT;
                        ACTION[s][t].val  = to;
                    }
                }
            } else {
                if (it.prod == 0) {
                    for (int b=0;b<term_count;b++)
                        if (bs_test(it.lookahead,b)) {
                            ACTION[s][b].type = ACT_ACCEPT;
                        }
                } else {
                    for (int b=0;b<term_count;b++)
                        if (bs_test(it.lookahead,b)) {
                            ACTION[s][b].type = ACT_REDUCE;
                            ACTION[s][b].val  = it.prod;
                        }
                }
            }
        }
    }
}

void parse_input(char **tokens, int ntok) {
    printf("\nParsing: ");
    for (int i=0;i<ntok;i++) printf("%s ", tokens[i]);
    printf("\n");

    int stack[1000], top=0;
    stack[top++] = 0;
    int ip=0;

    while (1) {
        int state = stack[top-1];

        int sym = sym_index_by_name(tokens[ip]);
        int t = term_offset[sym];
        ActionCell ac = ACTION[state][t];

        if (ac.type==ACT_SHIFT) {
            printf("Shift %s -> %d\n", tokens[ip], ac.val);
            stack[top++] = ac.val;
            ip++;
        }
        else if (ac.type==ACT_REDUCE) {
            Production *p = &prods[ac.val];
            printf("Reduce %s -> ...\n", syms[p->lhs].name);

            top -= p->rhs_len;
            int newState = GOTOtab[stack[top-1]][p->lhs];
            stack[top++] = newState;
        }
        else if (ac.type==ACT_ACCEPT) {
            printf("ACCEPT\n");
            return;
        }
        else {
            printf("ERROR at token %s\n", tokens[ip]);
            return;
        }
    }
}

int main() {
    build_grammar();

    assign_terminal_offsets();
    compute_nullable_and_first();

    build_canonical_collection();
    build_action_goto();

    parse_input(test_tokens, ntest_tokens);

    for (int i=0;i<nitemsets;i++)
        if (itemsets[i].items) free(itemsets[i].items);

    return 0;
}