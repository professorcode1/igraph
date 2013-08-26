/* -*- mode: C -*-  */
/* 
   IGraph library.
   Copyright (C) 2013  Gabor Csardi <csardi.gabor@gmail.com>
   334 Harvard street, Cambridge, MA 02139 USA
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 
   02110-1301 USA

*/

#ifdef IGRAPH_MC_ORIG
#define RESTYPE igraph_vector_ptr_t
#define SUFFIX
#endif

#ifdef IGRAPH_MC_COUNT
#define RESTYPE igraph_integer_t
#define SUFFIX _count
#endif

int FUNCTION(igraph_i_maximal_cliques_bk,SUFFIX)(
				igraph_vector_int_t *PX, int PS, int PE, 
				int XS, int XE, int oldPS, int oldXE,
				igraph_vector_int_t *R,
				igraph_vector_int_t *pos,
				igraph_adjlist_t *adjlist,
				RESTYPE *res,
				igraph_vector_int_t *nextv,
				igraph_vector_int_t *H,
				int min_size, int max_size) {

#ifdef DEBUG
  printf("<<<<\n");
  PRINT_PX;
#endif

  igraph_vector_int_push_back(H, -1); /* boundary */
  
  if (PS > PE && XS > XE) {
    /* Found a maximum clique, report it */
    int clsize=igraph_vector_int_size(R);
    if (min_size <= clsize && (clsize <= max_size || max_size <= 0)) {
#ifdef IGRAPH_MC_ORIG
      igraph_vector_t *cl=igraph_Calloc(1, igraph_vector_t);
      int j;
#ifdef DEBUG
      printf("clique: "); igraph_vector_int_print(R);
#endif
      if (!cl) {
	IGRAPH_ERROR("Cannot list maximal cliques", IGRAPH_ENOMEM);
      }
      igraph_vector_ptr_push_back(res, cl);
      igraph_vector_init(cl, clsize);
      for (j=0; j<clsize; j++) { VECTOR(*cl)[j] = VECTOR(*R)[j]; }
#endif
#ifdef IGRAPH_MC_COUNT
      (*res)++;
#endif
    }
  } else if (PS <= PE) {
    /* Select a pivot element */
    int pivot, mynextv;
    igraph_i_maximal_cliques_select_pivot(PX, PS, PE, XS, XE, pos,
					  adjlist, &pivot, nextv,
					  oldPS, oldXE);
#ifdef DEBUG
    printf("pivot: %i\n", pivot);
#endif
    while ((mynextv=igraph_vector_int_pop_back(nextv)) != -1) {
      int newPS, newXE;

      /* Going down, prepare */
      igraph_i_maximal_cliques_down(PX, PS, PE, XS, XE, pos, adjlist,
				    mynextv, R, &newPS, &newXE);
      /* Recursive call */
      FUNCTION(igraph_i_maximal_cliques_bk,SUFFIX)(
				  PX, newPS, PE, XS, newXE, PS, XE, R,
				  pos, adjlist, res, nextv, H,
				  min_size, max_size);

      /* igraph_i_maximal_cliques_check_order(PX, PS, PE, XS, XE, pos, */
      /* 					   adjlist); */

#ifdef DEBUG
      printf("Restored: ");
      PRINT_PX;
#endif
      /* Putting v from P to X */
      if (igraph_vector_int_tail(nextv) != -1) {
	igraph_i_maximal_cliques_PX(PX, PS, &PE, &XS, XE, pos, adjlist,
				    mynextv, H);
      }
    }
  }
  
  /* Putting back vertices from X to P, see notes in H */
  igraph_i_maximal_cliques_up(PX, PS, PE, XS, XE, pos, adjlist, R, H);

#ifdef DEBUG
  printf(">>>>\n");
#endif

  return 0;
}

#ifdef IGRAPH_MC_ORIG
void igraph_i_maximal_cliques_free(void *ptr) {
  igraph_vector_ptr_t *res=(igraph_vector_ptr_t*) ptr;
  int i, n=igraph_vector_ptr_size(res);
  for (i=0; i<n; i++) {
    igraph_vector_t *v=VECTOR(*res)[i];
    if (v) {
      igraph_Free(v);
      igraph_vector_destroy(v);
    }
  }
  igraph_vector_ptr_clear(res);
}
#endif

