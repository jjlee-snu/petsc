/* static char vcid[] = "$Id: icc.h,v 1.7 1999/01/31 16:08:24 bsmith Exp bsmith $ "; */
#include "src/sles/pc/pcimpl.h"          

#if !defined(__ICC_H)
#define __ICC_H

/* Incomplete Cholesky factorization context */

typedef struct {
  Mat             fact;
  MatOrderingType ordering;
  int             levels;
  void            *implctx;
} PC_ICC;

#endif
