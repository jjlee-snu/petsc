#ifndef lint
static char vcid[] = "$Id: precon.c,v 1.14 1995/03/27 21:02:31 bsmith Exp curfman $";
#endif

/*  
   Defines the abstract operations on index sets 
*/
#include "pcimpl.h"      /*I "pc.h" I*/

/*@
    PCPrintHelp - Prints help message for PC.

  Input Parameter:
.  pc - a preconditioner context
@*/
int PCPrintHelp(PC pc)
{
  char *p; 
  if (pc->prefix) p = pc->prefix; else p = "-";
  fprintf(stderr,"PC options ----------------------------------------\n");
  PCPrintMethods(p,"pcmethod");
  fprintf(stderr,"Run program with %spcmethod method -help for help on ",p);
  fprintf(stderr,"a particular method\n");
  if (pc->printhelp) (*pc->printhelp)(pc);
  return 0;
}

/*@
    PCDestroy - Destroys a preconditioner context.

  Input Parameters:
.  pc - the preconditioner context.

@*/
int PCDestroy(PC pc)
{
  int ierr = 0;
  VALIDHEADER(pc,PC_COOKIE);
  if (pc->destroy) ierr =  (*pc->destroy)((PetscObject)pc);
  else {
    if (pc->data) FREE(pc->data);
    PLogObjectDestroy(pc);
    PETSCHEADERDESTROY(pc);
  }
  return ierr;
}

/*@
    PCCreate - Creates a preconditioner context.

  Output Parameters:
.  pc - the preconditioner context.

  Note:
  The default preconditioner is PCJACOBI.
@*/
int PCCreate(PC *newpc)
{
  PC pc;
  *newpc          = 0;
  PETSCHEADERCREATE(pc,_PC,PC_COOKIE,PCJACOBI,MPI_COMM_WORLD);
  PLogObjectCreate(pc);
  pc->vec         = 0;
  pc->mat         = 0;
  pc->setupcalled = 0;
  pc->destroy     = 0;
  pc->data        = 0;
  pc->apply       = 0;
  pc->applytrans  = 0;
  pc->applyBA     = 0;
  pc->applyBAtrans= 0;
  pc->applyrich   = 0;
  pc->prefix      = 0;
  *newpc          = pc;
  /* this violates rule about seperating abstract from implementions*/
  return PCSetMethod(pc,PCJACOBI);
}

/*@
     PCApply - Applies preconditioner to a vector.
@*/
int PCApply(PC pc,Vec x,Vec y)
{
  VALIDHEADER(pc,PC_COOKIE);
  return (*pc->apply)(pc,x,y);
}
/*@
     PCApplyTrans - Applies transpose of preconditioner to a vector.
@*/
int PCApplyTrans(PC pc,Vec x,Vec y)
{
  VALIDHEADER(pc,PC_COOKIE);
  if (pc->applytrans) return (*pc->applytrans)(pc,x,y);
  SETERR(1,"No transpose for this precondition");
}

/*@
     PCApplyBAorAB - Applies preconditioner and operator to a vector. 
@*/
int PCApplyBAorAB(PC pc,int right,Vec x,Vec y,Vec work)
{
  int ierr;
  VALIDHEADER(pc,PC_COOKIE);
  if (pc->applyBA)  return (*pc->applyBA)(pc,right,x,y,work);
  if (right) {
    ierr = PCApply(pc,x,work); CHKERR(ierr);
    return MatMult(pc->mat,work,y); 
  }
  ierr = MatMult(pc->mat,x,work); CHKERR(ierr);
  return PCApply(pc,work,y);
}
/*@
     PCApplyBAorABTrans - Applies transpose of preconditioner and operator
                          to a vector.
@*/
int PCApplyBAorABTrans(PC pc,int right,Vec x,Vec y,Vec work)
{
  int ierr;
  VALIDHEADER(pc,PC_COOKIE);
  if (pc->applyBAtrans)  return (*pc->applyBAtrans)(pc,right,x,y,work);
  if (right) {
    ierr = MatMultTrans(pc->mat,x,work); CHKERR(ierr);
    return PCApplyTrans(pc,work,y);
  }
  ierr = PCApplyTrans(pc,x,work); CHKERR(ierr);
  return MatMultTrans(pc->mat,work,y); 
}

/*@
      PCApplyRichardson - Determines if a particular preconditioner has a 
                          built in fast application of Richardson's method.

  Input Parameters:
.   pc - the preconditioner
@*/
int PCApplyRichardsonExists(PC pc)
{
  if (pc->applyrich) return 1; else return 0;
}

