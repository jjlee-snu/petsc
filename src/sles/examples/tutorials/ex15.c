/*$Id: ex15.c,v 1.24 2001/08/07 21:30:54 bsmith Exp $*/

static char help[] = "Solves a linear system in parallel with KSP.  Also\n\
illustrates setting a user-defined shell preconditioner and using the\n\
macro __FUNCT__ to define routine names for use in error handling.\n\
Input parameters include:\n\
  -user_defined_pc : Activate a user-defined preconditioner\n\n";

/*T
   Concepts: KSP^basic parallel example
   Concepts: PC^setting a user-defined shell preconditioner
   Concepts: error handling^Using the macro __FUNCT__ to define routine names;
   Processors: n
T*/

/* 
  Include "petscksp.h" so that we can use KSP solvers.  Note that this file
  automatically includes:
     petsc.h       - base PETSc routines   petscvec.h - vectors
     petscsys.h    - system routines       petscmat.h - matrices
     petscis.h     - index sets            petscksp.h - Krylov subspace methods
     petscviewer.h - viewers               petscpc.h  - preconditioners
*/
#include "petscksp.h"

/* Define context for user-provided preconditioner */
typedef struct {
  Vec diag;
} SampleShellPC;

/* Declare routines for user-provided preconditioner */
extern int SampleShellPCCreate(SampleShellPC**);
extern int SampleShellPCSetUp(SampleShellPC*,Mat,Vec);
extern int SampleShellPCApply(void*,Vec x,Vec y);
extern int SampleShellPCDestroy(SampleShellPC*);

/* 
   User-defined routines.  Note that immediately before each routine below,
   we define the macro __FUNCT__ to be a string containing the routine name.
   If defined, this macro is used in the PETSc error handlers to provide a
   complete traceback of routine names.  All PETSc library routines use this
   macro, and users can optionally employ it as well in their application
   codes.  Note that users can get a traceback of PETSc errors regardless of
   whether they define __FUNCT__ in application codes; this macro merely
   provides the added traceback detail of the application routine names.
*/

