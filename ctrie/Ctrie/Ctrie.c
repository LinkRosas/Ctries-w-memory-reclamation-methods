#include "Ctrie.h"
#include "Nodes.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>

#define CAS(p1,p2,p3) __sync_bool_compare_and_swap(&p1,p2,p3)


//##############CREATE CTRIE######## 

Ctrie* new_ctrie(){
  Ctrie *ctrie = malloc(sizeof(Ctrie));
  ctrie->root= new_anode(16);
  return ctrie;
}


//#############LOOKUP############### 

size_t lookup(size_t key, Ctrie *ct ){
  return lookup_internal(key, key, 0, ct->root);
}


size_t lookup_internal(size_t key,size_t hash, int l, ANode *cur){
  int pos= POS(hash,l,cur->size); 
  void *oldtg= cur->array[pos]; 
  int type = GET_TAG(oldtg); 
  if(oldtg==NULL){ //if node == NULL, return 0 (key not found)
    return 0;
  }
  else if(type==ANODE){ //if ANode, continue search
    return lookup_internal(key,hash,l+1,(ANode*)UNTAG(oldtg));
  }
  else if(type==SNODE){ //if SNode, if same key, return value.
    SNode *old=(SNode*)UNTAG(oldtg);
   
    if(old->key==key)
      return old->val;
    else
      return 0;//key not found
    
  }
  else if(type==ENODE){ //if ENode, continue search using old version
    ANode *an=(ANode*)UNTAG(((ENode*)UNTAG(oldtg))->narrow); 
    return lookup_internal(key,hash,l+1,an);
  }/*
  else if(type==FROZEN){ //if FNode, se SNode e same key return value, se ANode continue 
    FNode *old =(FNode*)UNTAG(oldtg);
    if(old->frozen==NULL)
      return 0;//key not found
    
    else if(GET_TAG(old->frozen)==ANODE){
      return lookup_internal(key,hash,l+1,(ANode*)UNTAG(old->frozen));
    }
    }*/
  else if(type==FROZEN){
    if(UNTAG(oldtg)!=NULL)
      return lookup_internal(key, hash, l+1, (ANode*)UNTAG(oldtg));
    else
      return 0;
  }
  return -1; //why -1????
}


//see if ANODE all null 
int all_null(ANode* cur){
  for(int i=0; i<cur->size; i++){
    /*if(GET_TAG(cur->array[i])==FROZEN){
      FNode *fn = (FNode*)cur->array[i];
      if(fn->frozen!=NULL)
	return 0;
	}*/
    if(GET_TAG(cur->array[i])==FROZEN){
      if(UNTAG(cur->array[i])!=NULL)
	return 0;
    }
    else if(cur->array[i]!=NULL)
      return 0;
  }
  return 1;
}


//####################INSERT################## 
//return 0 to reenter insert, -1 to full Ctrie

int insert(size_t key, size_t val, Ctrie* ct){ 
  /* int res= insert_internal(key, val, key, 0,ct->root, NULL );
  if(res==0){
    res=insert(key, val, ct);
  }
  else if(res==-1)
    printf("CTRIE is full\n");
  */
  if(!insert_internal(key, val, key, 0, ct->root, NULL))
    insert(key, val, ct);
  return 1;
}

//insert a snode , return 1 se ok, 0 se falhou
int insert_snode(size_t key, size_t val, void *oldtg, ANode *cur, int pos){
  SNode *sn =new_snode(key,val,NULL);
  void *sntg = TAG(sn,SNODE);
  if(CAS(cur->array[pos], oldtg, sntg)){
    return 1;
  }
  else{  //se CAS falhou
    free(sn);
    printf("FAILED THE CAS ONE\n");
    return 0;
  }
  return 0;
}


//insert an enode and perform expansion          
ANode* replace_by_enode(size_t hash, int l, ANode *cur,ANode *prev){
  int ppos= POS(hash,(l-1),prev->size); 
  ENode* en=new_enode(prev, ppos, cur, hash,l, NULL);	
  void *entg = TAG(en,ENODE);
  void *curtg= TAG(cur,ANODE);
  if(CAS(prev->array[ppos],curtg, entg)){ //Assign enode to ctrie
    complete_expansion(en); 
    ANode *wide = en->wide;
    return wide; 
  }
  else{ // if enode never assigned, free
    free(en);
    return cur; 
  }
}

