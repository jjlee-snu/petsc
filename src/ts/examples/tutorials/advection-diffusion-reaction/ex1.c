
static char help[] = "Nonlinear Reaction Problem from Chemistry.\n";

/*F

     This directory contains examples based on the PDES/ODES given in the book
      Numerical Solution of Time-Dependent Advection-Diffusion-Reaction Equations by
      W. Hundsdorf and J.G. Verwer

     Page 3, Section 1.1 Nonlinear Reaction Problems from Chemistry

\begin{eqnarray}
                 {U_1}_t  - k U_1 U_2  & = & 0 \\
                 {U_2}_t  - k U_1 U_2 & = & 0 \\
                 {U_3}_t  - k U_1 U_2 & = & 0
\end{eqnarray}

     Helpful runtime monitoring options:
         -ts_view                  -  prints information about the solver being used
         -ts_monitor               -  prints the progess of the solver
         -ts_adapt_monitor         -  prints the progress of the time-step adaptor
         -ts_monitor_lg_timestep   -  plots the size of each timestep (at each time-step)
         -ts_monitor_lg_solution   -  plots each component of the solution as a function of time (at each timestep)
         -ts_monitor_lg_error      -  plots each component of the error in the solution as a function of time (at each timestep)
         -draw_pause -2            -  hold the plots a the end of the solution process, enter a mouse press in each window to end the process

         -ts_monitor_lg_timestep -1  -  plots the size of each timestep (at the end of the solution process)
         -ts_monitor_lg_solution -1  -  plots each component of the solution as a function of time (at the end of the solution process)
         -ts_monitor_lg_error -1     -  plots each component of the error in the solution as a function of time (at the end of the solution process)
         -lg_use_markers false       -  do NOT show the data points on the plots
         -draw_save                  -  save the timestep and solution plot as a .Gif image file

F*/

/*
      Project: Generate a nicely formated HTML page using
         1) the HTML version of the source code and text in this file, use make html to generate the file ex1.c.html
         2) the images (Draw_XXX_0_0.Gif Draw_YYY_0_0.Gif Draw_ZZZ_1_0.Gif) and
         3) the text output (output.txt) generated by running the following commands.
         4) <iframe src="generated_topics.html" scrolling="no" frameborder="0"  width=600 height=300></iframe>

      rm -rf *.Gif
      ./ex1 -ts_monitor_lg_error -1 -ts_monitor_lg_solution -1   -draw_pause -2 -ts_max_steps 100 -ts_monitor_lg_timestep -1 -draw_save -draw_virtual -ts_monitor -ts_adapt_monitor -ts_view  > output.txt

      For example something like
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN">
<html>
  <head>
    <meta http-equiv="content-type" content="text/html;charset=utf-8">
    <title>PETSc Example -- Nonlinear Reaction Problem from Chemistry</title>
  </head>
  <body>
  <iframe src="ex1.c.html" scrolling="yes" frameborder="1"  width=2000 height=400></iframe>
  <img alt="" src="Draw_0x84000000_0_0.Gif"/><img alt="" src="Draw_0x84000001_0_0.Gif"/><img alt="" src="Draw_0x84000001_1_0.Gif"/>
  <iframe src="output.txt" scrolling="yes" frameborder="1"  width=2000 height=1000></iframe>
  </body>
</html>

*/

/*
   Include "petscts.h" so that we can use TS solvers.  Note that this
   file automatically includes:
     petscsys.h       - base PETSc routines   petscvec.h - vectors
     petscmat.h - matrices
     petscis.h     - index sets            petscksp.h - Krylov subspace methods
     petscviewer.h - viewers               petscpc.h  - preconditioners
     petscksp.h   - linear solvers
*/
#include <petscts.h>

typedef struct {
  PetscScalar k;
  Vec         initialsolution;
} AppCtx;

