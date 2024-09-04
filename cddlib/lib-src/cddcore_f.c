/* generated automatically from cddcore.c */
/* cddcore.c:  Core Procedures for cddlib
   written by Komei Fukuda, fukuda@math.ethz.ch
*/

/* cddlib : C-library of the double description method for
   computing all vertices and extreme rays of the polyhedron 
   P= {x :  b - A x >= 0}.  
   Please read COPYING (GNU General Public Licence) and
   the manual cddlibman.tex for detail.
*/

#include "setoper.h"
#include "cdd_f.h"
#include "splitmix64.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

void ddf_CheckAdjacency(ddf_ConePtr cone,
    ddf_RayPtr *RP1, ddf_RayPtr *RP2, ddf_boolean *adjacent)
{
  ddf_RayPtr TempRay;
  ddf_boolean localdebug=ddf_FALSE;
  static _Thread_local ddf_rowset Face, Face1;
  static _Thread_local ddf_rowrange last_m=0;
  
  if (last_m!=cone->m) {
    if (last_m>0){
      set_free(Face); set_free(Face1);
    }
    set_initialize(&Face, cone->m); 
    set_initialize(&Face1, cone->m); 
    last_m=cone->m;
  }

  localdebug=ddf_debug;
  *adjacent = ddf_TRUE;
  set_int(Face1, (*RP1)->ZeroSet, (*RP2)->ZeroSet);
  set_int(Face, Face1, cone->AddedHalfspaces);
  if (set_card(Face)< cone->d - 2) {
    *adjacent = ddf_FALSE;
    if (localdebug) {
      fprintf(stderr,"non adjacent: set_card(face) %ld < %ld = cone->d.\n",
        set_card(Face),cone->d);
    }
    return;
  }
  else if (cone->parent->NondegAssumed) {
  	*adjacent = ddf_TRUE;
  	return;
  }
  TempRay = cone->FirstRay;
  while (TempRay != NULL && *adjacent) {
    if (TempRay != *RP1 && TempRay != *RP2) {
    	set_int(Face1, TempRay->ZeroSet, cone->AddedHalfspaces);
      	if (set_subset(Face, Face1)) *adjacent = ddf_FALSE;
    }
    TempRay = TempRay->Next;
  }
}

void ddf_Eliminate(ddf_ConePtr cone, ddf_RayPtr*Ptr)
{
  /*eliminate the record pointed by Ptr^.Next*/
  ddf_RayPtr TempPtr;
  ddf_colrange j;

  TempPtr = (*Ptr)->Next;
  (*Ptr)->Next = (*Ptr)->Next->Next;
  if (TempPtr == cone->FirstRay)   /*Update the first pointer*/
    cone->FirstRay = (*Ptr)->Next;
  if (TempPtr == cone->LastRay)   /*Update the last pointer*/
    cone->LastRay = *Ptr;

  /* Added, Marc Pfetsch 010219 */
  for (j=0;j < cone->d;j++)
ddf_clear(TempPtr->Ray[j]);
  ddf_clear(TempPtr->ARay);

  free(TempPtr->Ray);          /* free the ray vector memory */
  set_free(TempPtr->ZeroSet);  /* free the ZeroSet memory */
  free(TempPtr);   /* free the ddf_Ray structure memory */
  cone->RayCount--; 
}

void ddf_SetInequalitySets(ddf_ConePtr cone)
{
  ddf_rowrange i;
  
  set_emptyset(cone->GroundSet);
  set_emptyset(cone->EqualitySet);
  set_emptyset(cone->NonequalitySet);  
  for (i = 1; i <= (cone->parent->m); i++){
    set_addelem(cone->GroundSet, i);
    if (cone->parent->EqualityIndex[i]==1) set_addelem(cone->EqualitySet,i);
    if (cone->parent->EqualityIndex[i]==-1) set_addelem(cone->NonequalitySet,i);
  }
}


void ddf_AValue(myfloat *val, ddf_colrange d_size, ddf_Amatrix A, myfloat *p, ddf_rowrange i)
{
  /*return the ith component of the vector  A x p */
  ddf_colrange j;
  myfloat x;

  ddf_init(x); 
  ddf_set(*val,ddf_purezero);
 /* Changed by Marc Pfetsch 010219 */

  for (j = 0; j < d_size; j++){
    ddf_mul(x,A[i - 1][j], p[j]);
    ddf_add(*val, *val, x);
  }
  ddf_clear(x);
}

void ddf_StoreRay1(ddf_ConePtr cone, myfloat *p, ddf_boolean *feasible)
{  /* Original ray storing routine when RelaxedEnumeration is ddf_FALSE */
  ddf_rowrange i,k,fii=cone->m+1;
  ddf_colrange j;
  myfloat temp;
  ddf_RayPtr RR;
  ddf_boolean localdebug=ddf_debug;

  ddf_init(temp);
  RR=cone->LastRay;
  *feasible = ddf_TRUE;
  set_initialize(&(RR->ZeroSet),cone->m);
  for (j = 0; j < cone->d; j++){
    ddf_set(RR->Ray[j],p[j]);
  }
  for (i = 1; i <= cone->m; i++) {
    k=cone->OrderVector[i];
    ddf_AValue(&temp, cone->d, cone->A, p, k);
    if (localdebug) {
      fprintf(stderr,"ddf_StoreRay1: ddf_AValue at row %ld =",k);
      ddf_WriteNumber(stderr, temp);
      fprintf(stderr,"\n");
    }
    if (ddf_EqualToZero(temp)) {
      set_addelem(RR->ZeroSet, k);
      if (localdebug) {
        fprintf(stderr,"recognized zero!\n");
      }
    }
    if (ddf_Negative(temp)){
      if (localdebug) {
        fprintf(stderr,"recognized negative!\n");
      }
      *feasible = ddf_FALSE;
      if (fii>cone->m) fii=i;  /* the first violating inequality index */
      if (localdebug) {
        fprintf(stderr,"this ray is not feasible, neg comp = %ld\n", fii);
        ddf_WriteNumber(stderr, temp);  fprintf(stderr,"\n");
      }
    }
  }
  RR->FirstInfeasIndex=fii;
  RR->feasible = *feasible;
  ddf_clear(temp);
}

void ddf_StoreRay2(ddf_ConePtr cone, myfloat *p, 
    ddf_boolean *feasible, ddf_boolean *weaklyfeasible)
   /* Ray storing routine when RelaxedEnumeration is ddf_TRUE.
       weaklyfeasible is true iff it is feasible with
       the strict_inequality conditions deleted. */
{
  ddf_RayPtr RR;
  ddf_rowrange i,k,fii=cone->m+1;
  ddf_colrange j;
  myfloat temp;
  ddf_boolean localdebug=ddf_debug;

  ddf_init(temp);
  RR=cone->LastRay;
  localdebug=ddf_debug;
  *feasible = ddf_TRUE;
  *weaklyfeasible = ddf_TRUE;
  set_initialize(&(RR->ZeroSet),cone->m);
  for (j = 0; j < cone->d; j++){
    ddf_set(RR->Ray[j],p[j]);
  }
  for (i = 1; i <= cone->m; i++) {
    k=cone->OrderVector[i];
    ddf_AValue(&temp, cone->d, cone->A, p, k);
    if (ddf_EqualToZero(temp)){
      set_addelem(RR->ZeroSet, k);
      if (cone->parent->EqualityIndex[k]==-1) 
        *feasible=ddf_FALSE;  /* strict inequality required */
    }
/*    if (temp < -zero){ */
    if (ddf_Negative(temp)){
      *feasible = ddf_FALSE;
      if (fii>cone->m && cone->parent->EqualityIndex[k]>=0) {
        fii=i;  /* the first violating inequality index */
        *weaklyfeasible=ddf_FALSE;
      }
    }
  }
  RR->FirstInfeasIndex=fii;
  RR->feasible = *feasible;
  ddf_clear(temp);
}


void ddf_AddRay(ddf_ConePtr cone, myfloat *p)
{  
  ddf_boolean feasible, weaklyfeasible;
  ddf_colrange j;

  if (cone->FirstRay == NULL) {
    cone->FirstRay = (ddf_RayPtr) malloc(sizeof(ddf_RayType));
    cone->FirstRay->Ray = (myfloat *) calloc(cone->d, sizeof(myfloat));
    for (j=0; j<cone->d; j++) ddf_init(cone->FirstRay->Ray[j]);
    ddf_init(cone->FirstRay->ARay);
    if (ddf_debug)
      fprintf(stderr,"Create the first ray pointer\n");
    cone->LastRay = cone->FirstRay;
    cone->ArtificialRay->Next = cone->FirstRay;
  } else {
    cone->LastRay->Next = (ddf_RayPtr) malloc(sizeof(ddf_RayType));
    cone->LastRay->Next->Ray = (myfloat *) calloc(cone->d, sizeof(myfloat));
    for (j=0; j<cone->d; j++) ddf_init(cone->LastRay->Next->Ray[j]);
    ddf_init(cone->LastRay->Next->ARay);
    if (ddf_debug) fprintf(stderr,"Create a new ray pointer\n");
    cone->LastRay = cone->LastRay->Next;
  }
  cone->LastRay->Next = NULL;
  cone->RayCount++;
  cone->TotalRayCount++;
  if (ddf_debug) {
    if (cone->TotalRayCount % 100 == 0) {
      fprintf(stderr,"*Rays (Total, Currently Active, Feasible) =%8ld%8ld%8ld\n",
	 cone->TotalRayCount, cone->RayCount, cone->FeasibleRayCount);
    }
  }
  if (cone->parent->RelaxedEnumeration){
    ddf_StoreRay2(cone, p, &feasible, &weaklyfeasible);
    if (weaklyfeasible) (cone->WeaklyFeasibleRayCount)++;
  } else {
    ddf_StoreRay1(cone, p, &feasible);
    if (feasible) (cone->WeaklyFeasibleRayCount)++;
    /* weaklyfeasible is equiv. to feasible in this case. */
  }
  if (!feasible) return;
  else {
    (cone->FeasibleRayCount)++;
  }
}