//return 0 to reenter, -1 to full                
int insert_internal(size_t key,size_t val,size_t hash,int l,ANode *cur,ANode *prev){ 
  //if(l==MAX_LEVEL) 
    //return -1;
  int pos= POS(hash,l,cur->size);
  
  void *oldtg=cur->array[pos];
  int type=GET_TAG(oldtg);
  if(oldtg==NULL){
    if(insert_snode(key,val, cur->array[pos], cur, pos))
      return 1;
    else
      return insert_internal(key,val,hash,l,cur,prev);
  }
  else if(type==ANODE){ //if ANode, continue
    return insert_internal(key,val,hash,l+1,(ANode*)UNTAG(oldtg),cur);
  }
  else if(type==SNODE){ //if snode, ver txn
    SNode *old =(SNode*)UNTAG(oldtg);
    int txn_type=GET_TAG(old->txn);
    if(old->txn==NULL){ //if txn==NULL and same key, replace snode
      if(old->key==key){
	void *sn=new_snode(key,val,NULL);
	void *sntg= TAG(sn,SNODE); 
	if( CAS(old->txn, NULL, sntg)){ //assign snode to txn
	  CAS(cur->array[pos],oldtg, sntg);
	  return 1;
	}
	else{ //if snode not assigned, free
	  free(sn);
	  return insert_internal(key,val,hash,l,cur,prev);
	}
      }	
      else if(cur->size==4){ //not same key and narrow, expand 
	ANode *an = replace_by_enode(hash,l,cur,prev);
	return insert_internal(key,val,hash,l,an,prev);
      }
      else{ //if not same key and  wide, new level               //CREATE NEW LEVEL 4size JUST 1 node and reenter 
	ANode *an=anode_from_snode(old->key, old->val, l+1);
	if(CAS(old->txn, NULL, an)){
	  CAS(cur->array[pos], oldtg, an);
	  return 0;
	}
	else{
	  free(an);
	  return insert_internal(key,val,hash,l,cur,prev);
	}
	  
	  

      }
    }
    else if(txn_type==FROZEN){
      return 0;
    }
    else if(txn_type==ANODE || txn_type==SNODE){
      CAS(cur->array[pos],oldtg,old->txn);
      return insert_internal(key,val,hash,l,cur,prev);
    }
  }
  else if(type==FROZEN){ //if FNODE,if in compress situation, should unfrozan to insert and ignore compression??
    /*if(all_null(cur) && l>0 ){
    compress(cur,prev,hash,l);
    }*/
    return 0;
  }
  else if(type==ENODE){//
    complete_expansion((ENode*)UNTAG(oldtg));
  }
  return 0;      
}



//###################COMPLETE_EXPANSION############# 
//Perform expansion from 4 slots anode to 16 

void complete_expansion(ENode *en){
  freeze(en->narrow);
  ANode *wide = new_anode(16);
  set_wide(en->narrow, wide, en->level); //copy nodes in narrow, recalc pos and assign in wide 
  if(!CAS(en->wide,NULL,wide)){ // if wide already assigned
    wide = en->wide;  //read wide
  }
  int ppos=en->parent_pos;
  void *entg= TAG(en,ENODE);
  void *wtg= TAG(wide,ANODE);
  CAS(en->parent->array[ppos],entg, wtg); //if wide not assigned to ctrie
    //free(wide); //N posso dar free aqui!!wide ja faz parte da struct! TAva a dar segfault
  
}




//####################FREEZE################### 

void freeze(ANode* cur){
  int i=0;
  while(i<cur->size){
    void *oldtg =cur->array[i];
    int type=GET_TAG(oldtg);
    if(oldtg==NULL){ //if NULL, tag as frozen 
      void *fntg= TAG(NULL,FROZEN);
      if(!CAS(cur->array[i],oldtg,fntg)){//if dont tag ,retry
	i--;
      }
    }
    else if(type==SNODE){ //if SNode, se txn NULL, tag txn
      SNode *sn =(SNode*)UNTAG(oldtg);
      if(sn->txn==NULL){
	void *fntg= TAG(NULL,FROZEN);
	if(!CAS(sn->txn,NULL,fntg)){//if dont tag txn ,retry
	  i--;
	}
      }
      else if(GET_TAG(sn->txn)!=FROZEN){ //if txn!=NULL, make change and retry
	CAS(cur->array[i],oldtg,sn->txn);
	i--;
	}
    }
    else if(type==ANODE){
      void *fntg = TAG(oldtg, FROZEN);
      printf("MERDA!!!!\n\n\n");
      if(!CAS(cur->array[i], oldtg, fntg))
	i--;
    }
    else if(type==ENODE){
      ENode *enode= UNTAG(oldtg);
      complete_expansion(enode);
      i--;
    }
    i++;
  }
 
}




//##############REMOVE########################


