#!/usr/bin/env python
from __future__ import generators
import user
import config.base
import os
import PETSc.package

class Configure(PETSc.package.Package):
  def __init__(self, framework):
    PETSc.package.Package.__init__(self, framework)
    self.download          = ['ftp://ftp.mcs.anl.gov/pub/petsc/externalpackages/Prometheus-1.8.1-Petsc-2.3.tar.gz']
    self.functions         = []
    self.includes          = []
    self.liblist           = [['libpromfei.a','libprometheus.a']]
    self.compilePrometheus = 0
    return

  def setupDependencies(self, framework):
    PETSc.package.Package.setupDependencies(self, framework)
    self.mpi        = framework.require('PETSc.packages.MPI',self)
    self.blasLapack = framework.require('PETSc.packages.BlasLapack',self)
    self.parmetis   = framework.require('PETSc.packages.ParMetis',self)
    self.deps       = [self.parmetis,self.mpi,self.blasLapack]
    return

  def generateLibList(self,dir):
    '''Normally the one in package.py is used, but Prometheus requires the extra C++ library'''
    alllibs = PETSc.package.Package.generateLibList(self,dir)
    import config.setCompilers
    if self.languages.clanguage == 'C':
      alllibs[0].extend(self.setCompilers.cxxlibs)
    return alllibs

  def Install(self):
    prometheusDir = self.getDir()
    installDir = os.path.join(prometheusDir, self.arch.arch)
    if not os.path.isdir(os.path.join(installDir,'lib')):
      os.mkdir(os.path.join(installDir,'lib'))
      os.mkdir(os.path.join(installDir,'include'))            
    self.framework.pushLanguage('C')
    args  = 'C_CC = '+self.framework.getCompiler()+'\n'
    args += 'PETSC_INCLUDE = -I'+os.path.join(self.arch.dir,'bmake',self.arch.arch)+' -I'+os.path.join(self.arch.dir)+' -I'+os.path.join(self.arch.dir,'include')+' '+' '.join([self.headers.getIncludeArgument(inc) for inc in self.mpi.include+self.parmetis.include])+'\n'
    args += 'BUILD_DIR  = '+prometheusDir+'\n'
    args += 'LIB_DIR  = $(BUILD_DIR)/lib/\n'
    args += 'RANLIB = '+self.setCompilers.RANLIB+'\n'
    self.framework.popLanguage()
    self.framework.pushLanguage('C++')+'\n'
    if self.languages.clanguage == 'C':
      args += 'CC = '+self.framework.getCompiler()+' -DPETSC_USE_EXTERN_CXX'    
    else:
      args += 'CC = '+self.framework.getCompiler()    
    args += ' -DPROM_HAVE_METIS'
    # Instead of doing all this, we could try to have Prometheus just use the PETSc bmake
    # files. But need to pass in USE_EXTERN_CXX flag AND have a C and C++ compiler
    if self.compilers.fortranMangling == 'underscore':
      args += ' -DHAVE_FORTRAN_UNDERSCORE=1'
      if self.compilers.fortranManglingDoubleUnderscore:
        args += ' -DHAVE_FORTRAN_UNDERSCORE_UNDERSCORE=1'
    elif self.blasLapack.f2c:
      args += ' -DBLASLAPACK_UNDERSCORE=1'
    elif self.compilers.fortranMangling == 'unchanged':
      args += ' -DHAVE_FORTRAN_NOUNDERSCORE=1'
    elif self.compilers.fortranMangling == 'capitalize':
      args += ' -DHAVE_FORTRAN_CAPS=1'
    elif self.compilers.fortranMangling == 'stdcall':
      args += ' -DHAVE_FORTRAN_STDCALL=1'
      args += ' -DSTDCALL=__stdcall'
      args += ' -DHAVE_FORTRAN_CAPS=1'
      args += ' -DHAVE_FORTRAN_MIXED_STR_ARG=1'
 
    args += '\nPETSCFLAGS = '+self.framework.getCompilerFlags()+'\n'
    self.framework.popLanguage()
    try:
      fd      = file(os.path.join(installDir,'makefile.petsc'))
      oldargs = fd.readline()
      fd.close()
    except:
      oldargs = ''
    if not oldargs == args:
      self.framework.log.write('Have to rebuild Prometheus oldargs = '+oldargs+'\n new args = '+args+'\n')
      self.logPrintBox('Configuring Prometheus; this may take a minute')
      fd = file(os.path.join(installDir,'makefile.petsc'),'w')
      fd.write(args)
      fd.close()
      fd = file(os.path.join(prometheusDir,'makefile.petsc'),'w')
      fd.write(args)
      fd.close()
      fd = file(os.path.join(prometheusDir,'makefile.in'),'a')
      fd.write('include makefile.petsc\n')
      fd.close()
      self.compilePrometheus = 1
      self.prometheusDir     = prometheusDir
      self.installDir        = installDir
    return prometheusDir

  def configureLibrary(self):
    '''Calls the regular package configureLibrary and then does an additional test needed by Prometheus'''
    '''Normally you do not need to provide this method'''
    PETSc.package.Package.configureLibrary(self)
    # Prometheus requires LAPACK routine dorgqr()
    if not self.blasLapack.checkForRoutine('dorgqr'):
      raise RuntimeError('Prometheus requires the LAPACK routine dorgqr(), the current Lapack libraries '+str(self.blasLapack.lib)+' does not have it\nIf you are using the IBM ESSL library, it does not contain this function. After installing a complete copy of lapack\n You can run config/configure.py with --with-blas-lib=libessl.a --with-lapack-lib=/usr/local/lib/liblapack.a')
    self.framework.log.write('Found dorgqr() in Lapack library as needed by Prometheus\n')
    return

  def postProcess(self):
    if self.compilePrometheus:
      self.logPrintBox('Compiling Prometheus; this may take several minutes')
      output  = config.base.Configure.executeShellCommand('cd '+self.prometheusDir+'; make prom; mv '+os.path.join('lib','lib*.a')+' '+os.path.join(self.installDir,'lib'),timeout=250, log = self.framework.log)[0]
      self.framework.log.write(output)
      output  = config.base.Configure.executeShellCommand('cp '+os.path.join(self.prometheusDir,'include','*.*')+' '+os.path.join(self.prometheusDir,'fei_prom','*.h')+' '+os.path.join(self.installDir,'include'),timeout=250, log = self.framework.log)[0]      
      self.framework.log.write(output)
      try:
        output  = config.base.Configure.executeShellCommand(self.setCompilers.RANLIB+' '+os.path.join(self.installDir,'lib')+'/lib*.a', timeout=250, log = self.framework.log)[0]
        self.framework.log.write(output)
      except RuntimeError, e:
        raise RuntimeError('Error running ranlib on PROMETHEUS libraries: '+str(e))

if __name__ == '__main__':
  import config.framework
  import sys
  framework = config.framework.Framework(sys.argv[1:])
  framework.setupLogging(framework.clArgs)
  framework.children.append(Configure(framework))
  framework.configure()
  framework.dumpSubstitutions()