void ddf_AddArtificialRay(ddf_ConePtr cone)
{  
  ddf_Arow zerovector;
  ddf_colrange j,d1;
  ddf_boolean feasible;

  if (cone->d<=0) d1=1; else d1=cone->d;
  ddf_InitializeArow(d1, &zerovector);
  if (cone->ArtificialRay != NULL) {
    fprintf(stderr,"Warning !!!  FirstRay in not nil.  Illegal Call\n");
    free(zerovector); /* 086 */
    return;
  }
  cone->ArtificialRay = (ddf_RayPtr) malloc(sizeof(ddf_RayType));
  cone->ArtificialRay->Ray = (myfloat *) calloc(d1, sizeof(myfloat));
  for (j=0; j<d1; j++) ddf_init(cone->ArtificialRay->Ray[j]);
  ddf_init(cone->ArtificialRay->ARay);

  if (ddf_debug) fprintf(stderr,"Create the artificial ray pointer\n");

  cone->LastRay=cone->ArtificialRay;
  ddf_StoreRay1(cone, zerovector, &feasible);  
    /* This stores a vector to the record pointed by cone->LastRay */
  cone->ArtificialRay->Next = NULL;
  for (j = 0; j < d1; j++){
    ddf_clear(zerovector[j]);
  }
  free(zerovector); /* 086 */
}

void ddf_ConditionalAddEdge(ddf_ConePtr cone, 
    ddf_RayPtr Ray1, ddf_RayPtr Ray2, ddf_RayPtr ValidFirstRay)
{
  long it,it_row,fii1,fii2,fmin,fmax;
  ddf_boolean adjacent,lastchance;
  ddf_RayPtr TempRay,Rmin,Rmax;
  ddf_AdjacencyType *NewEdge;
  ddf_boolean localdebug=ddf_FALSE;
  ddf_rowset ZSmin, ZSmax;
  static _Thread_local ddf_rowset Face, Face1;
  static _Thread_local ddf_rowrange last_m=0;
  
  if (last_m!=cone->m) {
    if (last_m>0){
      set_free(Face); set_free(Face1);
    }
    set_initialize(&Face, cone->m);
    set_initialize(&Face1, cone->m);
    last_m=cone->m;
  }
  
  fii1=Ray1->FirstInfeasIndex;
  fii2=Ray2->FirstInfeasIndex;
  if (fii1<fii2){
    fmin=fii1; fmax=fii2;
    Rmin=Ray1;
    Rmax=Ray2;
  }
  else{
    fmin=fii2; fmax=fii1;
    Rmin=Ray2;
    Rmax=Ray1;
  }
  ZSmin = Rmin->ZeroSet;
  ZSmax = Rmax->ZeroSet;
  if (localdebug) {
    fprintf(stderr,"ddf_ConditionalAddEdge: FMIN = %ld (row%ld)   FMAX=%ld\n",
      fmin, cone->OrderVector[fmin], fmax);
  }
  if (fmin==fmax){
    if (localdebug) fprintf(stderr,"ddf_ConditionalAddEdge: equal FII value-> No edge added\n");
  }
  else if (set_member(cone->OrderVector[fmin],ZSmax)){
    if (localdebug) fprintf(stderr,"ddf_ConditionalAddEdge: No strong separation -> No edge added\n");
  }
  else {  /* the pair will be separated at the iteration fmin */
    lastchance=ddf_TRUE;
    /* flag to check it will be the last chance to store the edge candidate */
    set_int(Face1, ZSmax, ZSmin);
    (cone->count_int)++;
    if (localdebug){
      fprintf(stderr,"Face: ");
      for (it=1; it<=cone->m; it++) {
        it_row=cone->OrderVector[it];
        if (set_member(it_row, Face1)) fprintf(stderr,"%ld ",it_row);
      }
      fprintf(stderr,"\n");
    }
    for (it=cone->Iteration+1; it < fmin && lastchance; it++){
      it_row=cone->OrderVector[it];
      if (cone->parent->EqualityIndex[it_row]>=0 && set_member(it_row, Face1)){
        lastchance=ddf_FALSE;
        (cone->count_int_bad)++;
        if (localdebug){
          fprintf(stderr,"There will be another chance iteration %ld (row %ld) to store the pair\n", it, it_row);
        }
      }
    }
    if (lastchance){
      adjacent = ddf_TRUE;
      (cone->count_int_good)++;
      /* adjacent checking */
      set_int(Face, Face1, cone->AddedHalfspaces);
      if (localdebug){
        fprintf(stderr,"Check adjacency\n");
        fprintf(stderr,"AddedHalfspaces: "); set_fwrite(stderr,cone->AddedHalfspaces);
        fprintf(stderr,"Face: ");
        for (it=1; it<=cone->m; it++) {
          it_row=cone->OrderVector[it];
          if (set_member(it_row, Face)) fprintf(stderr,"%ld ",it_row);
        }
        fprintf(stderr,"\n");
      }
      if (set_card(Face)< cone->d - 2) {
        adjacent = ddf_FALSE;
      }
      else if (cone->parent->NondegAssumed) {
    	adjacent = ddf_TRUE;
      }
      else{
        TempRay = ValidFirstRay;  /* the first ray for adjacency checking */
        while (TempRay != NULL && adjacent) {
          if (TempRay != Ray1 && TempRay != Ray2) {
            set_int(Face1, TempRay->ZeroSet, cone->AddedHalfspaces);
            if (set_subset(Face, Face1)) {
              if (localdebug) set_fwrite(stderr,Face1);
              adjacent = ddf_FALSE;
            }
          }
          TempRay = TempRay->Next;
        }
      }
      if (adjacent){
        if (localdebug) fprintf(stderr,"The pair is adjacent and the pair must be stored for iteration %ld (row%ld)\n",
          fmin, cone->OrderVector[fmin]);
        NewEdge=(ddf_AdjacencyPtr) malloc(sizeof *NewEdge);
        NewEdge->Ray1=Rmax;  /* save the one remains in iteration fmin in the first */
        NewEdge->Ray2=Rmin;  /* save the one deleted in iteration fmin in the second */
        NewEdge->Next=NULL;
        (cone->EdgeCount)++; 
        (cone->TotalEdgeCount)++;
        if (cone->Edges[fmin]==NULL){
          cone->Edges[fmin]=NewEdge;
          if (localdebug) fprintf(stderr,"Create a new edge list of %ld\n", fmin);
        }else{
          NewEdge->Next=cone->Edges[fmin];
          cone->Edges[fmin]=NewEdge;
        }
      }
    }
  }
}

void ddf_CreateInitialEdges(ddf_ConePtr cone)
{
  ddf_RayPtr Ptr1, Ptr2;
  ddf_rowrange fii1,fii2;
  long count=0;
  ddf_boolean adj,localdebug=ddf_FALSE;

  cone->Iteration=cone->d;  /* CHECK */
  if (cone->FirstRay ==NULL || cone->LastRay==NULL){
    /* fprintf(stderr,"Warning: ddf_ CreateInitialEdges called with NULL pointer(s)\n"); */
    goto _L99;
  }
  Ptr1=cone->FirstRay;
  while(Ptr1!=cone->LastRay && Ptr1!=NULL){
    fii1=Ptr1->FirstInfeasIndex;
    Ptr2=Ptr1->Next;
    while(Ptr2!=NULL){
      fii2=Ptr2->FirstInfeasIndex;
      count++;
      if (localdebug) fprintf(stderr,"ddf_ CreateInitialEdges: edge %ld \n",count);
      ddf_CheckAdjacency(cone, &Ptr1, &Ptr2, &adj);
      if (fii1!=fii2 && adj) 
        ddf_ConditionalAddEdge(cone, Ptr1, Ptr2, cone->FirstRay);
      Ptr2=Ptr2->Next;
    }
    Ptr1=Ptr1->Next;
  }
_L99:;  
}


void ddf_UpdateEdges(ddf_ConePtr cone, ddf_RayPtr RRbegin, ddf_RayPtr RRend)
/* This procedure must be called after the ray list is sorted
   by ddf_EvaluateARay2 so that FirstInfeasIndex's are monotonically
   increasing.
*/
{
  ddf_RayPtr Ptr1, Ptr2begin, Ptr2;
  ddf_rowrange fii1;
  ddf_boolean ptr2found,quit,localdebug=ddf_FALSE;
  long count=0,pos1, pos2;
  float workleft,prevworkleft=110.0,totalpairs;

  totalpairs=(cone->ZeroRayCount-1.0)*(cone->ZeroRayCount-2.0)+1.0;
  Ptr2begin = NULL; 
  if (RRbegin ==NULL || RRend==NULL){
    if (1) fprintf(stderr,"Warning: ddf_UpdateEdges called with NULL pointer(s)\n");
    goto _L99;
  }
  Ptr1=RRbegin;
  pos1=1;
  do{
    ptr2found=ddf_FALSE;
    quit=ddf_FALSE;
    fii1=Ptr1->FirstInfeasIndex;
    pos2=2;
    for (Ptr2=Ptr1->Next; !ptr2found && !quit; Ptr2=Ptr2->Next,pos2++){
      if  (Ptr2->FirstInfeasIndex > fii1){
        Ptr2begin=Ptr2;
        ptr2found=ddf_TRUE;
      }
      else if (Ptr2==RRend) quit=ddf_TRUE;
    }
    if (ptr2found){
      quit=ddf_FALSE;
      for (Ptr2=Ptr2begin; !quit ; Ptr2=Ptr2->Next){
        count++;
        if (localdebug) fprintf(stderr,"ddf_UpdateEdges: edge %ld \n",count);
        ddf_ConditionalAddEdge(cone, Ptr1,Ptr2,RRbegin);
        if (Ptr2==RRend || Ptr2->Next==NULL) quit=ddf_TRUE;
      }
    }
    Ptr1=Ptr1->Next;
    pos1++;
    workleft = 100.0 * (cone->ZeroRayCount-pos1) * (cone->ZeroRayCount - pos1-1.0) / totalpairs;
    if (cone->ZeroRayCount>=500 && ddf_debug && pos1%10 ==0 && prevworkleft-workleft>=10 ) {
      fprintf(stderr,"*Work of iteration %5ld(/%ld): %4ld/%4ld => %4.1f%% left\n",
	     cone->Iteration, cone->m, pos1, cone->ZeroRayCount, workleft);
      prevworkleft=workleft;
    }    
  }while(Ptr1!=RRend && Ptr1!=NULL);
_L99:;  
}