/*@
     PCApplyRichardson - Applies several steps of Richardson iteration with 
                         the particular preconditioner. This routine is 
                         usually used by the Krylov solvers and not the 
                         application code directly.

  Input Parameters:
.   pc - the preconditioner context
.   x, y - the initial guess and the solution
.   w    - one work vector
.   its - the number of iterations to apply.

   Note: most preconditioners do not support this function. Use the command
         PCApplyRichardsonExists() to determine if one does.
@*/
int PCApplyRichardson(PC pc,Vec x,Vec y,Vec w,int its)
{
  VALIDHEADER(pc,PC_COOKIE);
  return (*pc->applyrich)(pc,x,y,w,its);
}

/* 
      a setupcall of 0 indicates never setup, 
                     1 needs to be resetup,
                     2 does not need any changes.
*/
/*@
    PCSetUp - Prepares for the use of a preconditioner.

  Input parameters:
.   pc - the preconditioner context
@*/
int PCSetUp(PC pc)
{
  int ierr;
  if (pc->setupcalled > 1) return 0;
  if (!pc->vec) {SETERR(1,"Vector must be set before calling PCSetUp");}
  if (!pc->mat) {SETERR(1,"Matrix must be set before calling PCSetUp");}
  if (pc->setup) { ierr = (*pc->setup)(pc); CHKERR(ierr);}
  pc->setupcalled = 2;
  return 0;
}

/*@
    PCSetOperators - Set the matrix associated with the linear system and 
          a (possibly) different one associated with the preconditioner.

  Input Parameters:
.  pc - the preconditioner context
.  mat - the matrix
.  pmat - matrix to be used in constructing preconditioner, usually the same
          as mat.  If pmat is 0, the old preconditioner is used.
.  flag - use either 0 or MAT_SAME_NONZERO_PATTERN

  Notes:
  The flag can be used to eliminate unnecessary repeated work in the 
  repeated solution of linear systems of the same size using the same 
  preconditioner.  The user can set flag = MAT_SAME_NONZERO_PATTERN to 
  indicate that the preconditioning matrix has the same nonzero pattern 
  during successive linear solves.
@*/
int PCSetOperators(PC pc,Mat mat,Mat pmat,int flag)
{
  VALIDHEADER(pc,PC_COOKIE);
  pc->mat         = mat;
  if (pc->setupcalled == 0 && !pmat) SETERR(1,"Must set preconditioner");
  if (pc->setupcalled && pmat) {
    pc->pmat        = pmat;
    pc->setupcalled = 1;  
  }
  else if (pc->setupcalled == 0) {
    pc->pmat = pmat;
  }
  pc->flag        = flag;
  return 0;
}
/*@
    PCGetMat - Gets the matrix associated with the preconditioner.

  Input Parameters:
.  pc - the preconditioner context

  Output Parameter:
.  mat - the matrix
@*/
Mat PCGetMat(PC pc)
{
  return pc->mat;
}

/*@
    PCSetVector - Set a vector associated with the preconditioner.

  Input Parameters:
.  pc - the preconditioner context
.  vec - the vector
@*/
int PCSetVector(PC pc,Vec vec)
{
  VALIDHEADER(pc,PC_COOKIE);
  pc->vec = vec;
  return 0;
}

/*@
     PCGetMethodFromContext - Gets the preconditioner method from an 
            active preconditioner context.

  Input Parameters:
.  pc - the preconditioner context

  Output parameters:
.  method - the method id
@*/
int PCGetMethodFromContext(PC pc,PCMETHOD *method)
{
  VALIDHEADER(pc,PC_COOKIE);
  *method = (PCMETHOD) pc->type;
  return 0;
}

/*@
    PCSetOptionsPrefix - Sets the prefix used for searching for all 
       PC options in the database.

  Input Parameters:
.  pc - the preconditioner context
.  prefix - the prefix string to prepend to all PC option requests

@*/
int PCSetOptionsPrefix(PC pc,char *prefix)
{
  pc->prefix = prefix;
  return 0;
}

int PCPreSolve(PC pc,KSP ksp)
{
  if (pc->presolve) return (*pc->presolve)(pc,ksp);
  else return 0;
}

int PCPostSolve(PC pc,KSP ksp)
{
  if (pc->postsolve) return (*pc->postsolve)(pc,ksp);
  else return 0;
}
