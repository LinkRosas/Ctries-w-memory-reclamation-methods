#include "Ctrie.h"
#include "Nodes.h"
#include <stdio.h>

#define CAS(p1,p2,p3) __sync_bool_compare_and_swap(&p1,p2,p3)



size_t lookup_debug(size_t key, Ctrie *ct );
size_t lookup_internal_debug(size_t key, size_t hash, int l, ANode *cur);


int insert_debug(size_t key, size_t val, Ctrie* ct){
  printf("INSERT DEBUGGING\n");
  int res= insert_internal_debug(key, val, key, 0,ct->root, NULL );
  if(res==0)
    res=insert_debug(key, val, ct);
  else if(res==-1)
    printf("CTRIE is full\n");
  return 1;
}

int insert_snode_debug(size_t key, size_t val, void *oldtg, ANode *cur, int pos){
  SNode *sn =new_snode(key,val,NULL);
  void *sntg = TAG(sn,SNODE);
  if(CAS(cur->array[pos], oldtg, sntg)){
    printf("SUCESS CAS in %p , pos:%d, %p\n", cur, pos, cur->array[pos]);
    return 1;
  }
  else{  //se CAS falhou
    printf("FAILED CAS\n");
    free(sn);
  }
  return 0;
}





//return 0 to reenter, -1 to full                
int insert_internal_debug(size_t key,size_t val,size_t hash,int l,ANode *cur,ANode *prev){ 
  //if(l==MAX_LEVEL) 
    //return -1;
  int pos= POS(hash,l,cur->size);
  if(cur->array[pos]==NULL){
    printf("SHOULD INSERT in cur:%p at p:%d where is NULL \n", cur, pos);
    if(insert_snode_debug(key,val, cur->array[pos], cur, pos)){
      printf("SUCESS INSERTING SNODE\n");
      printf("lookup:%lu, key:%lu\n",lookup_internal_debug(key,hash, l, cur) ,key);
    }
    else{
      printf("FAILED INSERTING SNODE\n");
      return insert_internal_debug(key, val, hash, l, cur, prev);
    }
    return 1;
  }
  void *oldtg=cur->array[pos];
  int type=GET_TAG(oldtg);
  if(type==ANODE){ 
    printf("FOLLOWING TO cur[%d]:%p\n", pos, cur->array[pos]);
    return insert_internal_debug(key,val,hash,l+1,(ANode*)UNTAG(oldtg),cur);
  }
  else if(type==SNODE){ 
    SNode *old =(SNode*)UNTAG(oldtg);
    int txn_type=GET_TAG(old->txn);
    if(old->txn==NULL){
      if(old->key==key){//TENTAR COM &&old->val!=val
	printf("REPLACING SNODE k:%ld", key);
	if(old->val==val)
	  printf(" w/ same values");
	printf("\n");
	/*
	void *sn=new_snode(key,val,NULL);
	void *sntg= TAG(sn,SNODE); 
	if( CAS(old->txn, NULL, sntg)){ //assign snode to txn
	  CAS(cur->array[pos],oldtg, sntg);
	  return 1;
	}
	else{
	  free(sn);
	  printf("FAIL IN REPLACE\n");
	  return insert_internal_debug(key,val,hash,l,cur,prev);
	  }*/
      }	
      else if(cur->size==4){ //if not same key and narrow, expand
	printf("EXPANSION SITUATION, create ENODE\n");
	//ANode *an = replace_by_enode(hash,l,cur,prev);
	//return insert_internal_debug(key,val,hash,l,an,prev);
	return 1;
      }
      else{ //if not same key and  wide, new level
	printf("NEW LEVEL SITUATION\n");
	ANode *an=anode_from_snode(old->key, old->val,l+1); //create new level anode
	print_trie(an,0);
	printf("SNODE SHOULD BE IN NEW LEVEL\n");
	//void *antg = TAG(an,ANODE);
	//if( CAS(old->txn, NULL, antg)){ //assign anode to txn 
	//CAS(cur->array[pos],oldtg , antg);
	//return 1; 
	//}
	//else{ //if anode not assigned, free
	//free(an);
	//printf("FAIL ASSIGNING NEW LEVEL\n");
	//return insert_internal_debug(key,val,hash,l,cur,prev);
	//}
	return 1; 
      }
    }
    else if(txn_type==FROZEN){
      printf("FOUND A FROZEN SNODE\n");
      return 0;
    }
    else if(txn_type==ANODE || txn_type==SNODE){
      printf("PENDING TXN\n");
      //CAS(cur->array[pos],oldtg,old->txn);
      //return insert_internal_debug(key,val,hash,l,cur,prev);
      return 0;//
    }
  }
  else if(type==FROZEN){ //if FNODE,if in compress situation, compress
    printf("FOUND FNODE ");
    if(all_null(cur) && l>0 ){
      printf(" in compress situation");
      //compress(cur,prev,hash,l);
    }
    printf("\n");
    return 0;
  }
  else if(type==ENODE){//
    printf("FOUND ENODE\n");
    //complete_expansion((ENode*)UNTAG(oldtg));
  }
  return 0;      
}