void ddf_FreeDDMemory0(ddf_ConePtr cone)
{
  ddf_RayPtr Ptr, PrevPtr;
  long count;
  ddf_colrange j;
  ddf_boolean localdebug=ddf_FALSE;
  
  /* THIS SHOULD BE REWRITTEN carefully */
  PrevPtr=cone->ArtificialRay;
  if (PrevPtr!=NULL){
    count=0;
    for (Ptr=cone->ArtificialRay->Next; Ptr!=NULL; Ptr=Ptr->Next){
      /* Added Marc Pfetsch 2/19/01 */
      for (j=0;j < cone->d;j++)
ddf_clear(PrevPtr->Ray[j]);
      ddf_clear(PrevPtr->ARay);

      free(PrevPtr->Ray);
      free(PrevPtr->ZeroSet);
      free(PrevPtr);
      count++;
      PrevPtr=Ptr;
    };
    cone->FirstRay=NULL;
    /* Added Marc Pfetsch 010219 */
    for (j=0;j < cone->d;j++)
ddf_clear(cone->LastRay->Ray[j]);
    ddf_clear(cone->LastRay->ARay);

    free(cone->LastRay->Ray);
    cone->LastRay->Ray = NULL;
    set_free(cone->LastRay->ZeroSet);
    cone->LastRay->ZeroSet = NULL;
    free(cone->LastRay);
    cone->LastRay = NULL;
    cone->ArtificialRay=NULL;
    if (localdebug) fprintf(stderr,"%ld ray storage spaces freed\n",count);
  }
/* must add (by Sato) */
  free(cone->Edges);
  
  set_free(cone->GroundSet); 
  set_free(cone->EqualitySet); 
  set_free(cone->NonequalitySet); 
  set_free(cone->AddedHalfspaces); 
  set_free(cone->WeaklyAddedHalfspaces); 
  set_free(cone->InitialHalfspaces);
  free(cone->InitialRayIndex);
  free(cone->OrderVector);
  free(cone->newcol);

/* Fixed by Shawn Rusaw.  Originally it was cone->d instead of cone->d_alloc */
  ddf_FreeBmatrix(cone->d_alloc,cone->B);
  ddf_FreeBmatrix(cone->d_alloc,cone->Bsave);

/* Fixed by Marc Pfetsch 010219*/
  ddf_FreeAmatrix(cone->m_alloc,cone->d_alloc,cone->A);
  cone->A = NULL;

  free(cone);
}

void ddf_FreeDDMemory(ddf_PolyhedraPtr poly)
{
  ddf_FreeDDMemory0(poly->child);
  poly->child=NULL;
}

void ddf_FreePolyhedra(ddf_PolyhedraPtr poly)
{
  ddf_bigrange i;

  if ((poly)->child != NULL) ddf_FreeDDMemory(poly);
  ddf_FreeAmatrix((poly)->m_alloc,poly->d_alloc, poly->A);
  ddf_FreeArow((poly)->d_alloc,(poly)->c);
  free((poly)->EqualityIndex);
  if (poly->AincGenerated){
    for (i=1; i<=poly->m1; i++){
      set_free(poly->Ainc[i-1]);
    }
    free(poly->Ainc);
    set_free(poly->Ared);
    set_free(poly->Adom);
    poly->Ainc=NULL;
  }

  free(poly);
}

void ddf_Normalize(ddf_colrange d_size, myfloat *V)
{
  long j;
  myfloat temp,min;
  ddf_boolean nonzerofound=ddf_FALSE;

  if (d_size>0){
    ddf_init(min);  ddf_init(temp);
    ddf_abs(min,V[0]); /* set the minmizer to 0 */
    if (ddf_Positive(min)) nonzerofound=ddf_TRUE;
    for (j = 1; j < d_size; j++) {
      ddf_abs(temp,V[j]);
      if (ddf_Positive(temp)){
        if (!nonzerofound || ddf_Smaller(temp,min)){
          nonzerofound=ddf_TRUE;
          ddf_set(min, temp);
        }
      }
    }
    if (ddf_Positive(min)){
      for (j = 0; j < d_size; j++) ddf_div(V[j], V[j], min);
    }
    ddf_clear(min); ddf_clear(temp);
  }
}


void ddf_ZeroIndexSet(ddf_rowrange m_size, ddf_colrange d_size, ddf_Amatrix A, myfloat *x, ddf_rowset ZS)
{
  ddf_rowrange i;
  myfloat temp;

  /* Changed by Marc Pfetsch 010219 */
  ddf_init(temp);
  set_emptyset(ZS);
  for (i = 1; i <= m_size; i++) {
    ddf_AValue(&temp, d_size, A, x, i);
    if (ddf_EqualToZero(temp)) set_addelem(ZS, i);
  }

  /* Changed by Marc Pfetsch 010219 */
  ddf_clear(temp);
}

void ddf_CopyBmatrix(ddf_colrange d_size, ddf_Bmatrix T, ddf_Bmatrix TCOPY)
{
  ddf_rowrange i;
  ddf_colrange j;

  for (i=0; i < d_size; i++) {
    for (j=0; j < d_size; j++) {
      ddf_set(TCOPY[i][j],T[i][j]);
    }
  }
}


void ddf_CopyArow(myfloat *acopy, myfloat *a, ddf_colrange d)
{
  ddf_colrange j;

  for (j = 0; j < d; j++) {
    ddf_set(acopy[j],a[j]);
  }
}

void ddf_CopyNormalizedArow(myfloat *acopy, myfloat *a, ddf_colrange d)
{
  ddf_CopyArow(acopy, a, d);
  ddf_Normalize(d,acopy);
}

void ddf_CopyAmatrix(myfloat **Acopy, myfloat **A, ddf_rowrange m, ddf_colrange d)
{
  ddf_rowrange i;

  for (i = 0; i< m; i++) {
    ddf_CopyArow(Acopy[i],A[i],d);
  }
}

void ddf_CopyNormalizedAmatrix(myfloat **Acopy, myfloat **A, ddf_rowrange m, ddf_colrange d)
{
  ddf_rowrange i;

  for (i = 0; i< m; i++) {
    ddf_CopyNormalizedArow(Acopy[i],A[i],d);
  }
}

void ddf_PermuteCopyAmatrix(myfloat **Acopy, myfloat **A, ddf_rowrange m, ddf_colrange d, ddf_rowindex roworder)
{
  ddf_rowrange i;

  for (i = 1; i<= m; i++) {
    ddf_CopyArow(Acopy[i-1],A[roworder[i]-1],d);
  }
}

void ddf_PermutePartialCopyAmatrix(myfloat **Acopy, myfloat **A, ddf_rowrange m, ddf_colrange d, ddf_rowindex roworder,ddf_rowrange p, ddf_rowrange q)
{
 /* copy the rows of A whose roworder is positive.  roworder[i] is the row index of the copied row. */
  ddf_rowrange i;

  for (i = 1; i<= m; i++) {
    if (roworder[i]>0) ddf_CopyArow(Acopy[roworder[i]-1],A[i-1],d);
  }
}

// The three following functions seems trivial but they
// are usefull when writing a wrapper, e.g. they are used by CDDLib.jl
void ddf_SetMatrixObjective(ddf_MatrixPtr M, ddf_LPObjectiveType objective)
{
  M->objective = objective;
}

void ddf_SetMatrixNumberType(ddf_MatrixPtr M, ddf_NumberType numbtype)
{
  M->numbtype = numbtype;
}

void ddf_SetMatrixRepresentationType(ddf_MatrixPtr M, ddf_RepresentationType representation)
{
  M->representation = representation;
}

void ddf_InitializeArow(ddf_colrange d,ddf_Arow *a)
{
  ddf_colrange j;

  *a=(myfloat*) calloc(d,sizeof(myfloat));
  for (j = 0; j < d; j++) {
      ddf_init((*a)[j]);
  }
}

void ddf_InitializeAmatrix(ddf_rowrange m,ddf_colrange d,ddf_Amatrix *A)
{
  ddf_rowrange i;

  (*A)=(myfloat**) calloc(m,sizeof(myfloat*));
  for (i = 0; i < m; i++) {
    ddf_InitializeArow(d,&((*A)[i]));
  }
}

void ddf_FreeAmatrix(ddf_rowrange m,ddf_colrange d,ddf_Amatrix A)
{
  ddf_rowrange i;
  ddf_colrange j;

  for (i = 0; i < m; i++) {
    for (j = 0; j < d; j++) {
      ddf_clear(A[i][j]);
    }
  }
  if (A!=NULL) {
    for (i = 0; i < m; i++) {
      free(A[i]);
    }
    free(A);
  }
}

void ddf_FreeArow(ddf_colrange d, ddf_Arow a)
{
  ddf_colrange j;

  for (j = 0; j < d; j++) {
    ddf_clear(a[j]);
  }
  free(a);
}


void ddf_InitializeBmatrix(ddf_colrange d,ddf_Bmatrix *B)
{
  ddf_colrange i,j;

  (*B)=(myfloat**) calloc(d,sizeof(myfloat*));
  for (j = 0; j < d; j++) {
    (*B)[j]=(myfloat*) calloc(d,sizeof(myfloat));
  }
  for (i = 0; i < d; i++) {
    for (j = 0; j < d; j++) {
      ddf_init((*B)[i][j]);
    }
  }
}

void ddf_FreeBmatrix(ddf_colrange d,ddf_Bmatrix B)
{
  ddf_colrange i,j;

  for (i = 0; i < d; i++) {
    for (j = 0; j < d; j++) {
      ddf_clear(B[i][j]);
    }
  }
  if (B!=NULL) {
    for (j = 0; j < d; j++) {
      free(B[j]);
    }
    free(B);
  }
}

