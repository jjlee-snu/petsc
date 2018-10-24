
static char help[] = "Reads a PETSc matrix and computes the 2 norm of the columns\n\n";

/*T
   Concepts: Mat^loading a binary matrix;
   Processors: n
T*/

/*
  Include "petscmat.h" so that we can use matrices.
  automatically includes:
     petscsys.h       - base PETSc routines   petscvec.h    - vectors
     petscmat.h    - matrices
     petscis.h     - index sets            petscviewer.h - viewers
*/
#include <petscmat.h>


int main(int argc,char **args)
{
  Mat            A;                       /* matrix */
  PetscViewer    fd;                      /* viewer */
  char           file[PETSC_MAX_PATH_LEN];            /* input file name */
  PetscErrorCode ierr;
  PetscReal      *norms;
  PetscInt       n,cstart,cend;
  PetscBool      flg;

  ierr = PetscInitialize(&argc,&args,(char*)0,help);if (ierr) return ierr;
  /*
     Determine files from which we read the matrix
  */
  ierr = PetscOptionsGetString(NULL,NULL,"-f",file,PETSC_MAX_PATH_LEN,&flg);CHKERRQ(ierr);
  if (!flg) SETERRQ(PETSC_COMM_WORLD,1,"Must indicate binary file with the -f option");

  /*
     Open binary file.  Note that we use FILE_MODE_READ to indicate
     reading from this file.
  */
  ierr = PetscViewerCreate(PETSC_COMM_WORLD,&fd);CHKERRQ(ierr);
  ierr = PetscViewerSetType(fd,PETSCVIEWERBINARY);CHKERRQ(ierr);
  ierr = PetscViewerSetFromOptions(fd);CHKERRQ(ierr);
  ierr = PetscViewerFileSetMode(fd,FILE_MODE_READ);CHKERRQ(ierr);
  ierr = PetscViewerFileSetName(fd,file);CHKERRQ(ierr);

  /*
    Load the matrix; then destroy the viewer.
  */
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetOptionsPrefix(A,"a_");CHKERRQ(ierr);
  ierr = PetscObjectSetName((PetscObject) A,"A");CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatLoad(A,fd);CHKERRQ(ierr);
  ierr = PetscViewerDestroy(&fd);CHKERRQ(ierr);

  ierr = MatGetSize(A,NULL,&n);CHKERRQ(ierr);
  ierr = MatGetOwnershipRangeColumn(A,&cstart,&cend);CHKERRQ(ierr);
  ierr = PetscMalloc1(n,&norms);CHKERRQ(ierr);
  ierr = MatGetColumnNorms(A,NORM_2,norms);CHKERRQ(ierr);
  ierr = PetscRealView(cend-cstart,norms+cstart,PETSC_VIEWER_STDOUT_WORLD);CHKERRQ(ierr);
  ierr = PetscFree(norms);CHKERRQ(ierr);

  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = PetscFinalize();
  return ierr;
}



/*TEST

   test:
      suffix: mpiaij
      nsize: 2
      requires: datafilespath double !complex !define(PETSC_USE_64BIT_INDICES)
      args: -f ${DATAFILESPATH}/matrices/small -a_mat_type mpiaij

   test:
      suffix: mpiaij_hdf5
      nsize: 2
      requires: datafilespath double !complex !define(PETSC_USE_64BIT_INDICES) hdf5 zlib
      args: -f ${DATAFILESPATH}/matrices/small.mat -a_mat_type mpiaij -viewer_type hdf5

   test:
      # test for more processes than rows
      #TODO hang
      suffix: mpiaij_hdf5_tiny
      nsize: 8
      requires: double !complex !define(PETSC_USE_64BIT_INDICES) hdf5 zlib
      args: -f ${wPETSC_DIR}/share/petsc/datafiles/matrices/tiny_system_with_x0.mat -a_mat_type mpiaij -viewer_type hdf5

   test:
      suffix: mpidense
      nsize: 2
      requires: datafilespath double !complex !define(PETSC_USE_64BIT_INDICES)
      args: -f ${DATAFILESPATH}/matrices/small -a_mat_type mpidense

   test:
      suffix: seqaij
      requires: datafilespath double !complex !define(PETSC_USE_64BIT_INDICES)
      args: -f ${DATAFILESPATH}/matrices/small -a_mat_type seqaij

   test:
      suffix: seqaij_hdf5
      requires: datafilespath double !complex !define(PETSC_USE_64BIT_INDICES) hdf5 zlib
      args: -f ${DATAFILESPATH}/matrices/small.mat -a_mat_type seqaij -viewer_type hdf5

   test:
      suffix: seqdense
      requires: datafilespath double !complex !define(PETSC_USE_64BIT_INDICES)
      args: -f ${DATAFILESPATH}/matrices/small -a_mat_type seqdense

TEST*/
