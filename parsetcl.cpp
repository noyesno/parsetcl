
#include <tcl.h>
#include <stdio.h>
#include <string>

static std::string parsetcl_listener = "parsetclcbk";
static struct {
  std::string listener;
  bool        raw;
} parsetclcfg = {"parsetclcbk", false};

const char* getTclTokenName(int type){
  switch(type){
    case TCL_TOKEN_WORD : return "TCL_TOKEN_WORD";
    case TCL_TOKEN_SIMPLE_WORD : return "TCL_TOKEN_SIMPLE_WORD";
    case TCL_TOKEN_TEXT: return "TCL_TOKEN_TEXT"; 
    case TCL_TOKEN_BS: return "TCL_TOKEN_BS";
    case TCL_TOKEN_COMMAND: return "TCL_TOKEN_COMMAND";
    case TCL_TOKEN_VARIABLE: return "TCL_TOKEN_VARIABLE";
    default:
      return "TCL_TOKEN_OTHER";
  }
}

int Tcl_CmdObjTraceProc_impl(
        ClientData clientData,
        Tcl_Interp* interp,
        int level,
        const char *command,
        Tcl_Command commandToken,
        int objc,
        Tcl_Obj *const objv[]){

  // printf("#trace# %s\n", command);
  printf("#TRACE-CMD# %d %s\n", level, Tcl_GetString(objv[0]));
  /*
  Tcl_CmdInfo cmdInfo;
  Tcl_GetCommandInfo(interp, "pvtrace", &cmdInfo);
  Tcl_SetCommandInfo(interp, "pvtrace", &cmdInfo);
   */
  return TCL_OK;
}




int parse_tcl_command(Tcl_Interp *interp, const char *start, int numBytes=-1) {
  //const char *start = "set abc [expr 3+4]";
  //int numBytes = -1;
  int nested = 0;
  int append = 0;
  Tcl_Parse parsePtr;
  int status = Tcl_ParseCommand(interp, start, numBytes, nested, &parsePtr);
  if(status==TCL_OK){
    if(parsePtr.commentSize>0){
      //TORM: printf("%.*s",parsePtr.commentSize, parsePtr.commentStart);
    }

    Tcl_Obj *objv = Tcl_NewListObj(0, NULL);
    Tcl_Obj *command = Tcl_NewStringObj(parsetcl_listener.c_str(),-1);
    Tcl_ListObjAppendElement(interp, objv, command);
    // Tcl_DecrRefCount(obj);
    // Tcl_SetListObj(objv, 0, NULL);

    //-- if(parsePtr.commentSize>0){
    //--   int size = parsePtr.commentStart + parsePtr.commentSize - start;
    //--   //Tcl_Obj *comment = Tcl_NewStringObj(parsePtr.commentStart, parsePtr.commentSize);
    //--   Tcl_Obj *comment = Tcl_NewStringObj(start, size);
    //--   Tcl_ListObjAppendElement(interp, objv, comment);
    //-- }

    // comment include preceding white space
    Tcl_Obj *comment = Tcl_NewStringObj(start, parsePtr.commandStart - start);
    Tcl_ListObjAppendElement(interp, objv, comment);
    if(parsePtr.commandSize==0){
      //printf("comment size: %d\n", parsePtr.commandStart - start);
    }


    if (parsetclcfg.raw) {
      //if(parsePtr.commandSize>0){
        Tcl_Obj *cmdtext = Tcl_NewStringObj(parsePtr.commandStart, parsePtr.commandSize);
        Tcl_ListObjAppendElement(interp, objv, cmdtext);
      //} else {
        // printf("comment only: %d %d %d\n", parsePtr.commandStart, parsePtr.commandSize, parsePtr.numTokens);
      //}
    }


    bool is_simple = true;
    bool has_cmd = false, has_var = false;
    for(int i=0, n=parsePtr.numTokens;i<n;i++){
      const Tcl_Token *tok = parsePtr.tokenPtr+i;
      if(tok->type == TCL_TOKEN_SIMPLE_WORD){
      }else if(tok->type == TCL_TOKEN_WORD){
        is_simple = false;
      //}else if(tok->type == TCL_TOKEN_EXPAND_WORD){
      //  is_simple = false;
      }

      for(int j=0, jn=tok->numComponents; j<jn; j++){
        const Tcl_Token *t = parsePtr.tokenPtr+(i+1+j);
        //printf("%s\n", getTclTokenName(t->type));
        if(t->type == TCL_TOKEN_COMMAND){
          has_cmd = true;
          //TODO: parse_tcl_command(interp, t->start+1, t->size-2);
        }else if(t->type == TCL_TOKEN_VARIABLE){
          has_var = true;
        }
      }

      i += tok->numComponents;
      Tcl_Obj *part = Tcl_NewStringObj(tok->start, tok->size);
      Tcl_ListObjAppendElement(interp, objv, part);
      // TODO: add space between token
      if (parsetclcfg.raw) {
        const char *p = tok->start + tok->size;
        Tcl_Obj *sp;
        if(i<n-1){
          int size = (parsePtr.tokenPtr+i+1)->start - p;
          sp = Tcl_NewStringObj(p, size);
        } else {
          // sp = Tcl_NewStringObj(p, numBytes<0?-1:(start+numBytes - p));
          sp = Tcl_NewStringObj(p, (parsePtr.commandStart + parsePtr.commandSize - p));
        }
        Tcl_ListObjAppendElement(interp, objv, sp);
      }
      //printf("tok %.*s %s %d %d %d\n",tok->size, tok->start, getTclTokenName(tok->type), tok->type, tok->size, tok->numComponents);
    }
    int        objv_c;
    Tcl_Obj  **objv_v;
    Tcl_ListObjGetElements(interp, objv, &objv_c, &objv_v);

    int ret = Tcl_EvalObjv(interp, objv_c, objv_v, 0);
    if(ret != TCL_OK){
      Tcl_Obj *options = Tcl_GetReturnOptions(interp, ret);
      Tcl_Obj *key = Tcl_NewStringObj("-errorinfo", -1);
      Tcl_Obj *stackTrace;
      Tcl_IncrRefCount(key);
      Tcl_DictObjGet(NULL, options, key, &stackTrace);
      Tcl_DecrRefCount(key);
    /* Do something with stackTrace */
      fprintf(stderr, "Tcl Error %s\n", Tcl_GetString(stackTrace));
      Tcl_DecrRefCount(options);
      return -1;
      // TODO: return TCL_OK;
    }

    //Tcl_DecrRefCount(command);
    Tcl_DecrRefCount(objv);
    //Tcl_DecrRefCount(comment);

    if(parsePtr.commandSize>0){
      Tcl_Token *tok = parsePtr.tokenPtr;
      if(is_simple){
        //TODO: printf("// CMD: %.*s\n", parsePtr.commandSize, parsePtr.commandStart);
      }else{
        //TODO: printf("//*CMD: %d %d %d %.*s\n",parsePtr.commandSize, parsePtr.numWords, parsePtr.numTokens, parsePtr.commandSize, parsePtr.commandStart);
      }
      //TODO: printf("CMD: %.*s\n", tok->size, tok->start);
    }

    int size = (parsePtr.commandStart - start) + parsePtr.commandSize;
    Tcl_FreeParse(&parsePtr);
    return size;
  } else {
    Tcl_FreeParse(&parsePtr);
    return -1;
  }
}

