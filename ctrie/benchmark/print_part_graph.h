#include <stdio.h>
#include <stdint.h>
//#include <Ctrie.c>

#define IC(x) x
#define BP rep
#define POS(h,l,s) (int)((h>>((l)*4))%s)


unsigned long value;


void print_pnode(void *node,int l, FILE *file);

//??
/*
struct Ctrie_node *ptr(struct lfht_node *next){
  return (struct Ctrie_node *) ((uintptr_t) next & ~((WORD_SIZE/2)-1));
}
*/
//??
/*
unsigned tag(struct Ctrie_node *ptr){
  return ((uintptr_t) ptr & (((WORD_SIZE/2)-2))) >> 1;
}
*/
/*
unsigned flag(struct Ctrie_node *ptr){
  return ((uintptr_t) ptr & 1);
}
*/

const char *pcolor(int val){
  if(val==1)
    return "red";
  else if(val==2)
    return "blue";
  else if(val==3)
    return "black";
  else
    return "green";
}

void print_pANode( ANode *anode , int l,  FILE *file){
  fprintf(
	  file,
	  "\t%llu [label=\"<in>%llx|{%p|%d}",
	  anode,
	  anode,
	  anode->array,
	  anode->size);
  for(size_t i=0; i< anode->size; i++)
    fprintf(file, "|<%d>%d:%p", i,i, anode->array[i]);
  fprintf(file, "\"]\n");
  for(size_t i=0; i< anode->size; i++){
    if(anode->array[i]!=NULL){
      if(i==POS(value,l,anode->size)){
	print_pnode(anode->array[i], l, file);
	fprintf(
		file,
		"\t%llu:%d -> %llu:in [color=%s]\n",
		anode,
		i,
		UNTAG(anode->array[i]),
		pcolor(GET_TAG(anode->array[i]))
		);
      }
    }
  }

}


void print_pSNode( SNode *snode, FILE *file){
  fprintf(
	  file,
	  "\t%llu [label=\"%llx|key:%d|val:%d|txn:%d\"]\n",
	  snode,
	  snode,
	  snode->key,
	  snode->val,
	  snode->txn
	  );
}

void print_pENode( ENode *enode,int l, FILE *file){
  fprintf(
	  file,
	  "\t%llu [label=\"%llx|nar:%p|wid:%p\"]\n",
	  enode,
	  enode,
	  enode->narrow,
	  enode->wide
	  );
  fprintf(
	  file,
	  "\t%llu:out -> %llu:in [color=%s]\n",
	  enode,
	  enode->narrow,
	  pcolor(GET_TAG(enode->narrow))
	  );

   fprintf(
	  file,
	  "\t%llu:out -> %llu:in [color=%s]\n",
	  enode,
	  enode->wide,
	  pcolor(GET_TAG(enode->wide))
	  );
   print_pnode(enode->narrow,l, file);
   print_pnode(enode->wide,l, file);
}

void print_pFNode( FNode *fnode,int l, FILE *file){
  fprintf(
	  file,
	  "\t%llu [label=\"%llx| frozen:%llx\"]\n",
	  fnode,
	  fnode,
	  fnode->frozen
	  );
  fprintf(
	  file,
	  "\t%llu:out -> %llu:in [color=%s]\n",
	  fnode,
	  fnode->frozen,
	  pcolor(GET_TAG(fnode->frozen))
	  );
  print_pnode(fnode->frozen,l, file);
}

void print_pnode(void *node,int l, FILE *file){
  int type=GET_TAG(node);
  if(node==NULL)
    return;
  //print_Null(node, file);
  else if(type== ANODE)
    print_pANode(UNTAG(node), l+1, file);
  else if(type == SNODE)
    print_pSNode(UNTAG(node), file);
  else if(type == ENODE)
    print_pENode(UNTAG(node),l, file);
  else if(type==FROZEN)
    print_pFNode(UNTAG(node), l,file);
  
}

FILE *init_pgraph(){
  FILE *file = fopen("part_graph.dot", "w+");
  fprintf(file, "digraph G {\n\trankdir=LR;\n\tnode [shape=record]\n");
  return file;
}

void end_pgraph(FILE *file){
  fprintf(file, "}");
  fclose(file);
}



__attribute__ ((used)) void print_part_graph(Ctrie *ct, unsigned long val){
  FILE *file = init_pgraph();
  value=val;
  print_pANode(ct->root, 0,file);
  end_pgraph(file);
}