ddf_SetFamilyPtr ddf_CreateSetFamily(ddf_bigrange fsize, ddf_bigrange ssize)
{
  ddf_SetFamilyPtr F;
  ddf_bigrange i,f0,f1,s0,s1;

  if (fsize<=0) {
    f0=0; f1=1;  
    /* if fsize<=0, the fsize is set to zero and the created size is one */
  } else {
    f0=fsize; f1=fsize;
  }
  if (ssize<=0) {
    s0=0; s1=1;  
    /* if ssize<=0, the ssize is set to zero and the created size is one */
  } else {
    s0=ssize; s1=ssize;
  }

  F=(ddf_SetFamilyPtr) malloc (sizeof(ddf_SetFamilyType));
  F->set=(set_type*) calloc(f1,sizeof(set_type));
  for (i=0; i<f1; i++) {
    set_initialize(&(F->set[i]), s1);
  }
  F->famsize=f0;
  F->setsize=s0;
  return F;
}


void ddf_FreeSetFamily(ddf_SetFamilyPtr F)
{
  ddf_bigrange i,f1;

  if (F!=NULL){
    if (F->famsize<=0) f1=1; else f1=F->famsize; 
      /* the smallest created size is one */
    for (i=0; i<f1; i++) {
      set_free(F->set[i]);
    }
    free(F->set);
    free(F);
  }
}

ddf_MatrixPtr ddf_CreateMatrix(ddf_rowrange m_size,ddf_colrange d_size)
{
  ddf_MatrixPtr M;
  ddf_rowrange m0,m1;
  ddf_colrange d0,d1;

  if (m_size<=0){ 
    m0=0; m1=1;  
    /* if m_size <=0, the number of rows is set to zero, the actual size is 1 */
  } else {
    m0=m_size; m1=m_size;
  }
  if (d_size<=0){ 
    d0=0; d1=1;  
    /* if d_size <=0, the number of cols is set to zero, the actual size is 1 */
  } else {
    d0=d_size; d1=d_size;
  }
  M=(ddf_MatrixPtr) malloc (sizeof(ddf_MatrixType));
  ddf_InitializeAmatrix(m1,d1,&(M->matrix));
  ddf_InitializeArow(d1,&(M->rowvec));
  M->rowsize=m0;
  set_initialize(&(M->linset), m1);
  M->colsize=d0;
  M->objective=ddf_LPnone;
  M->numbtype=ddf_Unknown;
  M->representation=ddf_Unspecified;
  return M;
}

void ddf_FreeMatrix(ddf_MatrixPtr M)
{
  ddf_rowrange m1;
  ddf_colrange d1;

  if (M!=NULL) {
    if (M->rowsize<=0) m1=1; else m1=M->rowsize;
    if (M->colsize<=0) d1=1; else d1=M->colsize;
    if (M!=NULL) {
      ddf_FreeAmatrix(m1,d1,M->matrix);
      ddf_FreeArow(d1,M->rowvec);
      set_free(M->linset);
      free(M);
    }
  }
}

void ddf_SetToIdentity(ddf_colrange d_size, ddf_Bmatrix T)
{
  ddf_colrange j1, j2;

  for (j1 = 1; j1 <= d_size; j1++) {
    for (j2 = 1; j2 <= d_size; j2++) {
      if (j1 == j2)
        ddf_set(T[j1 - 1][j2 - 1],ddf_one);
      else
        ddf_set(T[j1 - 1][j2 - 1],ddf_purezero);
    }
  }
}

void ddf_ColumnReduce(ddf_ConePtr cone)
{
  ddf_colrange j,j1=0;
  ddf_rowrange i;
  ddf_boolean localdebug=ddf_FALSE;

  for (j=1;j<=cone->d;j++) {
    if (cone->InitialRayIndex[j]>0){
      j1=j1+1;
      if (j1<j) {
        for (i=1; i<=cone->m; i++) ddf_set(cone->A[i-1][j1-1],cone->A[i-1][j-1]);
        cone->newcol[j]=j1;
        if (localdebug){
          fprintf(stderr,"shifting the column %ld to column %ld\n", j, j1);
        }
          /* shifting forward */
      }
    } else {
      cone->newcol[j]=0;
      if (localdebug) {
        fprintf(stderr,"a generator (or an equation) of the linearity space: ");
        for (i=1; i<=cone->d; i++) ddf_WriteNumber(stderr, cone->B[i-1][j-1]);
        fprintf(stderr,"\n");
      }
    }
  }
  cone->d=j1;  /* update the dimension. cone->d_orig remembers the old. */
  ddf_CopyBmatrix(cone->d_orig, cone->B, cone->Bsave);  
    /* save the dual basis inverse as Bsave.  This matrix contains the linearity space generators. */
  cone->ColReduced=ddf_TRUE;
}

long ddf_MatrixRank(ddf_MatrixPtr M, ddf_rowset ignoredrows, ddf_colset ignoredcols, ddf_rowset *rowbasis, ddf_colset *colbasis)
{
  ddf_boolean stop, chosen, localdebug=ddf_debug;
  ddf_rowset NopivotRow,PriorityRow;
  ddf_colset ColSelected;
  ddf_Bmatrix B;
  ddf_rowindex roworder;
  ddf_rowrange r;
  ddf_colrange s;
  long rank;

  rank = 0;
  stop = ddf_FALSE;
  set_initialize(&ColSelected, M->colsize);
  set_initialize(&NopivotRow, M->rowsize);
  set_initialize(rowbasis, M->rowsize);
  set_initialize(colbasis, M->colsize);
  set_initialize(&PriorityRow, M->rowsize);
  set_copy(NopivotRow,ignoredrows);
  set_copy(ColSelected,ignoredcols);
  ddf_InitializeBmatrix(M->colsize, &B);
  ddf_SetToIdentity(M->colsize, B);
  roworder=(long *)calloc(M->rowsize+1,sizeof(long));
  for (r=0; r<M->rowsize; r++){roworder[r+1]=r+1;
  }

  do {   /* Find a set of rows for a basis */
      ddf_SelectPivot2(M->rowsize, M->colsize,M->matrix,B,ddf_MinIndex,roworder,
       PriorityRow,M->rowsize, NopivotRow, ColSelected, &r, &s, &chosen);
      if (ddf_debug && chosen) 
        fprintf(stderr,"Procedure ddf_MatrixRank: pivot on (r,s) =(%ld, %ld).\n", r, s);
      if (chosen) {
        set_addelem(NopivotRow, r);
        set_addelem(*rowbasis, r);
        set_addelem(ColSelected, s);
        set_addelem(*colbasis, s);
        rank++;
        ddf_GaussianColumnPivot(M->rowsize, M->colsize, M->matrix, B, r, s);
        if (localdebug) ddf_WriteBmatrix(stderr,M->colsize,B);
      } else {
        stop=ddf_TRUE;
      }
      if (rank==M->colsize) stop = ddf_TRUE;
  } while (!stop);
  ddf_FreeBmatrix(M->colsize,B);
  free(roworder);
  set_free(ColSelected);
  set_free(NopivotRow);
  set_free(PriorityRow);
  return rank;
}


void ddf_FindBasis(ddf_ConePtr cone, long *rank)
{
  ddf_boolean stop, chosen, localdebug=ddf_debug;
  ddf_rowset NopivotRow;
  ddf_colset ColSelected;
  ddf_rowrange r;
  ddf_colrange j,s;

  *rank = 0;
  stop = ddf_FALSE;
  for (j=0;j<=cone->d;j++) cone->InitialRayIndex[j]=0;
  set_emptyset(cone->InitialHalfspaces);
  set_initialize(&ColSelected, cone->d);
  set_initialize(&NopivotRow, cone->m);
  set_copy(NopivotRow,cone->NonequalitySet);
  ddf_SetToIdentity(cone->d, cone->B);
  do {   /* Find a set of rows for a basis */
      ddf_SelectPivot2(cone->m, cone->d,cone->A,cone->B,cone->HalfspaceOrder,cone->OrderVector,
       cone->EqualitySet,cone->m, NopivotRow, ColSelected, &r, &s, &chosen);
      if (ddf_debug && chosen) 
        fprintf(stderr,"Procedure ddf_FindBasis: pivot on (r,s) =(%ld, %ld).\n", r, s);
      if (chosen) {
        set_addelem(cone->InitialHalfspaces, r);
        set_addelem(NopivotRow, r);
        set_addelem(ColSelected, s);
        cone->InitialRayIndex[s]=r;    /* cone->InitialRayIndex[s] stores the corr. row index */
        (*rank)++;
        ddf_GaussianColumnPivot(cone->m, cone->d, cone->A, cone->B, r, s);
        if (localdebug) ddf_WriteBmatrix(stderr,cone->d,cone->B);
      } else {
        stop=ddf_TRUE;
      }
      if (*rank==cone->d) stop = ddf_TRUE;
  } while (!stop);
  set_free(ColSelected);
  set_free(NopivotRow);
}


void ddf_FindInitialRays(ddf_ConePtr cone, ddf_boolean *found)
{
  ddf_rowset CandidateRows;
  ddf_rowrange i;
  long rank;
  ddf_RowOrderType roworder_save=ddf_LexMin;

  *found = ddf_FALSE;
  set_initialize(&CandidateRows, cone->m);
  if (cone->parent->InitBasisAtBottom==ddf_TRUE) {
    roworder_save=cone->HalfspaceOrder;
    cone->HalfspaceOrder=ddf_MaxIndex;
    cone->PreOrderedRun=ddf_FALSE;
  }
  else cone->PreOrderedRun=ddf_TRUE;
  if (ddf_debug) ddf_WriteBmatrix(stderr, cone->d, cone->B);
  for (i = 1; i <= cone->m; i++)
    if (!set_member(i,cone->NonequalitySet)) set_addelem(CandidateRows, i);
    /*all rows not in NonequalitySet are candidates for initial cone*/
  ddf_FindBasis(cone, &rank);
  if (ddf_debug) ddf_WriteBmatrix(stderr, cone->d, cone->B);
  if (ddf_debug) fprintf(stderr,"ddf_FindInitialRays: rank of Amatrix = %ld\n", rank);
  cone->LinearityDim=cone->d - rank;
  if (ddf_debug) fprintf(stderr,"Linearity Dimension = %ld\n", cone->LinearityDim);
  if (cone->LinearityDim > 0) {
     ddf_ColumnReduce(cone);
     ddf_FindBasis(cone, &rank);
  }
  if (!set_subset(cone->EqualitySet,cone->InitialHalfspaces)) {
    if (ddf_debug) {
      fprintf(stderr,"Equality set is dependent. Equality Set and an initial basis:\n");
      set_fwrite(stderr,cone->EqualitySet);
      set_fwrite(stderr,cone->InitialHalfspaces);
    };
  }
  *found = ddf_TRUE;
  set_free(CandidateRows);
  if (cone->parent->InitBasisAtBottom==ddf_TRUE) {
    cone->HalfspaceOrder=roworder_save;
  }
  if (cone->HalfspaceOrder==ddf_MaxCutoff||
      cone->HalfspaceOrder==ddf_MinCutoff||
      cone->HalfspaceOrder==ddf_MixCutoff){
    cone->PreOrderedRun=ddf_FALSE;
  } else cone->PreOrderedRun=ddf_TRUE;
}

