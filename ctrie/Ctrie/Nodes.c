#include "Nodes.h"
#include <stdlib.h>
#include <stdio.h>

#define CAS(p1,p2,p3) __sync_bool_compare_and_swap(&p1,p2,p3)




SNode* new_snode(size_t key, size_t val, void *txn){
  SNode *snode = malloc(sizeof(SNode));
  snode->hash=key;
  snode->key=key;
  snode->val=val;
  snode->txn=txn;
  return snode;
}

ANode* new_anode(int size){
  ANode *anode= malloc(sizeof(ANode));
  anode->array=malloc(sizeof(void*)*size);
  if(!anode->array)
    return NULL;
  for(int i=0; i<size;i++)
    anode->array[i]=NULL;
  anode->size=size;
  return anode;
}

ANode* anode_from_snode(size_t k1, size_t v1, int level){
  SNode *sn1 = new_snode(k1, v1, NULL);
  ANode *an= new_anode(4);
  int pos= POS(sn1->hash, level, 4);
  an->array[pos]=TAG(sn1, SNODE);
  return an;
}


FNode* new_fnode(void *frozen){
  FNode *fnode = malloc(sizeof(FNode));
  fnode->frozen = frozen;
  return fnode;
}

ENode* new_enode(ANode *parent, int ppos, ANode *narrow, size_t hash, int l, ANode *wide){
  ENode *enode = malloc(sizeof(ENode));
  enode->parent=parent;
  enode->parent_pos=ppos;
  enode->narrow=narrow;
  enode->hash=hash;
  enode->level=l;
  enode->wide=NULL; //=wide
  return enode;
}



void set_wide(ANode *narrow,ANode *wide,int l){
  ANode *ufnarrow = unfrozANret(narrow);
  for(int i=0; i<4; i++){
    void *old =ufnarrow->array[i];
    int type=GET_TAG(old);
    if(type==SNODE){
      SNode *sn = (SNode*)UNTAG(old);
      int pos= POS(sn->hash, l, 16);
      sn = TAG(new_snode(sn->key, sn->val, NULL), SNODE);
      wide->array[pos]=sn;
    }
    else
      wide->array[i]=ufnarrow->array[i]; //NAO DEVERIA ACONTECER
  }
}

void unfrozAN(ANode *an){
  for(int i=0; i<an->size; i++){
    void *old=an->array[i];
    int type=GET_TAG(old);
    if(type==FROZEN){
      if(UNTAG(old)==NULL){
	CAS(an->array[i],old, NULL);
      }
      else
	printf("ANODE INSIDE A FROZEN\n");
    }
    else if(type==SNODE){
      SNode *sn =UNTAG(old);
      void *txn =sn->txn;
      if(GET_TAG(txn)==FROZEN)
	CAS(sn->txn, txn, NULL);
    }
  }
}

ANode* unfrozANret(ANode *an){
  ANode *ufan= new_anode(an->size);
  for(int i=0; i<an->size; i++){
    void *old = an->array[i];
    int type=GET_TAG(old);
    if(type==FROZEN){//fn->frozen==NULL?? ou NULL tagged w/ frozen?
      if(UNTAG(old)==NULL)
	ufan->array[i]=NULL;
      else{//SE nao NULL vai ser Anode
	printf("FROZENret %p  s:%d\n",UNTAG(old), ((ANode*)UNTAG(old))->size);
      }
    }
    else if(type==SNODE){
      SNode *sn = (SNode*)UNTAG(old);
      SNode *nsn = TAG(new_snode(sn->key, sn->val, NULL),SNODE);
      ufan->array[i]=nsn;
    }
  }
  return ufan;
}

//verifica se anode apenas tem um no (pode ser comprimido)
int one_node(ANode *anode){
  int c=0;
  for(int i=0; i<anode->size; i++){
    if(anode->array[i]!=NULL){
      if(c==0)
	c++;
      else
	return 0;
    }
  }
  return 1;   
}
