#include "parsetcl.cpp"

int Tcl_AppInit(Tcl_Interp *interp){

  // Tcl_Init(interp);
  // Tcl_SourceRCFile(interp);

  //Tcl_SetStartupScript("/remote/us01home19/szhang/scratch/disk.dc/dc2nwtn/misc/tcltrace.tcl",NULL);

  // parse_tcl_command(interp);

  // Tcl_AllowExceptions(interp);

  Parsetcl_Init(interp);
  Tcl_StaticPackage(interp, "Parsetcl", Parsetcl_Init, NULL);
  /*
  if(TCL_OK != Tcl_Eval(interp,"load {} Parsetcl")){
    printf("Error: load error\n");
  }
  */

  char buf[512];
  const char *name = Tcl_GetNameOfExecutable();
  sprintf(buf, "%s.lib.tcl", name);
  Tcl_EvalFile(interp, buf);

  //Tcl_Obj *startup = Tcl_GetStartupScript(NULL);

  //parse_tcl_file(interp,"dcp558_gif_fp.cstr.tcl");

  /*
  int level = 0;
  int flags = 0; // TCL_ALLOW_INLINE_COMPILATION
  Tcl_CmdObjTraceProc	*objProc = Tcl_CmdObjTraceProc_impl;
  ClientData	clientData = 0;
  Tcl_CmdObjTraceDeleteProc	*deleteProc=NULL;
  Tcl_CreateObjTrace(interp, level, flags, objProc, clientData, deleteProc);
  */

  // Tcl_Eval(interp,"exit");
  return TCL_OK;
}

int main(int argc, char **argv){
  Tcl_Main(argc, argv, Tcl_AppInit);
  return 0;
}

