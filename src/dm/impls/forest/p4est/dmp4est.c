#if defined(PETSC_HAVE_P4EST)
#define _append_pforest(a) a ## _p4est
#include "pforest.c"
#endif
