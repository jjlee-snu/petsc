/*
      Wrappers for PETSc PC ESI implementation
*/

#include "esi/petsc/solveriterative.h"
#include "esi/petsc/preconditioner.h"

esi::petsc::SolverIterative<double,int>::SolverIterative(MPI_Comm icomm)
{
  int      ierr;

  ierr = KSPCreate(icomm,&this->ksp);if (ierr) return;
  ierr = KSPSetOptionsPrefix(this->ksp,"esi_");
  ierr = KSPSetFromOptions(this->ksp);

  ierr = PetscObjectGetComm((PetscObject)this->ksp,&this->comm);if (ierr) return;
  this->op = 0;
}

esi::petsc::SolverIterative<double,int>::SolverIterative(KSP iksp)
{
  int ierr;
  this->ksp    = iksp;
  ierr = PetscObjectGetComm((PetscObject)this->ksp,&this->comm);if (ierr) return;
  ierr = PetscObjectReference((PetscObject)iksp);if (ierr) return;
}

esi::petsc::SolverIterative<double,int>::~SolverIterative()
{
  int ierr;
  ierr = PetscObjectDereference((PetscObject)this->ksp);if (ierr) return;
  if (this->op) {ierr = this->op->deleteReference();if (ierr) return;}
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::getInterface(const char* name, void *& iface)
{
  PetscTruth flg;

  if (!PetscStrcmp(name,"esi::Object",&flg),flg){
    iface = (void *) (esi::Object *) this;
  } else if (!PetscStrcmp(name,"esi::Operator",&flg),flg){
    iface = (void *) (esi::Operator<double,int> *) this;
  } else if (!PetscStrcmp(name,"esi::SolverIterative",&flg),flg){
    iface = (void *) (esi::SolverIterative<double,int> *) this;
  } else if (!PetscStrcmp(name,"esi::Solver",&flg),flg){
    iface = (void *) (esi::Solver<double,int> *) this;
  } else if (!PetscStrcmp(name,"KSP",&flg),flg){
    iface = (void *) this->ksp;
  } else if (!PetscStrcmp(name,"esi::petsc::SolverIterative",&flg),flg){
    iface = (void *) (esi::petsc::SolverIterative<double,int> *) this;
  } else {
    iface = 0;
  }
  return 0;
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::getInterfacesSupported(esi::Argv * list)
{
  list->appendArg("esi::Object");
  list->appendArg("esi::Operator");
  list->appendArg("esi::SolverIterative");
  list->appendArg("esi::Solver");
  list->appendArg("esi::petsc::SolverIterative");
  list->appendArg("KSP");
  return 0;
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::apply( esi::Vector<double,int> &xx,esi::Vector<double,int> &yy)
{
  int ierr;
  Vec py,px;

  ierr = yy.getInterface("Vec",reinterpret_cast<void*&>(py));if (ierr) return ierr;
  ierr = xx.getInterface("Vec",reinterpret_cast<void*&>(px));if (ierr) return ierr;

  KSPSetRhs(this->ksp,px);
  KSPSetSolution(this->ksp,py);
  return KSPSolve(this->ksp);
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::solve( esi::Vector<double,int> &xx,esi::Vector<double,int> &yy)
{
  return this->apply(xx,yy);
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::setup()
{
  return 0;
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::setOperator( esi::Operator<double,int> &iop)
{
  /*
        For now require Operator to be a PETSc Mat
  */
  Mat A;
  this->op = &iop;
  iop.addReference();
  int ierr = iop.getInterface("Mat",reinterpret_cast<void*&>(A));CHKERRQ(ierr);
  if (!A) {
    /* ierr = MatCreate( &A);if (ierr) return ierr;
       ierr = MatSetType(A,MATESI);if (ierr) return ierr;
       ierr = MatESISetOperator(A,&op);if (ierr) return ierr;*/
  }
  ierr = KSPSetOperators(this->ksp,A,A,DIFFERENT_NONZERO_PATTERN);CHKERRQ(ierr);
  return 0;
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::getOperator( esi::Operator<double,int> *&iop)
{
  iop = this->op;
  return 0;
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::getPreconditioner( esi::Preconditioner<double,int> *&ipre)
{
  if (!this->pre) {
    PC  pc;
        ierr  = KSPGetPC(this->ksp,&pc);if (ierr) return ierr;
    this->pre = new ::esi::petsc::Preconditioner<double,int>(pc); 
  }
  ipre = this->pre;
  return 0;
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::setPreconditioner( esi::Preconditioner<double,int> &ipre)
{
  PC  pc;
  ierr = KSPGetPC(this->ksp,&pc);if (ierr) return ierr;
  ierr = PCSetType(pc,PCESI);if (ierr) return ierr;
  ierr = PCESISetPreconditioner(pc,&ipre);if (ierr) return ierr;
  if (this->pre) {ierr = this->pre->deleteReference();if (ierr) return ierr;}
  this->pre = &ipre;
  ierr = ipre.addReference();
  return 0;
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::parameters(int numParams, char** paramStrings)
{
  return 1;
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::getTolerance( magnitude_type & tol )
{
  int ierr;
  ierr = KSPGetTolerances(this->ksp,&tol,0,0,0);if (ierr) return ierr;
  return 0;
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::setTolerance( magnitude_type  tol )
{
  int ierr;
  ierr = KSPSetTolerances(this->ksp,tol,PETSC_DEFAULT,PETSC_DEFAULT,PETSC_DEFAULT);if (ierr) return ierr;
  return 0;
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::setMaxIterations(int its)
{
  int ierr;
  ierr = KSPSetTolerances(this->ksp,PETSC_DEFAULT,PETSC_DEFAULT,PETSC_DEFAULT,its);if (ierr) return ierr;
  return 0;
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::getMaxIterations(int &its)
{
  int ierr;
  ierr = KSPGetTolerances(this->ksp,0,0,0,&its);if (ierr) return ierr;
  return 0;
}

esi::ErrorCode esi::petsc::SolverIterative<double,int>::getNumIterationsTaken(int &its)
{
  int ierr;
  ierr = KSPGetIterationNumber(this->ksp,&its);if (ierr) return ierr;
  return 0;
}

// --------------------------------------------------------------------------
::esi::ErrorCode esi::petsc::SolverIterative<double,int>::Factory::create (char *commname,void *icomm,::esi::SolverIterative<double,int>*&v)
{
  PetscTruth flg;
  int        ierr = PetscStrcmp(commname,"MPI",&flg);CHKERRQ(ierr);
  v = new esi::petsc::SolverIterative<double,int>(*(MPI_Comm*)icomm);
  return 0;
};

//::esi::petsc::SolverIterativeFactory<double,int> SFInstForIntel64CompilerBug;

EXTERN_C_BEGIN
::esi::SolverIterative<double,int>::Factory *create_esi_petsc_solveriterativefactory(void)
{
  return dynamic_cast< ::esi::SolverIterative<double,int>::Factory *>(new esi::petsc::SolverIterative<double,int>::Factory);
}
EXTERN_C_END