#undef __FUNCT__
#define __FUNCT__ "IFunctionView"
PetscErrorCode IFunctionView(AppCtx *ctx,PetscViewer v)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscViewerBinaryWrite(v,&ctx->k,1,PETSC_SCALAR,PETSC_FALSE);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "IFunctionLoad"
PetscErrorCode IFunctionLoad(AppCtx **ctx,PetscViewer v)
{
  PetscErrorCode ierr;

  PetscFunctionBegin;
  ierr = PetscMalloc(sizeof(AppCtx),ctx);CHKERRQ(ierr);
  ierr = PetscViewerBinaryRead(v,&(*ctx)->k,1,PETSC_SCALAR);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "IFunction"
/*
     Defines the ODE passed to the ODE solver
*/
PetscErrorCode IFunction(TS ts,PetscReal t,Vec U,Vec Udot,Vec F,AppCtx *ctx)
{
  PetscErrorCode    ierr;
  PetscScalar       *f;
  const PetscScalar *u,*udot;

  PetscFunctionBegin;
  /*  The next three lines allow us to access the entries of the vectors directly */
  ierr = VecGetArrayRead(U,&u);CHKERRQ(ierr);
  ierr = VecGetArrayRead(Udot,&udot);CHKERRQ(ierr);
  ierr = VecGetArray(F,&f);CHKERRQ(ierr);
  f[0] = udot[0] + ctx->k*u[0]*u[1];
  f[1] = udot[1] + ctx->k*u[0]*u[1];
  f[2] = udot[2] - ctx->k*u[0]*u[1];
  ierr = VecRestoreArrayRead(U,&u);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(Udot,&udot);CHKERRQ(ierr);
  ierr = VecRestoreArray(F,&f);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "IJacobian"
/*
     Defines the Jacobian of the ODE passed to the ODE solver. See TSSetIJacobian() for the meaning of a and the Jacobian.
*/
PetscErrorCode IJacobian(TS ts,PetscReal t,Vec U,Vec Udot,PetscReal a,Mat A,Mat B,AppCtx *ctx)
{
  PetscErrorCode    ierr;
  PetscInt          rowcol[] = {0,1,2};
  PetscScalar       J[3][3];
  const PetscScalar *u,*udot;

  PetscFunctionBegin;
  ierr    = VecGetArrayRead(U,&u);CHKERRQ(ierr);
  ierr    = VecGetArrayRead(Udot,&udot);CHKERRQ(ierr);
  J[0][0] = a + ctx->k*u[1];   J[0][1] = ctx->k*u[0];       J[0][2] = 0.0;
  J[1][0] = ctx->k*u[1];       J[1][1] = a + ctx->k*u[0];   J[1][2] = 0.0;
  J[2][0] = -ctx->k*u[1];      J[2][1] = -ctx->k*u[0];      J[2][2] = a;
  ierr    = MatSetValues(B,3,rowcol,3,rowcol,&J[0][0],INSERT_VALUES);CHKERRQ(ierr);
  ierr    = VecRestoreArrayRead(U,&u);CHKERRQ(ierr);
  ierr    = VecRestoreArrayRead(Udot,&udot);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  if (A != B) {
    ierr = MatAssemblyBegin(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "Solution"
/*
     Defines the exact (analytic) solution to the ODE
*/
static PetscErrorCode Solution(TS ts,PetscReal t,Vec U,AppCtx *ctx)
{
  PetscErrorCode    ierr;
  const PetscScalar *uinit;
  PetscScalar       *u,d0,q;

  PetscFunctionBegin;
  ierr = VecGetArrayRead(ctx->initialsolution,&uinit);CHKERRQ(ierr);
  ierr = VecGetArray(U,&u);CHKERRQ(ierr);
  d0   = uinit[0] - uinit[1];
  if (d0 == 0.0) q = ctx->k*t;
  else q = (1.0 - PetscExpScalar(-ctx->k*t*d0))/d0;
  u[0] = uinit[0]/(1.0 + uinit[1]*q);
  u[1] = u[0] - d0;
  u[2] = uinit[1] + uinit[2] - u[1];
  ierr = VecRestoreArray(U,&u);CHKERRQ(ierr);
  ierr = VecRestoreArrayRead(ctx->initialsolution,&uinit);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  TS             ts;            /* ODE integrator */
  Vec            U;             /* solution will be stored here */
  Mat            A;             /* Jacobian matrix */
  PetscErrorCode ierr;
  PetscMPIInt    size;
  PetscInt       n = 3;
  AppCtx         ctx;
  PetscScalar    *u;
  const char     * const names[] = {"U1","U2","U3",NULL};

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Initialize program
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  if (size > 1) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_SUP,"Only for sequential runs");

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Create necessary matrix and vectors
    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,n,n,PETSC_DETERMINE,PETSC_DETERMINE);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);

  ierr = MatCreateVecs(A,&U,NULL);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Set runtime options
    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ctx.k = .9;
  ierr  = PetscOptionsGetScalar(NULL,"-k",&ctx.k,NULL);CHKERRQ(ierr);
  ierr  = VecDuplicate(U,&ctx.initialsolution);CHKERRQ(ierr);
  ierr  = VecGetArray(ctx.initialsolution,&u);CHKERRQ(ierr);
  u[0]  = 1;
  u[1]  = .7;
  u[2]  = 0;
  ierr  = VecRestoreArray(ctx.initialsolution,&u);CHKERRQ(ierr);
  ierr  = PetscOptionsGetVec(NULL,"-initial",ctx.initialsolution,NULL);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create timestepping solver context
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = TSCreate(PETSC_COMM_WORLD,&ts);CHKERRQ(ierr);
  ierr = TSSetProblemType(ts,TS_NONLINEAR);CHKERRQ(ierr);
  ierr = TSSetType(ts,TSROSW);CHKERRQ(ierr);
  ierr = TSSetIFunction(ts,NULL,(TSIFunction) IFunction,&ctx);CHKERRQ(ierr);
  ierr = TSSetIJacobian(ts,A,A,(TSIJacobian)IJacobian,&ctx);CHKERRQ(ierr);
  ierr = TSSetSolutionFunction(ts,(TSSolutionFunction)Solution,&ctx);CHKERRQ(ierr);

  {
    DM   dm;
    void *ptr;
    ierr = TSGetDM(ts,&dm);CHKERRQ(ierr);
    ierr = PetscDLSym(NULL,"IFunctionView",&ptr);CHKERRQ(ierr);
    ierr = PetscDLSym(NULL,"IFunctionLoad",&ptr);CHKERRQ(ierr);
    ierr = DMTSSetIFunctionSerialize(dm,(PetscErrorCode (*)(void*,PetscViewer))IFunctionView,(PetscErrorCode (*)(void**,PetscViewer))IFunctionLoad);CHKERRQ(ierr);
    ierr = DMTSSetIJacobianSerialize(dm,(PetscErrorCode (*)(void*,PetscViewer))IFunctionView,(PetscErrorCode (*)(void**,PetscViewer))IFunctionLoad);CHKERRQ(ierr);
  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set initial conditions
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = Solution(ts,0,U,&ctx);CHKERRQ(ierr);
  ierr = TSSetSolution(ts,U);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set solver options
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = TSSetDuration(ts,1000,20.0);CHKERRQ(ierr);
  ierr = TSSetInitialTimeStep(ts,0.0,.001);CHKERRQ(ierr);
  ierr = TSSetFromOptions(ts);CHKERRQ(ierr);
  ierr = TSMonitorLGSetVariableNames(ts,names);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Solve nonlinear system
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = TSSolve(ts,U);CHKERRQ(ierr);

  ierr = TSView(ts,PETSC_VIEWER_BINARY_WORLD);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Free work space.  All PETSc objects should be destroyed when they are no longer needed.
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = VecDestroy(&ctx.initialsolution);CHKERRQ(ierr);
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = VecDestroy(&U);CHKERRQ(ierr);
  ierr = TSDestroy(&ts);CHKERRQ(ierr);

  ierr = PetscFinalize();
  return(0);
}