void ddf_CheckEquality(ddf_colrange d_size, ddf_RayPtr*RP1, ddf_RayPtr*RP2, ddf_boolean *equal)
{
  long j;

  if (ddf_debug)
    fprintf(stderr,"Check equality of two rays\n");
  *equal = ddf_TRUE;
  j = 1;
  while (j <= d_size && *equal) {
    if (!ddf_Equal((*RP1)->Ray[j - 1],(*RP2)->Ray[j - 1]))
      *equal = ddf_FALSE;
    j++;
  }
  if (*equal)
    fprintf(stderr,"Equal records found !!!!\n");
}

void ddf_CreateNewRay(ddf_ConePtr cone, 
    ddf_RayPtr Ptr1, ddf_RayPtr Ptr2, ddf_rowrange ii)
{
  /*Create a new ray by taking a linear combination of two rays*/
  ddf_colrange j;
  myfloat a1, a2, v1, v2;
  static _Thread_local ddf_Arow NewRay;
  static _Thread_local ddf_colrange last_d=0;
  ddf_boolean localdebug=ddf_debug;

  ddf_init(a1); ddf_init(a2); ddf_init(v1); ddf_init(v2);
  if (last_d!=cone->d){
    if (last_d>0) {
      for (j=0; j<last_d; j++) ddf_clear(NewRay[j]);
      free(NewRay);
    }
    NewRay=(myfloat*)calloc(cone->d,sizeof(myfloat));
    for (j=0; j<cone->d; j++) ddf_init(NewRay[j]);
    last_d=cone->d;
  }

  ddf_AValue(&a1, cone->d, cone->A, Ptr1->Ray, ii);
  ddf_AValue(&a2, cone->d, cone->A, Ptr2->Ray, ii);
  if (localdebug) {
    fprintf(stderr,"CreatNewRay: Ray1 ="); ddf_WriteArow(stderr, Ptr1->Ray, cone->d);
    fprintf(stderr,"CreatNewRay: Ray2 ="); ddf_WriteArow(stderr, Ptr2->Ray, cone->d);
  }
  ddf_abs(v1,a1);
  ddf_abs(v2,a2);
  if (localdebug){
    fprintf(stderr,"ddf_AValue1 and ABS");  ddf_WriteNumber(stderr,a1); ddf_WriteNumber(stderr,v1); fprintf(stderr,"\n");
    fprintf(stderr,"ddf_AValue2 and ABS");  ddf_WriteNumber(stderr,a2); ddf_WriteNumber(stderr,v2); fprintf(stderr,"\n");
  }
  for (j = 0; j < cone->d; j++){
    ddf_LinearComb(NewRay[j], Ptr1->Ray[j],v2,Ptr2->Ray[j],v1);
  }
  if (localdebug) {
    fprintf(stderr,"CreatNewRay: New ray ="); ddf_WriteArow(stderr, NewRay, cone->d);
  }
  ddf_Normalize(cone->d, NewRay);
  if (localdebug) {
    fprintf(stderr,"CreatNewRay: ddf_Normalized ray ="); ddf_WriteArow(stderr, NewRay, cone->d);
  }
  ddf_AddRay(cone, NewRay);
  ddf_clear(a1); ddf_clear(a2); ddf_clear(v1); ddf_clear(v2);
}

void ddf_EvaluateARay1(ddf_rowrange i, ddf_ConePtr cone)
/* Evaluate the ith component of the vector  A x RD.Ray 
    and rearrange the linked list so that
    the infeasible rays with respect to  i  will be
    placed consecutively from First 
 */
{
  ddf_colrange j;
  myfloat temp,tnext;
  ddf_RayPtr Ptr, PrevPtr, TempPtr;

  ddf_init(temp); ddf_init(tnext);
  Ptr = cone->FirstRay;
  PrevPtr = cone->ArtificialRay;
  if (PrevPtr->Next != Ptr) {
    fprintf(stderr,"Error.  Artificial Ray does not point to FirstRay!!!\n");
  }
  while (Ptr != NULL) {
    ddf_set(temp,ddf_purezero);
    for (j = 0; j < cone->d; j++){
      ddf_mul(tnext,cone->A[i - 1][j],Ptr->Ray[j]);
      ddf_add(temp,temp,tnext);
    }
    ddf_set(Ptr->ARay,temp);
/*    if ( temp <= -zero && Ptr != cone->FirstRay) {*/
    if ( ddf_Negative(temp) && Ptr != cone->FirstRay) {
      /* fprintf(stderr,"Moving an infeasible record w.r.t. %ld to FirstRay\n",i); */
      if (Ptr==cone->LastRay) cone->LastRay=PrevPtr;
      TempPtr=Ptr;
      Ptr = Ptr->Next;
      PrevPtr->Next = Ptr;
      cone->ArtificialRay->Next = TempPtr;
      TempPtr->Next = cone->FirstRay;
      cone->FirstRay = TempPtr;
    }
    else {
      PrevPtr = Ptr;
      Ptr = Ptr->Next;
    }
  }
  ddf_clear(temp); ddf_clear(tnext);
}

void ddf_EvaluateARay2(ddf_rowrange i, ddf_ConePtr cone)
/* Evaluate the ith component of the vector  A x RD.Ray 
   and rearrange the linked list so that
   the infeasible rays with respect to  i  will be
   placed consecutively from First. Also for all feasible rays,
   "positive" rays and "zero" rays will be placed consecutively.
 */
{
  ddf_colrange j;
  myfloat temp,tnext;
  ddf_RayPtr Ptr, NextPtr;
  ddf_boolean zerofound=ddf_FALSE,negfound=ddf_FALSE,posfound=ddf_FALSE;

  if (cone==NULL || cone->TotalRayCount<=0) goto _L99;  
  ddf_init(temp); ddf_init(tnext);
  cone->PosHead=NULL;cone->ZeroHead=NULL;cone->NegHead=NULL;
  cone->PosLast=NULL;cone->ZeroLast=NULL;cone->NegLast=NULL;
  Ptr = cone->FirstRay;
  while (Ptr != NULL) {
    NextPtr=Ptr->Next;  /* remember the Next record */
    Ptr->Next=NULL;     /* then clear the Next pointer */
    ddf_set(temp,ddf_purezero);
    for (j = 0; j < cone->d; j++){
      ddf_mul(tnext,cone->A[i - 1][j],Ptr->Ray[j]);
      ddf_add(temp,temp,tnext);
    }
    ddf_set(Ptr->ARay,temp);
/*    if ( temp < -zero) {*/
    if ( ddf_Negative(temp)) {
      if (!negfound){
        negfound=ddf_TRUE;
        cone->NegHead=Ptr;
        cone->NegLast=Ptr;
      }
      else{
        Ptr->Next=cone->NegHead;
        cone->NegHead=Ptr;
      }
    }
/*    else if (temp > zero){*/
    else if (ddf_Positive(temp)){
      if (!posfound){
        posfound=ddf_TRUE;
        cone->PosHead=Ptr;
        cone->PosLast=Ptr;
      }
      else{  
        Ptr->Next=cone->PosHead;
        cone->PosHead=Ptr;
       }
    }
    else {
      if (!zerofound){
        zerofound=ddf_TRUE;
        cone->ZeroHead=Ptr;
        cone->ZeroLast=Ptr;
      }
      else{
        Ptr->Next=cone->ZeroHead;
        cone->ZeroHead=Ptr;
      }
    }
    Ptr=NextPtr;
  }
  /* joining three neg, pos and zero lists */
  if (negfound){                 /* -list nonempty */
    cone->FirstRay=cone->NegHead;
    if (posfound){               /* -list & +list nonempty */
      cone->NegLast->Next=cone->PosHead;
      if (zerofound){            /* -list, +list, 0list all nonempty */
        cone->PosLast->Next=cone->ZeroHead;
        cone->LastRay=cone->ZeroLast;
      } 
      else{                      /* -list, +list nonempty but  0list empty */
        cone->LastRay=cone->PosLast;      
      }
    }
    else{                        /* -list nonempty & +list empty */
      if (zerofound){            /* -list,0list nonempty & +list empty */
        cone->NegLast->Next=cone->ZeroHead;
        cone->LastRay=cone->ZeroLast;
      } 
      else {                      /* -list nonempty & +list,0list empty */
        cone->LastRay=cone->NegLast;
      }
    }
  }
  else if (posfound){            /* -list empty & +list nonempty */
    cone->FirstRay=cone->PosHead;
    if (zerofound){              /* -list empty & +list,0list nonempty */
      cone->PosLast->Next=cone->ZeroHead;
      cone->LastRay=cone->ZeroLast;
    } 
    else{                        /* -list,0list empty & +list nonempty */
      cone->LastRay=cone->PosLast;
    }
  }
  else{                          /* -list,+list empty & 0list nonempty */
    cone->FirstRay=cone->ZeroHead;
    cone->LastRay=cone->ZeroLast;
  }
  cone->ArtificialRay->Next=cone->FirstRay;
  cone->LastRay->Next=NULL;
  ddf_clear(temp); ddf_clear(tnext);
  _L99:;  
}