#undef __FUNCT__  
#define __FUNCT__ "main"
int main(int argc,char **args)
{
  Vec           x,b,u;   /* approx solution, RHS, exact solution */
  Mat           A;         /* linear system matrix */
  KSP           ksp;      /* linear solver context */
  PC            pc;        /* preconditioner context */
  PetscReal     norm;      /* norm of solution error */
  SampleShellPC *shell;    /* user-defined preconditioner context */
  PetscScalar   v,one = 1.0,none = -1.0;
  int           i,j,I,J,Istart,Iend,ierr,m = 8,n = 7,its;
  PetscTruth    user_defined_pc;

  PetscInitialize(&argc,&args,(char *)0,help);
  ierr = PetscOptionsGetInt(PETSC_NULL,"-m",&m,PETSC_NULL);CHKERRQ(ierr);
  ierr = PetscOptionsGetInt(PETSC_NULL,"-n",&n,PETSC_NULL);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
         Compute the matrix and right-hand-side vector that define
         the linear system, Ax = b.
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  /* 
     Create parallel matrix, specifying only its global dimensions.
     When using MatCreate(), the matrix format can be specified at
     runtime. Also, the parallel partioning of the matrix is
     determined by PETSc at runtime.
  */
  ierr = MatCreate(PETSC_COMM_WORLD,PETSC_DECIDE,PETSC_DECIDE,m*n,m*n,&A);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);

  /* 
     Currently, all PETSc parallel matrix formats are partitioned by
     contiguous chunks of rows across the processors.  Determine which
     rows of the matrix are locally owned. 
  */
  ierr = MatGetOwnershipRange(A,&Istart,&Iend);CHKERRQ(ierr);

  /* 
     Set matrix elements for the 2-D, five-point stencil in parallel.
      - Each processor needs to insert only elements that it owns
        locally (but any non-local elements will be sent to the
        appropriate processor during matrix assembly). 
      - Always specify global rows and columns of matrix entries.
   */
  for (I=Istart; I<Iend; I++) { 
    v = -1.0; i = I/n; j = I - i*n;  
    if (i>0)   {J = I - n; ierr = MatSetValues(A,1,&I,1,&J,&v,INSERT_VALUES);CHKERRQ(ierr);}
    if (i<m-1) {J = I + n; ierr = MatSetValues(A,1,&I,1,&J,&v,INSERT_VALUES);CHKERRQ(ierr);}
    if (j>0)   {J = I - 1; ierr = MatSetValues(A,1,&I,1,&J,&v,INSERT_VALUES);CHKERRQ(ierr);}
    if (j<n-1) {J = I + 1; ierr = MatSetValues(A,1,&I,1,&J,&v,INSERT_VALUES);CHKERRQ(ierr);}
    v = 4.0; ierr = MatSetValues(A,1,&I,1,&I,&v,INSERT_VALUES);CHKERRQ(ierr);
  }

  /* 
     Assemble matrix, using the 2-step process:
       MatAssemblyBegin(), MatAssemblyEnd()
     Computations can be done while messages are in transition
     by placing code between these two statements.
  */
  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);

  /* 
     Create parallel vectors.
      - When using VecCreate() VecSetSizes() and VecSetFromOptions(),
        we specify only the vector's global
        dimension; the parallel partitioning is determined at runtime. 
      - Note: We form 1 vector from scratch and then duplicate as needed.
  */
  ierr = VecCreate(PETSC_COMM_WORLD,&u);CHKERRQ(ierr);
  ierr = VecSetSizes(u,PETSC_DECIDE,m*n);CHKERRQ(ierr);
  ierr = VecSetFromOptions(u);CHKERRQ(ierr);
  ierr = VecDuplicate(u,&b);CHKERRQ(ierr); 
  ierr = VecDuplicate(b,&x);CHKERRQ(ierr);

  /* 
     Set exact solution; then compute right-hand-side vector.
  */
  ierr = VecSet(&one,u);CHKERRQ(ierr);
  ierr = MatMult(A,u,b);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
                Create the linear solver and set various options
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  /* 
     Create linear solver context
  */
  ierr = KSPCreate(PETSC_COMM_WORLD,&ksp);CHKERRQ(ierr);

  /* 
     Set operators. Here the matrix that defines the linear system
     also serves as the preconditioning matrix.
  */
  ierr = KSPSetOperators(ksp,A,A,DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);

  /* 
     Set linear solver defaults for this problem (optional).
     - By extracting the KSP and PC contexts from the KSP context,
       we can then directly call any KSP and PC routines
       to set various options.
  */
  ierr = KSPGetPC(ksp,&pc);CHKERRQ(ierr);
  ierr = KSPSetTolerances(ksp,1.e-7,PETSC_DEFAULT,PETSC_DEFAULT,
         PETSC_DEFAULT);CHKERRQ(ierr);

  /*
     Set a user-defined "shell" preconditioner if desired
  */
  ierr = PetscOptionsHasName(PETSC_NULL,"-user_defined_pc",&user_defined_pc);CHKERRQ(ierr);
  if (user_defined_pc) {
    /* (Required) Indicate to PETSc that we're using a "shell" preconditioner */
    ierr = PCSetType(pc,PCSHELL);CHKERRQ(ierr);

    /* (Optional) Create a context for the user-defined preconditioner; this
       context can be used to contain any application-specific data. */
    ierr = SampleShellPCCreate(&shell);CHKERRQ(ierr);

    /* (Required) Set the user-defined routine for applying the preconditioner */
    ierr = PCShellSetApply(pc,SampleShellPCApply,(void*)shell);CHKERRQ(ierr);

    /* (Optional) Set a name for the preconditioner, used for PCView() */
    ierr = PCShellSetName(pc,"MyPreconditioner");CHKERRQ(ierr);

    /* (Optional) Do any setup required for the preconditioner */
    ierr = SampleShellPCSetUp(shell,A,x);CHKERRQ(ierr);

  } else {
    ierr = PCSetType(pc,PCJACOBI);CHKERRQ(ierr);
  }

  /* 
    Set runtime options, e.g.,
        -ksp_type <type> -pc_type <type> -ksp_monitor -ksp_rtol <rtol>
    These options will override those specified above as long as
    KSPSetFromOptions() is called _after_ any other customization
    routines.
  */
  ierr = KSPSetFromOptions(ksp);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
                      Solve the linear system
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ierr = KSPSetRhs(ksp,b);CHKERRQ(ierr);
  ierr = KSPSetSolution(ksp,x);CHKERRQ(ierr);
  ierr = KSPSolve(ksp);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
                      Check solution and clean up
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  /* 
     Check the error
  */
  ierr = VecAXPY(&none,u,x);CHKERRQ(ierr);
  ierr = VecNorm(x,NORM_2,&norm);CHKERRQ(ierr);
  ierr = KSPGetIterationNumber(ksp,&its);CHKERRQ(ierr);
  ierr = PetscPrintf(PETSC_COMM_WORLD,"Norm of error %A iterations %d\n",norm,its);CHKERRQ(ierr);

  /* 
     Free work space.  All PETSc objects should be destroyed when they
     are no longer needed.
  */
  ierr = KSPDestroy(ksp);CHKERRQ(ierr);
  ierr = VecDestroy(u);CHKERRQ(ierr);  ierr = VecDestroy(x);CHKERRQ(ierr);
  ierr = VecDestroy(b);CHKERRQ(ierr);  ierr = MatDestroy(A);CHKERRQ(ierr);

  if (user_defined_pc) {
    ierr = SampleShellPCDestroy(shell);CHKERRQ(ierr);
  }

  ierr = PetscFinalize();CHKERRQ(ierr);
  return 0;

}

