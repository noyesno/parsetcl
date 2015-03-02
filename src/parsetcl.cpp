
#include <tcl.h>
#include <stdio.h>
#include <string>

static struct {
  std::string listener;
  std::string subst;
  bool        raw;
  int         debug;
  int         hint;
} parsetclcfg = {"parsetcl::command", "parsetcl::bracket", false, 0, 0};

const char* getTclTokenName(int type){
  switch(type){
    case TCL_TOKEN_WORD : return "TCL_TOKEN_WORD";
    case TCL_TOKEN_SIMPLE_WORD : return "TCL_TOKEN_SIMPLE_WORD";
    case TCL_TOKEN_EXPAND_WORD : return "TCL_TOKEN_EXPAND_WORD";
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




int parse_tcl_command(Tcl_Interp *interp, const char *start, int numBytes=-1, const char *action=NULL) {
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
    Tcl_Obj *command;

    if(action==NULL){
      command = Tcl_NewStringObj(parsetclcfg.listener.c_str(),-1);
    }else{
      command = Tcl_NewStringObj(action,-1);
    }
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
    Tcl_Obj *cmdtext = Tcl_NewStringObj(parsePtr.commandStart, parsePtr.commandSize);
    Tcl_ListObjAppendElement(interp, objv, comment);
    Tcl_ListObjAppendElement(interp, objv, cmdtext);

    if(parsePtr.commandSize==0){
      //printf("comment size: %d\n", parsePtr.commandStart - start);
    }

    bool is_simple = true;
    bool has_cmd = false, has_var = false, has_expand = false, has_bs=false, has_txt=false;
    int  n_cmd = 0, n_var = 0, n_bs=0, n_txt=0;
    for(int i=0, n=parsePtr.numTokens; i<n; i++){
      const Tcl_Token *tok = parsePtr.tokenPtr+i;
      if(parsetclcfg.debug){
        printf("TOKEN: %s %.*s\n", getTclTokenName(tok->type), tok->size, tok->start);
      }
      switch(tok->type){
        case TCL_TOKEN_SIMPLE_WORD :
          // ...
          break;
        case TCL_TOKEN_WORD :
          is_simple = false;
          break;
        case TCL_TOKEN_EXPAND_WORD :
          has_expand = true;
          break;
        default:
          fprintf(stderr, "Error: unexpected token type %d for command\n", tok->type);
          return TCL_ERROR;
      }

      // TODO: parse the info to Tcl


      Tcl_Obj *part = Tcl_NewStringObj("", -1);

      if(has_expand){
        Tcl_AppendToObj(part, "{*}", -1);
      }

      for(int j=0, jn=tok->numComponents; j<jn; j++){
        const Tcl_Token *t = parsePtr.tokenPtr+(i+1+j);
        if(parsetclcfg.debug){
          printf("  tok: %s %.*s\n", getTclTokenName(t->type), t->size, t->start);
        }

        switch(t->type){
          case TCL_TOKEN_COMMAND :
            has_cmd = true; n_cmd++;
            /* recursive parse sub command */
            // TODO: [ a ... ] is actually a code block
            parse_tcl_command(interp, t->start+1, t->size-2, parsetclcfg.subst.c_str());
            Tcl_AppendObjToObj(part, Tcl_GetObjResult(interp));
            break;
          case TCL_TOKEN_VARIABLE :
            has_var = true; n_var++;
            j += t->numComponents; /* skip sub tokens of variable */
            Tcl_AppendToObj(part, t->start, t->size);
            break;
          case TCL_TOKEN_BS:
            has_bs  = true; n_bs++;
            break;
            Tcl_AppendToObj(part, t->start, t->size);
          case TCL_TOKEN_TEXT:
            has_txt  = true; n_txt++;
            Tcl_AppendToObj(part, t->start, t->size);
            break;
          default:
            Tcl_AppendToObj(part, t->start, t->size);
            break;
        }
      }


      //-- const Tcl_Token *subtok = parsePtr.tokenPtr+i+1;
      //-- if(tok->numComponents==1 && subtok->type == TCL_TOKEN_COMMAND){
      //--     std::string s(subtok->start,subtok->size);
      //--     // printf("DEBUG: %d %s\n", tok->numComponents, s.c_str());

      //--     parse_tcl_command(interp, subtok->start+1, subtok->size-2, parsetclcfg.subst.c_str());
      //--     if(has_expand){
      //--       part = Tcl_NewStringObj("{*}", -1);
      //--       Tcl_AppendObjToObj(part, Tcl_GetObjResult(interp));
      //--     }else{
      //--       part = Tcl_GetObjResult(interp);
      //--     }
      //--     // part = Tcl_ObjPrintf("[%s]", Tcl_GetStringResult(interp));
      //-- } else {
      //--     part = Tcl_NewStringObj(tok->start, tok->size);
      //-- }

      Tcl_ListObjAppendElement(interp, objv, part);

      // TODO: add space between token
      i += tok->numComponents;
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

    if(parsetclcfg.hint){
      Tcl_Obj *hint = Tcl_NewStringObj("::parsetcl::hint", -1);
      Tcl_ObjSetVar2(interp, hint, Tcl_NewStringObj("n_cmd", -1), Tcl_NewIntObj(n_cmd), TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
      Tcl_ObjSetVar2(interp, hint, Tcl_NewStringObj("n_var", -1), Tcl_NewIntObj(n_var), TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
      Tcl_ObjSetVar2(interp, hint, Tcl_NewStringObj("n_txt", -1), Tcl_NewIntObj(n_txt), TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
      Tcl_ObjSetVar2(interp, hint, Tcl_NewStringObj("n_bs", -1),  Tcl_NewIntObj(n_bs),  TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    }

    int ret = Tcl_EvalObjv(interp, objv_c, objv_v, 0);
    if(ret != TCL_OK){
      Tcl_Obj *options = Tcl_GetReturnOptions(interp, ret);
      Tcl_Obj *key = Tcl_NewStringObj("-errorinfo", -1);
      Tcl_Obj *stackTrace;
      Tcl_IncrRefCount(key);
      Tcl_DictObjGet(NULL, options, key, &stackTrace);
      Tcl_DecrRefCount(key);
    /* Do something with stackTrace */
      fprintf(stderr, "Error: Tcl Error %s\n", Tcl_GetString(stackTrace));
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
    fprintf(stderr, "Error: Tcl_ParseCommand: %s\n", Tcl_GetStringResult(interp));
    fprintf(stderr, "----\n");
    fprintf(stderr, "%.*s\n", 81, start); //TODO: the length
    fprintf(stderr, "----\n");
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
      // fprintf(stderr, "Error: parse_tcl_text: %s\n", Tcl_GetStringResult(interp));
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

  if(fp==NULL){
    fprintf(stderr, "Error: fail to open file '%s' for read.\n", file);
    return TCL_ERROR;
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

  return TCL_OK;
}

/*
 * parsetcl config -raw
 * parsetcl config -listener proc_name
 *
 * parsetcl -file $file
 * parsetcl -code $code
 */
int parsetcl_config_ObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
  for(int i=1; i<objc; i++){
    const char *subcmd = Tcl_GetString(objv[i]);
    Tcl_Obj *argval;
    if(0==strcmp(subcmd,"-raw")){
      parsetclcfg.raw = true;
    }else if(0==strcmp(subcmd,"-command")){
      argval = objv[++i];
      parsetclcfg.listener = Tcl_GetString(argval);
      Tcl_SetVar2Ex(interp, "::parsetcl::config", "-command", argval, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    }else if(0==strcmp(subcmd,"-bracket")){
      argval = objv[++i];
      parsetclcfg.subst = Tcl_GetString(argval);
      Tcl_SetVar2Ex(interp, "::parsetcl::config", "-bracket", argval, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    }else if(0==strcmp(subcmd,"-debug")){
      argval = objv[++i];
      Tcl_GetIntFromObj(interp, argval, &parsetclcfg.debug);
      Tcl_SetVar2Ex(interp, "::parsetcl::config", "-debug", argval, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    }else if(0==strcmp(subcmd,"-hint")){
      argval = objv[++i];
      Tcl_GetIntFromObj(interp, argval, &parsetclcfg.hint);
      Tcl_SetVar2Ex(interp, "::parsetcl::config", "-hint", argval, TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    }
  }
  return TCL_OK;
}

int parsetcl_parse_ObjCmd(ClientData clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]) {
  const char *subcmd = Tcl_GetString(objv[1]);

  // TODO: handle return error
  if(strcmp(subcmd,"-file")==0){
    const char *file = Tcl_GetString(objv[2]);
    Tcl_SetVar2(interp, "::parsetcl::config", "-input", "file", TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    return parse_tcl_file(interp, file);
  }else if(strcmp(subcmd,"-code")==0){
    int len = -1;
    const char *code = Tcl_GetStringFromObj(objv[2], &len);
    Tcl_SetVar2(interp, "::parsetcl::config", "-input", "code", TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    if (code[0]=='{' || code[0]=='"' || code[0]=='[') {
      parse_tcl_text(interp, code+1, len-2);
    }else{
      parse_tcl_text(interp, code, len);
    }
  }else if(strcmp(subcmd,"-cmd")==0){
    int len = -1;
    // TODO: use -block instead
    Tcl_SetVar2(interp, "::parsetcl::config", "-input", "cmd", TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG);
    const char *code = Tcl_GetStringFromObj(objv[2], &len);
    parse_tcl_command(interp, code);
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
#endif

 if(Tcl_PkgRequire(interp, "Tcl", "8.4", 0) == NULL) {
   fprintf(stderr, "Error: package require Tcl 8.4\n");
   return TCL_ERROR;
 }

 Tcl_CreateNamespace(interp, "parsetcl", NULL, NULL);
 Tcl_CreateObjCommand(interp, "parsetcl",         parsetcl_parse_ObjCmd, NULL, NULL);
 Tcl_CreateObjCommand(interp, "parsetcl::parse",  parsetcl_parse_ObjCmd, NULL, NULL);
 Tcl_CreateObjCommand(interp, "parsetcl::config", parsetcl_config_ObjCmd, NULL, NULL);
 //printf("parsetcl loaded!\n");
 return TCL_OK;
}

}