int parse_tcl_text(Tcl_Interp *interp, const char *buf, int length){
  char *start = (char *) buf;
  char *stop = start+length;
  while(start < stop){
    int n = parse_tcl_command(interp, start, stop-start);
    if(n>-1) {
      start = start + n;
    }else{
      fprintf(stderr, "Error: parse error ...\n");
      break;
    }
  }
  return 0;
}

int parse_tcl_file(Tcl_Interp *interp, const char *file){
  FILE *fp = NULL;

  if(file==NULL){
    fp = stdin;
  }else if(!strcmp(file,"-")){
    fp = stdin;
  }else{
    fp = fopen(file, "r");
  }

  char buf[10240];
  std::string s="";
  while(NULL!=fgets(buf, 10240, fp)){
    // TODO: long line
    s.append(buf);
    if(Tcl_CommandComplete(s.c_str())){
      parse_tcl_text(interp, s.c_str(), s.size());
      s="";
    }
  }

  /*
  size_t n = fread(buf,1,4096,fp);
  if(n>0){
    parse_tcl_text(interp, buf, n);
  }
  */
  fclose(fp);
}

int parsetcl_ObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
  const char *subcmd = Tcl_GetString(objv[1]);
  // -raw
  if(strcmp(subcmd,"-file")==0){
    const char *file = Tcl_GetString(objv[2]);
    parse_tcl_file(interp, file);
  }else if(strcmp(subcmd,"-code")==0){
    int len = -1;
    const char *code = Tcl_GetStringFromObj(objv[2], &len);
    if (code[0]=='{' || code[0]=='"' || code[0]=='[') {
      // parse_tcl_command(interp, code+1, len-2);
      parse_tcl_text(interp, code+1, len-2);
    }else{
      //parse_tcl_command(interp, code);
      parse_tcl_text(interp, code, len);
    }
  }else if(strcmp(subcmd,"-cmd")==0){
    int len = -1;
    const char *code = Tcl_GetStringFromObj(objv[2], &len);
    parse_tcl_command(interp, code);
  }else if(strcmp(subcmd,"-listener")==0){
    parsetcl_listener = Tcl_GetString(objv[2]);
  }else if(strcmp(subcmd,"-raw")==0){
    parsetclcfg.raw = true;
  } else {
    printf("Usage: -file | -code | -listener | -raw\n");
  }
  return TCL_OK;
}

extern "C" {

int Parsetcl_Init(Tcl_Interp *interp) {

#ifdef USE_TCL_STUBS
 if(Tcl_InitStubs(interp, "8.4", 0) == NULL) {
   fprintf(stderr, "Error: Tcl_InitStubs\n");
   return TCL_ERROR;
 }
#else
 if(Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == NULL) {
   fprintf(stderr, "Error: package require Tcl 8.4\n");
   return TCL_ERROR;
 }
#endif

 Tcl_CreateObjCommand(interp, "parsetcl", parsetcl_ObjCmd, NULL, NULL);
 //printf("parsetcl loaded!\n");
 return TCL_OK;
}

}


int appInitProc(Tcl_Interp *interp){

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

  /*
  char buf[255];
  const char *name = Tcl_GetNameOfExecutable();
  sprintf(buf, "%s.tcl", name);
  Tcl_EvalFile(interp, buf); // TODO
  */

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

#ifdef __MAIN__
int main(int argc, char **argv){
  Tcl_Main(argc, argv, appInitProc);
  return 0;
}
#endif



