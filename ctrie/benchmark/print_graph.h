#include <stdio.h>
#include <stdint.h>
//#include <Ctrie.c>

#define IC(x) x
#define BP rep


void print_node(void *node, FILE *file);

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

const char *color(int val){
  if(val==1)
    return "red";
  else if(val==2)
    return "blue";
  else if(val==3)
    return "black";
  else
    return "green";
}

void print_ANode( ANode *anode, FILE *file){
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
      fprintf(
	      file,
	      "\t%llu:%d -> %llu:in [color=%s]\n",
	      anode,
	      i,
	      UNTAG(anode->array[i]),
	      color(GET_TAG(anode->array[i]))
	      );
      print_node(anode->array[i], file);
    }
  }

}


void print_SNode( SNode *snode, FILE *file){
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

void print_ENode( ENode *enode, FILE *file){
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
	  color(GET_TAG(enode->narrow))
	  );

   fprintf(
	  file,
	  "\t%llu:out -> %llu:in [color=%s]\n",
	  enode,
	  enode->wide,
	  color(GET_TAG(enode->wide))
	  );
  print_node(enode->narrow, file);
  print_node(enode->wide, file);
}

void print_FNode( FNode *fnode, FILE *file){
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
	  color(GET_TAG(fnode->frozen))
	  );
  print_node(fnode->frozen, file);
}

void print_node(void *node, FILE *file){
  int type=GET_TAG(node);
  if(node==NULL)
    return;
  //print_Null(node, file);
  else if(type== ANODE)
    print_ANode(UNTAG(node), file);
  else if(type == SNODE)
    print_SNode(UNTAG(node), file);
  else if(type == ENODE)
    print_ENode(UNTAG(node), file);
  else if(type==FROZEN)
    print_FNode(UNTAG(node), file);
  
}

FILE *init_graph(){
  FILE *file = fopen("graph.dot", "w+");
  fprintf(file, "digraph G {\n\trankdir=LR;\n\tnode [shape=record]\n");
  return file;
}

void end_graph(FILE *file){
  fprintf(file, "}");
  fclose(file);
}



__attribute__ ((used)) void print_graph(Ctrie *ct){
  FILE *file = init_graph();
  print_ANode(ct->root,file);
  end_graph(file);
}