void ddf_DeleteNegativeRays(ddf_ConePtr cone)
/* Eliminate the infeasible rays with respect to  i  which
   are supposed to be consecutive from the head of the ddf_Ray list,
   and sort the zero list assumed to be consecutive at the
   end of the list.
 */
{
  ddf_rowrange fii,fiitest;
  myfloat temp;
  ddf_RayPtr Ptr, PrevPtr,NextPtr,ZeroPtr1,ZeroPtr0;
  ddf_boolean found, completed, zerofound=ddf_FALSE,negfound=ddf_FALSE,posfound=ddf_FALSE;
  ddf_boolean localdebug=ddf_FALSE;
  
  ddf_init(temp);
  cone->PosHead=NULL;cone->ZeroHead=NULL;cone->NegHead=NULL;
  cone->PosLast=NULL;cone->ZeroLast=NULL;cone->NegLast=NULL;

  /* Delete the infeasible rays  */
  PrevPtr= cone->ArtificialRay;
  Ptr = cone->FirstRay;
  if (PrevPtr->Next != Ptr) 
    fprintf(stderr,"Error at ddf_DeleteNegativeRays: ArtificialRay does not point the FirstRay.\n");
  completed=ddf_FALSE;
  while (Ptr != NULL && !completed){
/*    if ( (Ptr->ARay) < -zero ){ */
    if ( ddf_Negative(Ptr->ARay)){
      ddf_Eliminate(cone, &PrevPtr);
      Ptr=PrevPtr->Next;
    }
    else{
      completed=ddf_TRUE;
    }
  }
  
  /* Sort the zero rays */
  Ptr = cone->FirstRay;
  cone->ZeroRayCount=0;
  while (Ptr != NULL) {
    NextPtr=Ptr->Next;  /* remember the Next record */
    ddf_set(temp,Ptr->ARay);
    if (localdebug) {fprintf(stderr,"Ptr->ARay :"); ddf_WriteNumber(stderr, temp);}
/*    if ( temp < -zero) {*/
    if ( ddf_Negative(temp)) {
      if (!negfound){
        fprintf(stderr,"Error: An infeasible ray found after their removal\n");
        negfound=ddf_TRUE;
      }
    }
/*    else if (temp > zero){*/
    else if (ddf_Positive(temp)){
      if (!posfound){
        posfound=ddf_TRUE;
        cone->PosHead=Ptr;
        cone->PosLast=Ptr;
      }
      else{  
        cone->PosLast=Ptr;
       }
    }
    else {
      (cone->ZeroRayCount)++;
      if (!zerofound){
        zerofound=ddf_TRUE;
        cone->ZeroHead=Ptr;
        cone->ZeroLast=Ptr;
        cone->ZeroLast->Next=NULL;
      }
      else{/* Find a right position to store the record sorted w.r.t. FirstInfeasIndex */
        fii=Ptr->FirstInfeasIndex; 
        found=ddf_FALSE;
        ZeroPtr1=NULL;
        for (ZeroPtr0=cone->ZeroHead; !found && ZeroPtr0!=NULL ; ZeroPtr0=ZeroPtr0->Next){
          fiitest=ZeroPtr0->FirstInfeasIndex;
          if (fiitest >= fii){
            found=ddf_TRUE;
          }
          else ZeroPtr1=ZeroPtr0;
        }
        /* fprintf(stderr,"insert position found \n %d  index %ld\n",found, fiitest); */
        if (!found){           /* the new record must be stored at the end of list */
          cone->ZeroLast->Next=Ptr;
          cone->ZeroLast=Ptr;
          cone->ZeroLast->Next=NULL;
        }
        else{
          if (ZeroPtr1==NULL){ /* store the new one at the head, and update the head ptr */
            /* fprintf(stderr,"Insert at the head\n"); */
            Ptr->Next=cone->ZeroHead;
            cone->ZeroHead=Ptr;
          }
          else{                /* store the new one inbetween ZeroPtr1 and 0 */
            /* fprintf(stderr,"Insert inbetween\n");  */
            Ptr->Next=ZeroPtr1->Next;
            ZeroPtr1->Next=Ptr;
          }
        }
        /*
        Ptr->Next=cone->ZeroHead;
        cone->ZeroHead=Ptr;
        */
      }
    }
    Ptr=NextPtr;
  }
  /* joining the pos and zero lists */
  if (posfound){            /* -list empty & +list nonempty */
    cone->FirstRay=cone->PosHead;
    if (zerofound){              /* +list,0list nonempty */
      cone->PosLast->Next=cone->ZeroHead;
      cone->LastRay=cone->ZeroLast;
    } 
    else{                        /* 0list empty & +list nonempty */
      cone->LastRay=cone->PosLast;
    }
  }
  else{                          /* +list empty & 0list nonempty */
    cone->FirstRay=cone->ZeroHead;
    cone->LastRay=cone->ZeroLast;
  }
  cone->ArtificialRay->Next=cone->FirstRay;
  cone->LastRay->Next=NULL;
  ddf_clear(temp);
}

void ddf_FeasibilityIndices(long *fnum, long *infnum, ddf_rowrange i, ddf_ConePtr cone)
{
  /*Evaluate the number of feasible rays and infeasible rays*/
  /*  w.r.t the hyperplane  i*/
  ddf_colrange j;
  myfloat temp, tnext;
  ddf_RayPtr Ptr;

  ddf_init(temp); ddf_init(tnext);
  *fnum = 0;
  *infnum = 0;
  Ptr = cone->FirstRay;
  while (Ptr != NULL) {
    ddf_set(temp,ddf_purezero);
    for (j = 0; j < cone->d; j++){
      ddf_mul(tnext, cone->A[i - 1][j],Ptr->Ray[j]);
      ddf_add(temp, temp, tnext);
    }
    if (ddf_Nonnegative(temp))
      (*fnum)++;
    else
      (*infnum)++;
    Ptr = Ptr->Next;
  }
  ddf_clear(temp); ddf_clear(tnext);
}

ddf_boolean ddf_LexSmaller(myfloat *v1, myfloat *v2, long dmax)
{ /* dmax is the size of vectors v1,v2 */
  ddf_boolean determined, smaller;
  ddf_colrange j;

  smaller = ddf_FALSE;
  determined = ddf_FALSE;
  j = 1;
  do {
    if (!ddf_Equal(v1[j - 1],v2[j - 1])) {  /* 086 */
      if (ddf_Smaller(v1[j - 1],v2[j - 1])) {  /*086 */
	    smaller = ddf_TRUE;
	  }
      determined = ddf_TRUE;
    } else
      j++;
  } while (!(determined) && (j <= dmax));
  return smaller;
}


ddf_boolean ddf_LexLarger(myfloat *v1, myfloat *v2, long dmax)
{
  return ddf_LexSmaller(v2, v1, dmax);
}

ddf_boolean ddf_LexEqual(myfloat *v1, myfloat *v2, long dmax)
{ /* dmax is the size of vectors v1,v2 */
  ddf_boolean determined, equal;
  ddf_colrange j;

  equal = ddf_TRUE;
  determined = ddf_FALSE;
  j = 1;
  do {
    if (!ddf_Equal(v1[j - 1],v2[j - 1])) {  /* 093c */
	equal = ddf_FALSE;
        determined = ddf_TRUE;
    } else {
      j++;
    }
  } while (!(determined) && (j <= dmax));
  return equal;
}

void ddf_AddNewHalfspace1(ddf_ConePtr cone, ddf_rowrange hnew)
/* This procedure 1 must be used with PreorderedRun=ddf_FALSE 
   This procedure is the most elementary implementation of
   DD and can be used with any type of ordering, including
   dynamic ordering of rows, e.g. MaxCutoff, MinCutoff.
   The memory requirement is minimum because it does not
   store any adjacency among the rays.
*/
{
  ddf_RayPtr RayPtr0,RayPtr1,RayPtr2,RayPtr2s,RayPtr3;
  long pos1, pos2;
  double prevprogress, progress;
  myfloat value1, value2;
  ddf_boolean adj, equal, completed;

  ddf_init(value1); ddf_init(value2);
  ddf_EvaluateARay1(hnew, cone);  
   /*Check feasibility of rays w.r.t. hnew 
     and put all infeasible ones consecutively */

  RayPtr0 = cone->ArtificialRay;   /*Pointer pointing RayPrt1*/
  RayPtr1 = cone->FirstRay;        /*1st hnew-infeasible ray to scan and compare with feasible rays*/
  ddf_set(value1,cone->FirstRay->ARay);
  if (ddf_Nonnegative(value1)) {
    if (cone->RayCount==cone->WeaklyFeasibleRayCount) cone->CompStatus=ddf_AllFound;
    goto _L99;        /* Sicne there is no hnew-infeasible ray and nothing to do */
  }
  else {
    RayPtr2s = RayPtr1->Next;/* RayPtr2s must point the first feasible ray */
    pos2=1;
    while (RayPtr2s!=NULL && ddf_Negative(RayPtr2s->ARay)) {
      RayPtr2s = RayPtr2s->Next;
      pos2++;
    }
  }
  if (RayPtr2s==NULL) {
    cone->FirstRay=NULL;
    cone->ArtificialRay->Next=cone->FirstRay;
    cone->RayCount=0;
    cone->CompStatus=ddf_AllFound;
    goto _L99;   /* All rays are infeasible, and the computation must stop */
  }
  RayPtr2 = RayPtr2s;   /*2nd feasible ray to scan and compare with 1st*/
  RayPtr3 = cone->LastRay;    /*Last feasible for scanning*/
  prevprogress=-10.0;
  pos1 = 1;
  completed=ddf_FALSE;
  while ((RayPtr1 != RayPtr2s) && !completed) {
    ddf_set(value1,RayPtr1->ARay);
    ddf_set(value2,RayPtr2->ARay);
    ddf_CheckEquality(cone->d, &RayPtr1, &RayPtr2, &equal);
    if ((ddf_Positive(value1) && ddf_Negative(value2)) || (ddf_Negative(value1) && ddf_Positive(value2))){
      ddf_CheckAdjacency(cone, &RayPtr1, &RayPtr2, &adj);
      if (adj) ddf_CreateNewRay(cone, RayPtr1, RayPtr2, hnew);
    }
    if (RayPtr2 != RayPtr3) {
      RayPtr2 = RayPtr2->Next;
      continue;
    }
    if (ddf_Negative(value1) || equal) {
      ddf_Eliminate(cone, &RayPtr0);
      RayPtr1 = RayPtr0->Next;
      RayPtr2 = RayPtr2s;
    } else {
      completed=ddf_TRUE;
    }
    pos1++;
    progress = 100.0 * ((double)pos1 / pos2) * (2.0 * pos2 - pos1) / pos2;
    if (progress-prevprogress>=10 && pos1%10==0 && ddf_debug) {
      fprintf(stderr,"*Progress of iteration %5ld(/%ld):   %4ld/%4ld => %4.1f%% done\n",
	     cone->Iteration, cone->m, pos1, pos2, progress);
      prevprogress=progress;
    }
  }
  if (cone->RayCount==cone->WeaklyFeasibleRayCount) cone->CompStatus=ddf_AllFound;
  _L99:;
  ddf_clear(value1); ddf_clear(value2);
}