int remover(size_t key, Ctrie *ct){
  int res= remove_key(key,key,0,ct->root,NULL);
  if(res==-1){
    return remover(key, ct);
  }
  return res;
}

//return -1 to reenter
int remove_key(size_t key, size_t hash, int l, ANode* cur, ANode* prev){
  if(all_null(cur) && l>0){
    compress(cur,prev,hash,l);
    return -1;
  }
  int pos= POS(hash,l,cur->size);
  void *oldtg= cur->array[pos];
  int type = GET_TAG(oldtg);
  if(oldtg==NULL)
    return 0;
  else if(type==ANODE){
    return remove_key(key, hash, (l+1), UNTAG(oldtg), cur);
  }
  else if(type==SNODE){ 
    SNode *old =(SNode*)UNTAG(oldtg);
    int txn_type=GET_TAG(old->txn);
    if(old->key == key){
      if(old->txn==NULL){ 
	CAS(cur->array[pos], oldtg, NULL);
	return remove_key(key, hash,l,cur,prev);//fazer compress
      }
      else if(txn_type==FROZEN)//SNODEFROZEN
	return -1;
      else{//if txn_type==ANODE,SNODE,ENODE
	CAS(cur->array[pos], oldtg, old->txn);
	return remove_key(key,hash,l,cur,prev);
      }
    }
    else
      return 0;
  }
  else if(type==ENODE){
    complete_expansion(UNTAG(cur->array[pos]));
    return remove_key(key, hash, l, cur, prev);//-1??
  }
  
  else if(type==FROZEN){ 
    //FNode *fn = (FNode*)UNTAG(oldtg);
    if(UNTAG(oldtg)==NULL)
      return 0;
    return -1;
    }
  return 1;
  
}

void compress(ANode *cur, ANode *prev, size_t hash, int l){
    freeze(cur);
    if(all_null(cur)){
      int ppos = POS(hash, (l-1), prev->size);
      //void *curtg= TAG(cur, ANODE);
      CAS(prev->array[ppos],cur, NULL);
    }
    else{
      unfrozAN(cur);
    }  
}




//##################PRINT#####################

void print(Ctrie *ct){
  printf("___________________CTRIE_________________\n");
  print_trie(ct->root,0);
  printf("___________________END___________________\n");
}



  
void print_trie(ANode *cur, int l){
  for(int i=0; i<cur->size; i++){
    for(int x =0; x<l; x++)
      printf("  ");
    int type = GET_TAG(cur->array[i]);
    if(UNTAG(cur->array[i])==NULL)
      printf("NULL\n");
    else if(type==SNODE){
      SNode *snode=(SNode*)UNTAG(cur->array[i]); 
      printf("p:%d SNODE: k=%lu, v=%lu, txn:%p\n",i,snode->key, snode->val, snode->txn);
    }
    else if(type==ANODE){
      printf("p:%d ANODE:\n", i);
      print_trie((ANode*)UNTAG(cur->array[i]), l+1);
    }
    else{
      printf("type_node= %d\n", type);
      if(type==FROZEN){
	printf("frozen:%p, type_frozen:%d \n", ((FNode*)UNTAG(cur->array[i]))->frozen, GET_TAG(((FNode*)UNTAG(cur->array[i]))->frozen));
      }
    }
  }
}




/*
void* get_onlynode(ANode *cur){
  int pos= get_onlypos(cur);
  if(pos!=-1){
    if(GET_TAG(cur->array[pos])==FROZEN){
      return ((FNode*)UNTAG(cur->array[pos]))->frozen;
    }
    else{
      SNode *sn=(SNode*)UNTAG(cur->array[pos]);
      void *snode = new_snode(sn->key,sn->val, NULL);
      return snode;
    }
  }
  return NULL;
}


void my_compress(ANode *cur, ANode *prev, int hash, int l){
  freeze(cur);
  int ppos= POS(hash, (l-1), prev->size);
  void *curtg= TAG(cur,ANODE);
  void* node= get_onlynode(cur);
  CAS(prev->array[ppos], curtg, node);
}
*/

/*
int get_onlypos(ANode* cur){
  for(int i=0; i<cur->size; i++){
    if(cur->array[i]!=NULL){
      if(GET_TAG(cur->array[i])==FROZEN){
	if(((FNode*)cur->array[i])->frozen!=NULL)
	  return i;
      }
      else if(GET_TAG(cur->array[i])==SNODE){
	if(GET_TAG(((SNode*)cur->array[i])->txn)==FROZEN)
	  return i;
      }	
    }
  }
  return -1; //se nao tiver frozen, algo aconteceu
}
*/