size_t lookup_debug(size_t key, Ctrie *ct ){
  return lookup_internal_debug(key, key, 0, ct->root);
}


size_t lookup_internal_debug(size_t key, size_t hash, int l, ANode *cur){
  int pos= POS(hash,l,cur->size);
  printf("IN an:%p, pos:%d, find:%p\n", cur, pos, cur->array[pos]);
  //if(l==1)
  //print_trie(cur,1 );
  void *oldtg= cur->array[pos]; 
  int type = GET_TAG(oldtg); 
  if(oldtg==NULL){ //if node == NULL, return 0 (key not found)
    printf("oldtg NULL\n");
    return 0;
  }
  else if(type==ANODE){ //if ANode, continue search
    printf("oldtg ANODE size:%d\n", ((ANode*)oldtg)->size);
    return lookup_internal_debug(key,hash,l+1,(ANode*)UNTAG(oldtg));
  }
  else if(type==SNODE){ //if SNode, if same key, return value.
    SNode *old=(SNode*)UNTAG(oldtg);
    printf("oldtg SNODE k:%lu\n", old->key);
    if(old->key==key)
      return old->val;
    else{
      return 0;//key not found
    }
  }
  else if(type==ENODE){ //if ENode, continue search using old version
    printf("oldtg ENODE\n");
    ANode *an=(ANode*)UNTAG(((ENode*)UNTAG(oldtg))->narrow); 
    return lookup_internal_debug(key,hash,l+1,an);
  }
  else if(type==FROZEN){ //if FNode, se SNode e same key return value, se ANode continue
    printf("oldtg FROZEN\n");
    FNode *old =(FNode*)UNTAG(oldtg);
    if(old->frozen==NULL)
      return 0;//key not found
    
    else if(GET_TAG(old->frozen)==ANODE){
      return lookup_internal_debug(key,hash,l+1,(ANode*)UNTAG(old->frozen));
    }
    else
      printf("FUCK LOOKUP\n");
  }
  return -1; //why -1????
}


int is_present(size_t key, Ctrie *ct){
  return lookup_is_present(key, ct->root, 0);
}



  
int lookup_is_present(size_t key,ANode *cur, int l){
  for(int i=0; i<cur->size; i++){
    int type = GET_TAG(cur->array[i]);
    if(type==SNODE){
      SNode *snode=(SNode*)UNTAG(cur->array[i]); 
      if(snode->key==key)
	return 1;
    }
    else if(type==ANODE && cur->array[i]!=NULL){
      if(lookup_is_present(key,(ANode*)UNTAG(cur->array[i]), l+1))
	return 1;
    }
    else if(type==ENODE){
      if(lookup_is_present(key, ((ENode*)UNTAG(cur->array[i]))->narrow, l+1))
	return 1;
    }
    else if(type==FROZEN){
      if(lookup_is_present(key, ((FNode*)UNTAG(cur->array[i]))->frozen, l+1))
	return 1;
      
      
    }
    return 0;
  }
}