void ddf_AddNewHalfspace2(ddf_ConePtr cone, ddf_rowrange hnew)
/* This procedure must be used under PreOrderedRun mode */
{
  ddf_RayPtr RayPtr0,RayPtr1,RayPtr2;
  ddf_AdjacencyType *EdgePtr, *EdgePtr0;
  long pos1;
  ddf_rowrange fii1, fii2;
  ddf_boolean localdebug=ddf_FALSE;

  ddf_EvaluateARay2(hnew, cone);
   /* Check feasibility of rays w.r.t. hnew 
      and sort them. ( -rays, +rays, 0rays)*/

  if (cone->PosHead==NULL && cone->ZeroHead==NULL) {
    cone->FirstRay=NULL;
    cone->ArtificialRay->Next=cone->FirstRay;
    cone->RayCount=0;
    cone->CompStatus=ddf_AllFound;
    goto _L99;   /* All rays are infeasible, and the computation must stop */
  }
  
  if (localdebug){
    pos1=0;
    fprintf(stderr,"(pos, FirstInfeasIndex, A Ray)=\n");
    for (RayPtr0=cone->FirstRay; RayPtr0!=NULL; RayPtr0=RayPtr0->Next){
      pos1++;
      fprintf(stderr,"(%ld,%ld,",pos1,RayPtr0->FirstInfeasIndex);
      ddf_WriteNumber(stderr,RayPtr0->ARay); 
      fprintf(stderr,") ");
   }
    fprintf(stderr,"\n");
  }
  
  if (cone->ZeroHead==NULL) cone->ZeroHead=cone->LastRay;

  EdgePtr=cone->Edges[cone->Iteration];
  while (EdgePtr!=NULL){
    RayPtr1=EdgePtr->Ray1;
    RayPtr2=EdgePtr->Ray2;
    fii1=RayPtr1->FirstInfeasIndex;   
    ddf_CreateNewRay(cone, RayPtr1, RayPtr2, hnew);
    fii2=cone->LastRay->FirstInfeasIndex;
    if (fii1 != fii2) 
      ddf_ConditionalAddEdge(cone,RayPtr1,cone->LastRay,cone->PosHead);
    EdgePtr0=EdgePtr;
    EdgePtr=EdgePtr->Next;
    free(EdgePtr0);
    (cone->EdgeCount)--;
  }
  cone->Edges[cone->Iteration]=NULL;
  
  ddf_DeleteNegativeRays(cone);
    
  set_addelem(cone->AddedHalfspaces, hnew);

  if (cone->Iteration<cone->m){
    if (cone->ZeroHead!=NULL && cone->ZeroHead!=cone->LastRay){
      if (cone->ZeroRayCount>200 && ddf_debug) fprintf(stderr,"*New edges being scanned...\n");
      ddf_UpdateEdges(cone, cone->ZeroHead, cone->LastRay);
    }
  }

  if (cone->RayCount==cone->WeaklyFeasibleRayCount) cone->CompStatus=ddf_AllFound;
_L99:;
}


void ddf_SelectNextHalfspace0(ddf_ConePtr cone, ddf_rowset excluded, ddf_rowrange *hnext)
{
  /*A natural way to choose the next hyperplane.  Simply the largest index*/
  long i;
  ddf_boolean determined;

  i = cone->m;
  determined = ddf_FALSE;
  do {
    if (set_member(i, excluded))
      i--;
    else
      determined = ddf_TRUE;
  } while (!determined && i>=1);
  if (determined) 
    *hnext = i;
  else
    *hnext = 0;
}

void ddf_SelectNextHalfspace1(ddf_ConePtr cone, ddf_rowset excluded, ddf_rowrange *hnext)
{
  /*Natural way to choose the next hyperplane.  Simply the least index*/
  long i;
  ddf_boolean determined;

  i = 1;
  determined = ddf_FALSE;
  do {
    if (set_member(i, excluded))
      i++;
    else
      determined = ddf_TRUE;
  } while (!determined && i<=cone->m);
  if (determined) 
    *hnext = i;
  else 
    *hnext=0;
}

void ddf_SelectNextHalfspace2(ddf_ConePtr cone, ddf_rowset excluded, ddf_rowrange *hnext)
{
  /*Choose the next hyperplane with maximum infeasibility*/
  long i, fea, inf, infmin, fi=0;   /*feasibility and infeasibility numbers*/

  infmin = cone->RayCount + 1;
  for (i = 1; i <= cone->m; i++) {
    if (!set_member(i, excluded)) {
      ddf_FeasibilityIndices(&fea, &inf, i, cone);
      if (inf < infmin) {
	infmin = inf;
	fi = fea;
	*hnext = i;
      }
    }
  }
  if (ddf_debug) {
    fprintf(stderr,"*infeasible rays (min) =%5ld, #feas rays =%5ld\n", infmin, fi);
  }
}

void ddf_SelectNextHalfspace3(ddf_ConePtr cone, ddf_rowset excluded, ddf_rowrange *hnext)
{
  /*Choose the next hyperplane with maximum infeasibility*/
  long i, fea, inf, infmax, fi=0;   /*feasibility and infeasibility numbers*/
  ddf_boolean localdebug=ddf_debug;

  infmax = -1;
  for (i = 1; i <= cone->m; i++) {
    if (!set_member(i, excluded)) {
      ddf_FeasibilityIndices(&fea, &inf, i, cone);
      if (inf > infmax) {
	infmax = inf;
	fi = fea;
	*hnext = i;
      }
    }
  }
  if (localdebug) {
    fprintf(stderr,"*infeasible rays (max) =%5ld, #feas rays =%5ld\n", infmax, fi);
  }
}

void ddf_SelectNextHalfspace4(ddf_ConePtr cone, ddf_rowset excluded, ddf_rowrange *hnext)
{
  /*Choose the next hyperplane with the most unbalanced cut*/
  long i, fea, inf, max, tmax, fi=0, infi=0;
      /*feasibility and infeasibility numbers*/

  max = -1;
  for (i = 1; i <= cone->m; i++) {
    if (!set_member(i, excluded)) {
      ddf_FeasibilityIndices(&fea, &inf, i, cone);
      if (fea <= inf)
        tmax = inf;
      else
        tmax = fea;
      if (tmax > max) {
        max = tmax;
        fi = fea;
        infi = inf;
        *hnext = i;
      }
    }
  }
  if (!ddf_debug)
    return;
  if (max == fi) {
    fprintf(stderr,"*infeasible rays (min) =%5ld, #feas rays =%5ld\n", infi, fi);
  } else {
    fprintf(stderr,"*infeasible rays (max) =%5ld, #feas rays =%5ld\n", infi, fi);
  }
}

void ddf_SelectNextHalfspace5(ddf_ConePtr cone, ddf_rowset excluded, ddf_rowrange *hnext)
{
  /*Choose the next hyperplane which is lexico-min*/
  long i, minindex;
  myfloat *v1, *v2;

  minindex = 0;
  v1 = NULL;
  for (i = 1; i <= cone->m; i++) {
    if (!set_member(i, excluded)) {
	  v2 = cone->A[i - 1];
      if (minindex == 0) {
	    minindex = i;
	    v1=v2;
      } else if (ddf_LexSmaller(v2,v1,cone->d)) {
        minindex = i;
	    v1=v2;
      }
    }
  }
  *hnext = minindex;
}


void ddf_SelectNextHalfspace6(ddf_ConePtr cone, ddf_rowset excluded, ddf_rowrange *hnext)
{
  /*Choose the next hyperplane which is lexico-max*/
  long i, maxindex;
  myfloat *v1, *v2;

  maxindex = 0;
  v1 = NULL;
  for (i = 1; i <= cone->m; i++) {
    if (!set_member(i, excluded)) {
      v2= cone->A[i - 1];
      if (maxindex == 0) {
        maxindex = i;
        v1=v2;
      } else if (ddf_LexLarger(v2, v1, cone->d)) {
        maxindex = i;
        v1=v2;
     }
    }
  }
  *hnext = maxindex;
}

void ddf_UniqueRows(ddf_rowindex OV, long p, long r, ddf_Amatrix A, long dmax, ddf_rowset preferred, long *uniqrows)
{
 /* Select a subset of rows of A (with range [p, q] up to dimension dmax) by removing duplicates.
    When a row subset preferred is nonempty, those row indices in the set have priority.  If
    two priority rows define the same row vector, one is chosen.
    For a selected unique row i, OV[i] returns a new position of the unique row i. 
    For other nonuniqu row i, OV[i] returns a negative of the original row j dominating i.
    Thus the original contents of OV[p..r] will be rewritten.  Other components remain the same.
    *uniqrows returns the number of unique rows.
*/
  long i,iuniq,j;
  myfloat *x;
  ddf_boolean localdebug=ddf_FALSE;
  
  if (p<=0 || p > r) goto _L99;
  iuniq=p; j=1;  /* the first unique row candidate */
  x=A[p-1];
  OV[p]=j;  /* tentative row index of the candidate */
  for (i=p+1;i<=r; i++){
    if (!ddf_LexEqual(x,A[i-1],dmax)) {
      /* a new row vector found. */
      iuniq=i;
      j=j+1;
      OV[i]=j;    /* Tentatively register the row i.  */
      x=A[i-1];
    } else {
      /* rows are equal */
      if (set_member(i, preferred) && !set_member(iuniq, preferred)){
        OV[iuniq]=-i;  /* the row iuniq is dominated by the row i */
        iuniq=i;  /* the row i is preferred.  Change the candidate. */
        OV[i]=j;  /* the row i is tentatively registered. */
        x=A[i-1];
      } else {
        OV[i]=-iuniq;  /* the row iuniq is dominated by the row i */
      }
    }
  }
  *uniqrows=j;
  if (localdebug){
    printf("The number of unique rows are %ld\n with order vector",*uniqrows);
    for (i=p;i<=r; i++){
      printf(" %ld:%ld ",i,OV[i]);
    }
    printf("\n");
  }
  _L99:;
}

