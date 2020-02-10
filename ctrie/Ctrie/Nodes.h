#ifndef NODES_H_INCLUDED
#define NODES_H_INCLUDED

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>

#define USABLE_PTR 0xfffffffffffffffc
#define UNTAG(ptr) (void*)((uintptr_t)ptr & (uintptr_t)USABLE_PTR)
#define TAG(ptr, tag) (void*)((uintptr_t)ptr | (uintptr_t)tag)
#define GET_TAG(ptr) (uint8_t)((uintptr_t)ptr & ~USABLE_PTR)

#define ANODE 0x0
#define SNODE 0x1
#define ENODE 0x2
#define FROZEN 0x3

#define POS(h,l,s) (uint64_t)((h>>((l)*4))%s)


typedef struct SNode{
  size_t hash;
  size_t key;
  size_t val;
  void *txn;
}SNode;


typedef struct ANode{
  void **array;
  int size;
}ANode;

typedef struct FNode{
  void *frozen;
}FNode;


typedef struct ENode{
  ANode *parent;
  int parent_pos;
  ANode *narrow;
  size_t hash;
  int level;
  ANode *wide;
}ENode;

SNode* new_snode(size_t key, size_t val, void *txn);
ANode* new_anode(int size);
ANode* anode_from_snode(size_t k1, size_t v1, int l);
FNode* new_fnode(void *frozen);
ENode* new_enode(ANode *parent, int parent_pos, ANode *narrow, size_t hash, int l, ANode *wide);
void set_wide(ANode *narrow, ANode *wide, int l);
int one_node(ANode *anode);
void unfrozAN(ANode *an);
ANode* unfrozANret(ANode *an);
#endif
