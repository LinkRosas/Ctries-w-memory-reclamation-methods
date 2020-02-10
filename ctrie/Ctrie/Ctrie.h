#include "Nodes.h"

#ifndef CTRIE_H_INCLUDED
#define CTRIE_H_INCLUDED
#define MAX_LEVEL 8

typedef struct Ctrie{
  ANode *root;
}Ctrie;

Ctrie* new_ctrie();
size_t lookup(size_t key, Ctrie *ct);
size_t lookup_internal(size_t key, size_t hash, int level, ANode *cur);
int insert_internal(size_t key,size_t val, size_t hash, int level, ANode *cur, ANode *prev);
int insert_snode(size_t key, size_t val, void *oldt, ANode *cur, int pos);
int insert(size_t key, size_t val, Ctrie *ct);
int remover(size_t key, Ctrie *ct);
int remove_key(size_t key, size_t hash, int level, ANode *cur, ANode *prev);
void complete_expansion(ENode *enode);
void freeze(ANode *cur);
void print(Ctrie *ct);
void print_trie(ANode *cur, int l);
void compress(ANode *cur, ANode *prev, size_t hash, int l);

#endif
