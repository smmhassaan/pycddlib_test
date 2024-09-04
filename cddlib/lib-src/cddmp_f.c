/* generated automatically from cddmp.c */
/* cddmp.c       (cddlib arithmetic operations using gmp)
   written by Komei Fukuda, fukuda@math.ethz.ch
*/
/* This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "setoper.h"
#include "cdd_f.h"

void ddf_set_global_constants()
{
 ddf_init(ddf_zero);
 ddf_init(ddf_minuszero);
 ddf_init(ddf_one);
 ddf_init(ddf_minusone);
 ddf_init(ddf_purezero);
  
 time(&ddf_statStartTime); /* cddlib starting time */
 ddf_statBApivots=0;  /* basis finding pivots */
 ddf_statCCpivots=0;  /* criss-cross pivots */
 ddf_statDS1pivots=0; /* phase 1 pivots */
 ddf_statDS2pivots=0; /* phase 2 pivots */
 ddf_statACpivots=0;  /* anticycling (cc) pivots */

 ddf_choiceLPSolverDefault=ddf_DualSimplex;  /* Default LP solver Algorithm */
 ddf_choiceRedcheckAlgorithm=ddf_DualSimplex;  /* Redundancy Checking Algorithm */
 ddf_choiceLexicoPivotQ=ddf_TRUE;    /* whether to use the lexicographic pivot */
 
#if defined ddf_GMPRATIONAL
 ddf_statBSpivots=0;  /* basis status checking pivots */
 mpq_set_ui(ddf_zero,0U,1U);
 mpq_set_ui(ddf_purezero,0U,1U);
 mpq_set_ui(ddf_one,1U,1U);
 mpq_set_si(ddf_minusone,-1L,1U);
 ddf_set_global_constants();
#elif defined GMPFLOAT
 mpf_set_d(ddf_zero,ddf_almostzero);
 mpf_set_ui(ddf_purezero,0U);
 mpf_set_ui(ddf_one,1U);
 mpf_set_si(ddf_minusone,-1L,1U);
#else
 ddf_zero[0]= ddf_almostzero;  /*real zero */
 ddf_purezero[0]= 0.0;
 ddf_one[0]= 1L;
 ddf_minusone[0]= -1L;
#endif
 ddf_neg(ddf_minuszero,ddf_zero);
}

void ddf_free_global_constants()
{
 ddf_clear(ddf_zero);
 ddf_clear(ddf_minuszero);
 ddf_clear(ddf_one);
 ddf_clear(ddf_minusone);
 ddf_clear(ddf_purezero);
  
 time(&ddf_statStartTime); /* cddlib starting time */
 ddf_statBApivots=0;  /* basis finding pivots */
 ddf_statCCpivots=0;  /* criss-cross pivots */
 ddf_statDS1pivots=0; /* phase 1 pivots */
 ddf_statDS2pivots=0; /* phase 2 pivots */
 ddf_statACpivots=0;  /* anticycling (cc) pivots */

 ddf_choiceLPSolverDefault=ddf_DualSimplex;  /* Default LP solver Algorithm */
 ddf_choiceRedcheckAlgorithm=ddf_DualSimplex;  /* Redundancy Checking Algorithm */
 ddf_choiceLexicoPivotQ=ddf_TRUE;    /* whether to use the lexicographic pivot */
 
#if defined ddf_GMPRATIONAL
 ddf_statBSpivots=0;  /* basis status checking pivots */
 ddf_free_global_constants();
#endif
}


#if defined ddf_GMPRATIONAL
void dddf_mpq_set_si(myfloat a,signed long b)
{
  mpz_t nz, dz;

  mpz_init(nz); mpz_init(dz);

  mpz_set_si(nz, b);
  mpz_set_ui(dz, 1U);
  mpq_set_num(a, nz);
  mpq_set_den(a, dz);
  mpz_clear(nz);  mpz_clear(dz);
}
#endif

#if defined ddf_ddf_CDOUBLE
void dddf_init(myfloat a)   
{
  a[0]=0L;
}
  
void dddf_clear(myfloat a)
{
  /* a[0]=0L;  */
}

void dddf_set(myfloat a,myfloat b)
{
  a[0]=b[0];
}

void dddf_set_d(myfloat a,double b)
{
  a[0]=b;
}

void dddf_set_si(myfloat a,signed long b)
{
  a[0]=(double)b;
}

void dddf_set_si2(myfloat a,signed long b, unsigned long c)
{
  a[0]=(double)b/(double)c;
}

void dddf_add(myfloat a,myfloat b,myfloat c)
{
  a[0]=b[0]+c[0];
}

void dddf_sub(myfloat a,myfloat b,myfloat c)
{
  a[0]=b[0]-c[0];
}

void dddf_mul(myfloat a,myfloat b,myfloat c)
{
  a[0]=b[0]*c[0];
}

void dddf_div(myfloat a,myfloat b,myfloat c)
{
  a[0]=b[0]/c[0];
}

void dddf_neg(myfloat a,myfloat b)
{
  a[0]=-b[0];
}

void dddf_inv(myfloat a,myfloat b)
{
  a[0]=1/b[0];
}

int dddf_cmp(myfloat a,myfloat b)
{
  if (a[0]-b[0]>0) return 1;
  else if (a[0]-b[0]>=0) return 0;
  else return -1;
}

int dddf_sgn(myfloat a)
{
  if (a[0]>0) return 1;
  else if (a[0]>=0) return 0;
  else return -1;
}

double dddf_get_d(myfloat a)
{
  return a[0];
}
#endif

/* end of  cddmp.h  */