long ddf_Partition(ddf_rowindex OV, long p, long r, ddf_Amatrix A, long dmax)
{
  myfloat *x;
  long i,j,ovi;
  
  x=A[OV[p]-1];

  i=p-1;
  j=r+1;
  while (ddf_TRUE){
    do{
      j--;
    } while (ddf_LexLarger(A[OV[j]-1],x,dmax));
    do{
      i++;
    } while (ddf_LexSmaller(A[OV[i]-1],x,dmax));
    if (i<j){
      ovi=OV[i];
      OV[i]=OV[j];
      OV[j]=ovi;
    }
    else{
      return j;
    }
  }
}

void ddf_QuickSort(ddf_rowindex OV, long p, long r, ddf_Amatrix A, long dmax)
{
  long q;
  
  if (p < r){
    q = ddf_Partition(OV, p, r, A, dmax);
    ddf_QuickSort(OV, p, q, A, dmax);
    ddf_QuickSort(OV, q+1, r, A, dmax);
  }
}


#ifndef RAND_MAX 
#define RAND_MAX 32767 
#endif

void ddf_RandomPermutation(ddf_rowindex OV, long t, unsigned int seed)
{
  long k,j,ovj;
  double u,xk,r,rand_max=(double) UINT64_MAX;
  ddf_boolean localdebug=ddf_FALSE;

  srand_splitmix64(seed);
  for (j=t; j>1 ; j--) {
    r=rand_splitmix64();
    u=r/rand_max;
    xk=(double)(j*u +1);
    k=(long)xk;
    if (localdebug) fprintf(stderr,"u=%g, k=%ld, r=%g, randmax= %g\n",u,k,r,rand_max);
    ovj=OV[j];
    OV[j]=OV[k];
    OV[k]=ovj;
    if (localdebug) fprintf(stderr,"row %ld is exchanged with %ld\n",j,k); 
  }
}

void ddf_ComputeRowOrderVector(ddf_ConePtr cone)
{
  long i,itemp;

  cone->OrderVector[0]=0;
  switch (cone->HalfspaceOrder){
  case ddf_MaxIndex:
    for(i=1; i<=cone->m; i++) cone->OrderVector[i]=cone->m-i+1;
    break;

  case ddf_MinIndex: 
    for(i=1; i<=cone->m; i++) cone->OrderVector[i]=i;    
    break;

  case ddf_LexMin: case ddf_MinCutoff: case ddf_MixCutoff: case ddf_MaxCutoff:
    for(i=1; i<=cone->m; i++) cone->OrderVector[i]=i;
    ddf_RandomPermutation(cone->OrderVector, cone->m, cone->rseed);
    ddf_QuickSort(cone->OrderVector, 1, cone->m, cone->A, cone->d);
    break;

  case ddf_LexMax:
    for(i=1; i<=cone->m; i++) cone->OrderVector[i]=i;
    ddf_RandomPermutation(cone->OrderVector, cone->m, cone->rseed);
    ddf_QuickSort(cone->OrderVector, 1, cone->m, cone->A, cone->d);
    for(i=1; i<=cone->m/2;i++){   /* just reverse the order */
      itemp=cone->OrderVector[i];
      cone->OrderVector[i]=cone->OrderVector[cone->m-i+1];
      cone->OrderVector[cone->m-i+1]=itemp;
    }
    break;

  case ddf_RandomRow:
    for(i=1; i<=cone->m; i++) cone->OrderVector[i]=i;
    ddf_RandomPermutation(cone->OrderVector, cone->m, cone->rseed);
    break;

  }
}

void ddf_UpdateRowOrderVector(ddf_ConePtr cone, ddf_rowset PriorityRows)
/* Update the RowOrder vector to shift selected rows
in highest order.
*/
{
  ddf_rowrange i,j,k,j1=0,oj=0;
  long rr;
  ddf_boolean found, localdebug=ddf_FALSE;
  
  localdebug=ddf_debug;
  found=ddf_TRUE;
  rr=set_card(PriorityRows);
  if (localdebug) set_fwrite(stderr,PriorityRows);
  for (i=1; i<=rr; i++){
    found=ddf_FALSE;
    for (j=i; j<=cone->m && !found; j++){
      oj=cone->OrderVector[j];
      if (set_member(oj, PriorityRows)){
        found=ddf_TRUE;
        if (localdebug) fprintf(stderr,"%ldth in sorted list (row %ld) is in PriorityRows\n", j, oj);
        j1=j;
      }
    }
    if (found){
      if (j1>i) {
        /* shift everything lower: ov[i]->cone->ov[i+1]..ov[j1-1]->cone->ov[j1] */
        for (k=j1; k>=i; k--) cone->OrderVector[k]=cone->OrderVector[k-1];
        cone->OrderVector[i]=oj;
        if (localdebug){
          fprintf(stderr,"OrderVector updated to:\n");
          for (j = 1; j <= cone->m; j++) fprintf(stderr," %2ld", cone->OrderVector[j]);
          fprintf(stderr,"\n");
        }
      }
    } else {
      fprintf(stderr,"UpdateRowOrder: Error.\n");
      goto _L99;
    }
  }
_L99:;
}

void ddf_SelectPreorderedNext(ddf_ConePtr cone, ddf_rowset excluded, ddf_rowrange *hh)
{
  ddf_rowrange i,k;
  
  *hh=0;
  for (i=1; i<=cone->m && *hh==0; i++){
    k=cone->OrderVector[i];
    if (!set_member(k, excluded)) *hh=k ;
  }
}

void ddf_SelectNextHalfspace(ddf_ConePtr cone, ddf_rowset excluded, ddf_rowrange *hh)
{
  if (cone->PreOrderedRun){
    if (ddf_debug) {
      fprintf(stderr,"debug ddf_SelectNextHalfspace: Use PreorderNext\n");
    }
    ddf_SelectPreorderedNext(cone, excluded, hh);
  }
  else {
    if (ddf_debug) {
      fprintf(stderr,"debug ddf_SelectNextHalfspace: Use DynamicOrderedNext\n");
    }

    switch (cone->HalfspaceOrder) {

    case ddf_MaxIndex:
      ddf_SelectNextHalfspace0(cone, excluded, hh);
      break;

    case ddf_MinIndex:
      ddf_SelectNextHalfspace1(cone, excluded, hh);
      break;

    case ddf_MinCutoff:
      ddf_SelectNextHalfspace2(cone, excluded, hh);
      break;

    case ddf_MaxCutoff:
      ddf_SelectNextHalfspace3(cone, excluded, hh);
      break;

    case ddf_MixCutoff:
      ddf_SelectNextHalfspace4(cone, excluded, hh);
      break;

    default:
      ddf_SelectNextHalfspace0(cone, excluded, hh);
      break;
    }
  }
}

ddf_boolean ddf_Nonnegative(myfloat val)
{
/*  if (val>=-ddf_zero) return ddf_TRUE;  */
  if (ddf_cmp(val,ddf_minuszero)>=0) return ddf_TRUE;
  else return ddf_FALSE;
}

ddf_boolean ddf_Nonpositive(myfloat val)
{
/*  if (val<=ddf_zero) return ddf_TRUE;  */
  if (ddf_cmp(val,ddf_zero)<=0) return ddf_TRUE;
  else return ddf_FALSE;
}

ddf_boolean ddf_Positive(myfloat val)
{
  return !ddf_Nonpositive(val);
}

ddf_boolean ddf_Negative(myfloat val)
{
  return !ddf_Nonnegative(val);
}

ddf_boolean ddf_EqualToZero(myfloat val)
{
  return (ddf_Nonnegative(val) && ddf_Nonpositive(val));
}

ddf_boolean ddf_Nonzero(myfloat val)
{
  return (ddf_Positive(val) || ddf_Negative(val));
}

ddf_boolean ddf_Equal(myfloat val1,myfloat val2)
{
  return (!ddf_Larger(val1,val2) && !ddf_Smaller(val1,val2));
}

ddf_boolean ddf_Larger(myfloat val1,myfloat val2)
{
  myfloat temp;
  ddf_boolean answer;

  ddf_init(temp);
  ddf_sub(temp,val1, val2);
  answer=ddf_Positive(temp);
  ddf_clear(temp);
  return answer;
}

ddf_boolean ddf_Smaller(myfloat val1,myfloat val2)
{
  return ddf_Larger(val2,val1);
}

void ddf_abs(myfloat absval, myfloat val)
{
  if (ddf_Negative(val)) ddf_neg(absval,val);
  else ddf_set(absval,val); 
}

void ddf_LinearComb(myfloat lc, myfloat v1, myfloat c1, myfloat v2, myfloat c2)
/*  lc := v1 * c1 + v2 * c2   */
{
  myfloat temp;

  ddf_init(temp);
  ddf_mul(lc,v1,c1);
  ddf_mul(temp,v2,c2); 
  ddf_add(lc,lc,temp);
  ddf_clear(temp);
}

void ddf_InnerProduct(myfloat prod, ddf_colrange d, ddf_Arow v1, ddf_Arow v2)
{
  myfloat temp;
  ddf_colrange j;
  ddf_boolean localdebug=ddf_FALSE;

  ddf_init(temp);
  ddf_set_si(prod, 0);
  for (j = 0; j < d; j++){
    ddf_mul(temp,v1[j],v2[j]); 
    ddf_add(prod,prod,temp);
  }
  if (localdebug) {
    fprintf(stderr,"InnerProduct:\n"); 
    ddf_WriteArow(stderr, v1, d);
    ddf_WriteArow(stderr, v2, d);
    fprintf(stderr,"prod ="); 
    ddf_WriteNumber(stderr, prod);
    fprintf(stderr,"\n");
  }
  
  ddf_clear(temp);
}

/* end of cddcore.c */