/***********************************************************************/
/*          Routines for a user-defined shell preconditioner           */
/***********************************************************************/

#undef __FUNCT__  
#define __FUNCT__ "SampleShellPCCreate"
/*
   SampleShellPCCreate - This routine creates a user-defined
   preconditioner context.

   Output Parameter:
.  shell - user-defined preconditioner context
*/
int SampleShellPCCreate(SampleShellPC **shell)
{
  SampleShellPC *newctx;
  int           ierr;

  ierr         = PetscNew(SampleShellPC,&newctx);CHKERRQ(ierr);
  newctx->diag = 0;
  *shell       = newctx;
  return 0;
}
/* ------------------------------------------------------------------- */
#undef __FUNCT__  
#define __FUNCT__ "SampleShellPCSetUp"
/*
   SampleShellPCSetUp - This routine sets up a user-defined
   preconditioner context.  

   Input Parameters:
.  shell - user-defined preconditioner context
.  pmat  - preconditioner matrix
.  x     - vector

   Output Parameter:
.  shell - fully set up user-defined preconditioner context

   Notes:
   In this example, we define the shell preconditioner to be Jacobi's
   method.  Thus, here we create a work vector for storing the reciprocal
   of the diagonal of the preconditioner matrix; this vector is then
   used within the routine SampleShellPCApply().
*/
int SampleShellPCSetUp(SampleShellPC *shell,Mat pmat,Vec x)
{
  Vec diag;
  int ierr;

  ierr = VecDuplicate(x,&diag);CHKERRQ(ierr);
  ierr = MatGetDiagonal(pmat,diag);CHKERRQ(ierr);
  ierr = VecReciprocal(diag);CHKERRQ(ierr);
  shell->diag = diag;

  return 0;
}
/* ------------------------------------------------------------------- */
#undef __FUNCT__  
#define __FUNCT__ "SampleShellPCApply"
/*
   SampleShellPCApply - This routine demonstrates the use of a
   user-provided preconditioner.

   Input Parameters:
.  ctx - optional user-defined context, as set by PCShellSetApply()
.  x - input vector

   Output Parameter:
.  y - preconditioned vector

   Notes:
   Note that the PCSHELL preconditioner passes a void pointer as the
   first input argument.  This can be cast to be the whatever the user
   has set (via PCSetShellApply()) the application-defined context to be.

   This code implements the Jacobi preconditioner, merely as an
   example of working with a PCSHELL.  Note that the Jacobi method
   is already provided within PETSc.
*/
int SampleShellPCApply(void *ctx,Vec x,Vec y)
{
  SampleShellPC *shell = (SampleShellPC*)ctx;
  int           ierr;

  ierr = VecPointwiseMult(x,shell->diag,y);CHKERRQ(ierr);

  return 0;
}
/* ------------------------------------------------------------------- */
#undef __FUNCT__  
#define __FUNCT__ "SampleShellPCDestroy"
/*
   SampleShellPCDestroy - This routine destroys a user-defined
   preconditioner context.

   Input Parameter:
.  shell - user-defined preconditioner context
*/
int SampleShellPCDestroy(SampleShellPC *shell)
{
  int ierr;

  ierr = VecDestroy(shell->diag);CHKERRQ(ierr);
  ierr = PetscFree(shell);CHKERRQ(ierr);

  return 0;
}
