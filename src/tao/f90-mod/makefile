#requiresdefine   'PETSC_HAVE_FORTRAN'
#requiresdefine   'PETSC_USING_F90'
#

CFLAGS	   =
FFLAGS	   =
SOURCEC	   =
SOURCEF	   = petsctaomod.F
SOURCEH	   = petsctao.h petsctao.h90
LIBBASE	   = libpetsctao
MANSEC	   = Tao
LOCDIR	   = src/tao/f90-mod/
CLEANFILES = *.mod

include ${PETSC_DIR}/lib/petsc/conf/variables
include ${PETSC_DIR}/lib/petsc/conf/rules
include ${PETSC_DIR}/lib/petsc/conf/test