int FUNCTION(igraph_maximal_cliques,SUFFIX)(
			   const igraph_t *graph,
			   RESTYPE *res,
			   igraph_integer_t min_size, 
			   igraph_integer_t max_size) {

  /* Implementation details. TODO */

  igraph_vector_int_t PX, R, H, pos, nextv;
  igraph_vector_t coreness, order;
  igraph_vector_int_t rank;	/* TODO: this is not needed */
  int i, no_of_nodes=igraph_vcount(graph);
  igraph_adjlist_t adjlist, fulladjlist;

  if (igraph_is_directed(graph)) {
    IGRAPH_WARNING("Edge directions are ignored for maximal clique "
		   "calculation");
  }

  igraph_vector_init(&order, no_of_nodes);
  IGRAPH_FINALLY(igraph_vector_destroy, &order);
  igraph_vector_int_init(&rank, no_of_nodes);
  IGRAPH_FINALLY(igraph_vector_int_destroy, &rank);
  igraph_vector_init(&coreness, no_of_nodes);
  igraph_coreness(graph, &coreness, /*mode=*/ IGRAPH_ALL);
  IGRAPH_FINALLY(igraph_vector_destroy, &coreness);
  igraph_vector_qsort_ind(&coreness, &order, /*descending=*/ 0);
  for (i=0; i<no_of_nodes; i++) {
    int v=VECTOR(order)[i];
    VECTOR(rank)[v] = i;
  }

#ifdef DEBUG
  printf("coreness: "); igraph_vector_print(&coreness);
  printf("order:    "); igraph_vector_print(&order);
  printf("rank:     "); igraph_vector_int_print(&rank);
#endif

  igraph_vector_destroy(&coreness);
  IGRAPH_FINALLY_CLEAN(1);
  
  igraph_adjlist_init(graph, &adjlist, IGRAPH_ALL);

  igraph_adjlist_simplify(&adjlist);
  igraph_adjlist_init(graph, &fulladjlist, IGRAPH_ALL);
  IGRAPH_FINALLY(igraph_adjlist_destroy, &fulladjlist);
  igraph_adjlist_simplify(&fulladjlist);
  igraph_vector_int_init(&PX, 20);
  IGRAPH_FINALLY(igraph_vector_int_destroy, &PX);
  igraph_vector_int_init(&R,  20);
  IGRAPH_FINALLY(igraph_vector_int_destroy, &R);
  igraph_vector_int_init(&H, 100);
  IGRAPH_FINALLY(igraph_vector_int_destroy, &H);
  igraph_vector_int_init(&pos, no_of_nodes);
  IGRAPH_FINALLY(igraph_vector_int_destroy, &pos);
  igraph_vector_int_init(&nextv, 100);
  IGRAPH_FINALLY(igraph_vector_int_destroy, &nextv);

#ifdef IGRAPH_MC_ORIG  
  igraph_vector_ptr_clear(res);
  IGRAPH_FINALLY(igraph_i_maximal_cliques_free, res);
#endif
#ifdef IGRAPH_MC_COUNT
  *res=0;
#endif

  for (i=0; i<no_of_nodes; i++) {
    int v=VECTOR(order)[i];
    int vrank=VECTOR(rank)[v];
    igraph_vector_int_t *vneis=igraph_adjlist_get(&fulladjlist, v);
    int vdeg=igraph_vector_int_size(vneis);
    int Pptr=0, Xptr=vdeg-1, PS=0, PE, XS, XE=vdeg-1;
    int j;

#ifdef DEBUG
    printf("----------- vertex %i\n", v);
#endif
    
    IGRAPH_ALLOW_INTERRUPTION();
    
    igraph_vector_int_resize(&PX, vdeg);
    igraph_vector_int_resize(&R , 1);
    igraph_vector_int_resize(&H , 1);
    igraph_vector_int_null(&pos); /* TODO: makes it quadratic? */
    igraph_vector_int_resize(&nextv, 1);

    VECTOR(H)[0] = -1;		/* marks the end of the recursion */
    VECTOR(nextv)[0] = -1;

    /* ================================================================*/
    /* P <- G(v[i]) intersect { v[i+1], ..., v[n-1] }
       X <- G(v[i]) intersect { v[0], ..., v[i-1] } */
    
    VECTOR(R)[0] = v;
    for (j=0; j<vdeg; j++) {
      int vx=VECTOR(*vneis)[j];
      if (VECTOR(rank)[vx] > vrank) {
	VECTOR(PX)[Pptr] = vx;
	VECTOR(pos)[vx] = Pptr+1;
	Pptr++;
      } else if (VECTOR(rank)[vx] < vrank) {
	VECTOR(PX)[Xptr] = vx;
	VECTOR(pos)[vx] = Xptr+1;
	Xptr--;
      }
    }

    PE = Pptr-1; XS = Xptr+1;	/* end of P, start of X in PX */

    /* Create an adjacency list that is specific to the 
       v vertex. It only contains 'v' and its neighbors. Moreover, we 
       only deal with the vertices in P and X (and R). */
    igraph_vector_int_update(igraph_adjlist_get(&adjlist, v), 
			     igraph_adjlist_get(&fulladjlist, v));
    for (j=0; j<=vdeg-1; j++) {
      int vv=VECTOR(PX)[j];
      igraph_vector_int_t *fadj=igraph_adjlist_get(&fulladjlist, vv);
      igraph_vector_int_t *radj=igraph_adjlist_get(&adjlist, vv);
      int k, fn=igraph_vector_int_size(fadj);
      igraph_vector_int_clear(radj);
      for (k=0; k<fn; k++) {
	int nei=VECTOR(*fadj)[k];
	int neipos=VECTOR(pos)[nei]-1;
	if (neipos >= PS && neipos <= XE) {
	  igraph_vector_int_push_back(radj, nei);
	}
      }
    }

    /* Reorder the adjacency lists, according to P and X. */
    igraph_i_maximal_cliques_reorder_adjlists(&PX, PS, PE, XS, XE, &pos,
					      &adjlist);

    /* igraph_i_maximal_cliques_check_order(&PX, PS, PE, XS, XE, &pos, */
    /* 					 &adjlist); */

    FUNCTION(igraph_i_maximal_cliques_bk,SUFFIX)(
				&PX, PS, PE, XS, XE, PS, XE, &R, &pos,
				&adjlist, res, &nextv, &H, min_size,
				max_size);

  }

  igraph_vector_int_destroy(&nextv);
  igraph_vector_int_destroy(&pos);
  igraph_vector_int_destroy(&H);
  igraph_vector_int_destroy(&R);
  igraph_vector_int_destroy(&PX);
  igraph_adjlist_destroy(&fulladjlist);
  igraph_adjlist_destroy(&adjlist);
  igraph_vector_int_destroy(&rank);
  igraph_vector_destroy(&order);
  IGRAPH_FINALLY_CLEAN(10);	/* + res */

  return 0;
}

#undef RESTYPE
#undef SUFFIX

