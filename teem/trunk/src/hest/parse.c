/*
  The contents of this file are subject to the University of Utah Public
  License (the "License"); you may not use this file except in
  compliance with the License.
  
  Software distributed under the License is distributed on an "AS IS"
  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
  the License for the specific language governing rights and limitations
  under the License.

  The Original Source Code is "teem", released March 23, 2001.
  
  The Original Source Code was developed by the University of Utah.
  Portions created by UNIVERSITY are Copyright (C) 2001, 1998 University
  of Utah. All Rights Reserved.
*/

/*
learned: well duh: when you send arguments to printf(), they will
be evaluated before printf() sees them, so you can't use _hestIdent()
twice with differen values
*/

#include "hest.h"
#include "private.h"

#define ME ((parm && parm->verbosity) ? me : "")

/*
** _hestArgsInResponseFiles()
**
** returns the number of args that will be parsed from the response files.
** The role of this function is solely to simplify the task of avoiding
** memory leaks.  By knowing exactly how many args we'll get in the response
** file, then hestParse() can allocate its local argv[] for exactly as
** long as it needs to be, and we can avoid using an airArray.  The drawback
** is that we open and read through the response files twice.  Alas.
*/
int
_hestArgsInResponseFiles(int *argcP, int *nrfP,
			 char **argv, char *err, hestParm *parm) {
  FILE *file;
  char me[]="_hestArgsInResponseFiles: ", line[AIR_STRLEN_HUGE], *pound;
  int ai, len;

  *argcP = 0;
  *nrfP = 0;
  if (!parm->respFileEnable) {
    /* don't do response files; we're done */
    return 0;
  }

  ai = 0;
  while (argv[ai]) {
    if (parm->respFileFlag == argv[ai][0]) {
      if (!(file = fopen(argv[ai]+1, "r"))) {
	/* can't open the indicated response file for reading */
	sprintf(err, "%scouldn't open \"%s\" for reading as response file",
		ME, argv[ai]+1);
	*argcP = 0;
	*nrfP = 0;
	return 1;
      }
      len = airOneLine(file, line, AIR_STRLEN_HUGE);
      while (len > 0) {
	if (pound = strchr(line, parm->respFileComment))
	  *pound = '\0';
	airOneLinify(line);
	*argcP += airStrntok(line, AIR_WHITESPACE);
	len = airOneLine(file, line, AIR_STRLEN_HUGE);
      }
      fclose(file);
      (*nrfP)++;
    }
    ai++;
  }
  return 0;
}

/*
** _hestResponseFiles()
**
** This function is badly named.  Even if there are no response files,
** even if response files are disabled, this is the function that
** copies from the user's argc,argv to our local copy.
*/
int
_hestResponseFiles(char **newArgv, char **oldArgv, int nrf,
		   char *err, hestParm *parm, airArray *pmop) {
  char line[AIR_STRLEN_HUGE], *pound;
  int len, newArgc, oldArgc, incr, ai;
  FILE *file;
  
  newArgc = oldArgc = 0;
  while(oldArgv[oldArgc]) {
    if (parm->verbosity) {
      printf("!%s:________ newArgc = %d, oldArgc = %d\n", 
	     "dammit", newArgc, oldArgc);
      _hestPrintArgv(newArgc, newArgv);
    }
    if (!parm->respFileEnable
	|| parm->respFileFlag != oldArgv[oldArgc][0]) {
      /* nothing to do with a response file, just copy the arg over.
	 We are not allocating new memory in this case. */
      newArgv[newArgc] = oldArgv[oldArgc];
      newArgc += 1;
    }
    else {
      /* It is a response file.  Error checking on being able to open it
	 should have been done by _hestArgsInResponseFiles() */
      file = fopen(oldArgv[oldArgc]+1, "r");
      len = airOneLine(file, line, AIR_STRLEN_HUGE);
      while (len > 0) {
	if (parm->verbosity)
	  printf("_hestResponseFiles: line: |%s|\n", line);
	if (pound = strchr(line, parm->respFileComment))
	  *pound = '\0';
	if (parm->verbosity)
	  printf("_hestResponseFiles: -0-> line: |%s|\n", line);
	airOneLinify(line);
	incr = airStrntok(line, AIR_WHITESPACE);
	if (parm->verbosity)
	  printf("_hestResponseFiles: -1-> line: |%s|, incr=%d\n",
		 line, incr);
	airParseStrS(newArgv + newArgc, line, AIR_WHITESPACE, incr);
	for (ai=0; ai<=incr-1; ai++) {
	  /* This time, we did allocate memory.  We can use airFree and
	     not airFreeP because these will not be reset before mopping */
	  airMopAdd(pmop, newArgv[newArgc+ai], airFree, airMopAlways);
	}
	len = airOneLine(file, line, AIR_STRLEN_HUGE);
	newArgc += incr;
      }
      fclose(file);
    }
    oldArgc++;
    if (parm->verbosity) {
      _hestPrintArgv(newArgc, newArgv);
      printf("!%s: ^^^^^^^ newArgc = %d, oldArgc = %d\n", 
	     "dammit", newArgc, oldArgc);
    }
  }
  newArgv[newArgc] = NULL;

  return 0;
}

/*
** _hestPanic()
**
** all error checking on the given hest array itself (not the
** command line to be parsed).  Also, sets the "kind" field of
** the opt struct
*/
int
_hestPanic(hestOpt *opt, char *err, hestParm *parm) {
  char me[]="_hestPanic: ", tbuff[AIR_STRLEN_HUGE], *sep;
  int numvar, op, numOpts;

  numOpts = _hestNumOpts(opt);
  numvar = 0;
  for (op=0; op<=numOpts-1; op++) {
    opt[op].kind = _hestKind(opt + op);
    if (!(AIR_BETWEEN(airTypeUnknown, opt[op].type, airTypeLast))) {
      if (err)
	sprintf(err, "%s!!!!!! opt[%d].type (%d) not in valid range [%d,%d]",
		ME, op, opt[op].type, airTypeUnknown+1, airTypeLast-1);
      return 1;
    }
    if (-1 == opt[op].kind) {
      if (err)
	sprintf(err, "%s!!!!!! opt[%d]'s min (%d) and max (%d) incompatible",
		ME, op, opt[op].min, opt[op].max);
      return 1;
    }
    if (5 == opt[op].kind && !(opt[op].sawP)) {
      if (err)
	sprintf(err, "%s!!!!!! have multiple variable parameters, "
		"but sawP is NULL", ME);
      return 1;
    }
    if (airTypeOther == opt[op].type) {
      if (!(opt[op].CB)) {
	if (err)
	  sprintf(err, "%s!!!!!! opt[%d] is type \"other\", but no "
		  "callbacks given", ME, op);
	return 1;
      }
      if (!( opt[op].CB->size > 0 )) {
	if (err)
	  sprintf(err, "%s!!!!!! opt[%d]'s \"size\" (%d) invalid", 
		  ME, op, opt[op].CB->size);
	return 1;
      }
      if (!( opt[op].type )) {
	if (err)
	  sprintf(err, "%s!!!!!! opt[%d]'s \"type\" is NULL",
		  ME, op);
	return 1;

      }
      if (!( opt[op].CB->parse )) {
	if (err)
	  sprintf(err, "%s!!!!!! opt[%d]'s \"parse\" callback NULL", ME, op);
	return 1;
      }
      if (opt[op].CB->delete && (sizeof(void*) != opt[op].CB->size)) {
	if (err)
	  sprintf(err, "%s!!!!!! opt[%d] has a \"delete\", but size isn't "
		  "sizeof(void*)", ME, op);
	return 1;
      }
    }
    if (opt[op].flag) {
      strcpy(tbuff, opt[op].flag);
      if (( sep = strchr(tbuff, parm->multiFlagSep) )) {
	*sep = '\0';
	if (!( strlen(tbuff) && strlen(sep+1) )) {
	  if (err)
	    sprintf(err, "%s!!!!!! either short (\"%s\") or long (\"%s\") flag"
		    " of opt[%d] is zero length", ME, tbuff, sep+1, op);
	  return 1;
	}
      }
      else {
	if (!strlen(opt[op].flag)) {
	  if (err)
	    sprintf(err, "%s!!!!!! opt[%d].flag is zero length",
		    ME, op);
	  return 1;
	}
      }
      if (4 == opt[op].kind) {
	if (!opt[op].dflt) {
	  sprintf(err, "%s!!!!!! flagged single variable parameter must "
		  "specify a default", ME);
	  return 1;
	}
	if (!strlen(opt[op].dflt)) {
	  sprintf(err, "%s!!!!!! flagged single variable parameter default "
		  "must be non-zero length", ME);
	  return 1;
	}
      }
      /*
      sprintf(tbuff, "-%s", opt[op].flag);
      if (1 == sscanf(tbuff, "%f", &tmpF)) {
	if (err)
	  sprintf(err, "%s!!!!!! opt[%d].flag (\"%s\") is numeric, bad news",
		  ME, op, opt[op].flag);
	return 1;
      }
      */
    }
    if (1 == opt[op].kind) {
      if (!opt[op].flag) {
	if (err)
	  sprintf(err, "%s!!!!!! flags must have flags", ME);
	return 1;
      }
    }
    else {
      if (!opt[op].name) {
	if (err)
	  sprintf(err, "%s!!!!!! opt[%d] isn't a flag: must have \"name\"",
		  ME, op);
	return 1;
      }
    }
    if (4 == opt[op].kind && !opt[op].dflt) {
      if (err)
	sprintf(err, "%s!!!!!! opt[%d] is single variable parameter, but "
		"no default set", ME, op);
      return 1;
    }
    numvar += (opt[op].min < _hestMax(opt[op].max) && (NULL == opt[op].flag));
  }
  if (numvar > 1) {
    if (err)
      sprintf(err, "%s!!!!!! can't have %d unflagged min<max opts, only one", 
	      ME, numvar);
    return 1;
  }
  return 0;
}

int
_hestErrStrlen(hestOpt *opt, int argc, char **argv) {
  int a, numOpts, ret, other;

  ret = 0;
  numOpts = _hestNumOpts(opt);
  other = AIR_FALSE;
  if (argv) {
    for (a=0; a<=argc-1; a++) {
      ret = AIR_MAX(ret, airStrlen(argv[a]));
    }
  }
  for (a=0; a<=numOpts-1; a++) {
    ret = AIR_MAX(ret, airStrlen(opt[a].flag));
    ret = AIR_MAX(ret, airStrlen(opt[a].name));
    other |= opt[a].type == airTypeOther;
  }
  for (a=airTypeUnknown+1; a<=airTypeLast-1; a++) {
    ret = AIR_MAX(ret, airStrlen(airTypeStr[a]));
  }
  if (other) {
    /* the callback's error() function may sprintf an error message
       into a buffer which is size AIR_STRLEN_HUGE */
    ret += AIR_STRLEN_HUGE;
  }
  ret += 4 * 12;  /* as many as 4 ints per error message */
  ret += 257;     /* function name and text of hest's error message */

  return ret;
}

/*
** _hestExtractFlagged()
**
** extracts the parameters associated with all flagged options from the
** given argc and argv, storing them in prms[], recording the number
** of parameters in nprm[], and whether or not the flagged option appeared
** in appr[].
**
** The "saw" information is not set here, since it is better set
** at value parsing time, which happens after defaults are enstated.
*/
int
_hestExtractFlagged(char **prms, int *nprm, int *appr,
		     int *argcP, char **argv, 
		     hestOpt *opt,
		     char *err, hestParm *parm, airArray *pmop) {
  char me[]="_hestExtractFlagged: ", ident1[AIR_STRLEN_HUGE],
    ident2[AIR_STRLEN_HUGE];
  int a, np, flag, endflag, numOpts, op;

  a = 0;
  if (parm->verbosity) 
    printf("!%s: *argcP = %d\n", me, *argcP);
  while (a<=*argcP-1) {
    if (parm->verbosity) 
      printf("!%s: a = %d -> argv[a] = %s\n", me, a, argv[a]);
    flag = _hestWhichFlag(opt, argv[a], parm);
    if (parm->verbosity) 
      printf("!%s: A: a = %d -> flag = %d\n", me, a, flag);
    if (!(0 <= flag)) {
      /* not a flag, move on */
      a++;
      continue;
    }
    /* see if we can associate some parameters with the flag */
    np = 0;
    endflag = 0;
    while (np < _hestMax(opt[flag].max) &&
	   a+np+1 <= *argcP-1 &&
	   -1 == (endflag = _hestWhichFlag(opt, argv[a+np+1], parm))) {
      np++;
      if (parm->verbosity)
	printf("!%s: np --> %d with endflag = %d\n", me, np, endflag);
    }
    /* we stopped because we got the max number of parameters, or
       because we hit the end of the command line, or
       because _hestWhichFlag() returned something other than -1,
       which means it returned -2, or a valid option index.  If
       we stopped because of _hestWhichFlag()'s return value, 
       endflag has been set to that return value */
    if (parm->verbosity)
      printf("!%s: B: np = %d; endflag = %d\n", me, np, endflag); 
    if (np < opt[flag].min) {
      /* didn't get minimum number of parameters */
      if (!( a+np+1 <= *argcP-1 )) {
	sprintf(err, "%shit end of line before getting %d parameter%s "
		"for %s (got %d)",
		ME, opt[flag].min, opt[flag].min > 1 ? "s" : "",
		_hestIdent(ident1, opt+flag, parm), np);
      }
      else {
	sprintf(err, "%shit %s before getting %d parameter%s for %s (got %d)",
		ME, _hestIdent(ident1, opt+endflag, parm),
		opt[flag].min, opt[flag].min > 1 ? "s" : "",
		_hestIdent(ident2, opt+flag, parm), np);
      }
      return 1;
    }
    nprm[flag] = np;
    if (parm->verbosity) {
      printf("!%s:________ a=%d, *argcP = %d -> flag = %d\n", 
	     me, a, *argcP, flag);
      _hestPrintArgv(*argcP, argv);
    }
    /* lose the flag argument */
    free(_hestExtract(argcP, argv, a, 1));
    /* extract the args after the flag */
    if (appr[flag]) {
      airMopSub(pmop, prms[flag], airFree);
      airFree(prms[flag]);
    }
    prms[flag] = _hestExtract(argcP, argv, a, nprm[flag]);
    airMopAdd(pmop, prms[flag], airFree, airMopAlways);
    appr[flag] = AIR_TRUE;
    if (-2 == endflag) {
      /* we should lose the end-of-variable-parameter marker */
      free(_hestExtract(argcP, argv, a, 1));
    }
    if (parm->verbosity) {
      _hestPrintArgv(*argcP, argv);
      printf("!%s:^^^^^^^^ *argcP = %d\n", me, *argcP);
      printf("!%s: prms[%d] = %s\n", me, flag,
	     prms[flag] ? prms[flag] : "(null)");
    }
  }

  /* make sure that flagged options without default were given */
  numOpts = _hestNumOpts(opt);
  for (op=0; op<=numOpts-1; op++) {
    if (opt[op].flag && !opt[op].dflt && !appr[op]) {
      sprintf(err, "%sdidn't get required %s",
	      ME, _hestIdent(ident1, opt+op, parm));
      return 1;
    }
  }

  return 0;
}

int
_hestNextUnflagged(int op, hestOpt *opt, int numOpts) {

  for(; op<=numOpts-1; op++) {
    if (!opt[op].flag)
      break;
  }
  return op;
}

int
_hestExtractUnflagged(char **prms, int *nprm,
		      int *argcP, char **argv, 
		      hestOpt *opt,
		      char *err, hestParm *parm, airArray *pmop) {
  char me[]="_hestExtractUnflagged: ", ident[AIR_STRLEN_HUGE];
  int nvp, np, op, unflag1st, unflagVar, numOpts;

  numOpts = _hestNumOpts(opt);
  unflag1st = _hestNextUnflagged(0, opt, numOpts);
  if (numOpts == unflag1st) {
    /* no unflagged options; we're done */
    return 0;
  }

  for (unflagVar = unflag1st; 
       unflagVar != numOpts; 
       unflagVar = _hestNextUnflagged(unflagVar+1, opt, numOpts)) {
    if (opt[unflagVar].min < _hestMax(opt[unflagVar].max))
      break;
  }
  /* now, if there is a variable parameter unflagged opt, unflagVar is its
     index in opt[], or else unflagVar is numOpts */

  /* grab parameters for all unflagged opts before opt[t] */
  for (op = _hestNextUnflagged(0, opt, numOpts); 
       op < unflagVar; 
       op = _hestNextUnflagged(op+1, opt, numOpts)) {
    /* printf("!%s: op = %d; unflagVar = %d\n", me, op, unflagVar); */
    np = opt[op].min;  /* min == max */
    if (!(np <= *argcP)) {
      sprintf(err, "%sdon't have %d parameter%s %s%s%sfor %s", 
	      ME, np, np > 1 ? "s" : "", 
	      argv[0] ? "starting at \"" : "",
	      argv[0] ? argv[0] : "",
	      argv[0] ? "\" " : "",
	      _hestIdent(ident, opt+op, parm));
      return 1;
    }
    prms[op] = _hestExtract(argcP, argv, 0, np);
    airMopAdd(pmop, prms[op], airFree, airMopAlways);
    nprm[op] = np;
  }
  /*
  _hestPrintArgv(*argcP, argv);
  */
  /* we skip over the variable parameter unflagged option, subtract from *argcP
     the number of parameters in all the opts which follow it, in order to get
     the number of parameters in the sole variable parameter option, 
     store this in nvp */
  nvp = *argcP;
  for (op = _hestNextUnflagged(unflagVar+1, opt, numOpts); 
       op < numOpts; 
       op = _hestNextUnflagged(op+1, opt, numOpts)) {
    nvp -= opt[op].min;  /* min == max */
  }
  if (nvp < 0) {
    op = _hestNextUnflagged(unflagVar+1, opt, numOpts);
    np = opt[op].min;
    sprintf(err, "%sdon't have %d parameter%s for %s", 
	    ME, np, np > 1 ? "s" : "", 
	    _hestIdent(ident, opt+op, parm));
    return 1;
  }
  /* else we had enough args for all the unflagged options following
     the sole variable parameter unflagged option, so snarf them up */
  for (op = _hestNextUnflagged(unflagVar+1, opt, numOpts); 
       op < numOpts; 
       op = _hestNextUnflagged(op+1, opt, numOpts)) {
    np = opt[op].min;
    prms[op] = _hestExtract(argcP, argv, nvp, np);
    airMopAdd(pmop, prms[op], airFree, airMopAlways);
    nprm[op] = np;
  }

  /* now we grab the parameters of the sole variable parameter unflagged opt,
     if it exists (unflagVar < numOpts) */
  if (unflagVar < numOpts) {
    /*
    printf("!%s: unflagVar=%d: min, nvp, max = %d %d %d\n", me, unflagVar,
	   opt[unflagVar].min, nvp, _hestMax(opt[unflagVar].max));
    */
    /* we'll do error checking for unexpected args later */
    nvp = AIR_MIN(nvp, _hestMax(opt[unflagVar].max));
    if (nvp < opt[unflagVar].min) {
      sprintf(err, "%sdidn't get minimum of %d arg%s for %s (got %d)",
	      ME, opt[unflagVar].min, 
	      opt[unflagVar].min > 1 ? "s" : "",
	      _hestIdent(ident, opt+unflagVar, parm), nvp);
      return 1;
    }
    if (nvp) {
      prms[unflagVar] = _hestExtract(argcP, argv, 0, nvp);
      airMopAdd(pmop, prms[unflagVar], airFree, airMopAlways);
      nprm[unflagVar] = nvp;
    }
    else {
      prms[unflagVar] = NULL;
      nprm[unflagVar] = 0;
    }
  }
  return 0;
}

int
_hestDefaults(char **prms, int *udflt, int *nprm, int *appr, 
	      hestOpt *opt,
	      char *err, hestParm *parm, airArray *mop) {
  char *tmpS, me[]="_hestDefaults: ", ident[AIR_STRLEN_HUGE];
  int op, numOpts;

  numOpts = _hestNumOpts(opt);
  for (op=0; op<=numOpts-1; op++) {
    if (parm->verbosity) 
      printf("%s op=%d/%d: \"%s\" --> kind=%d, nprm=%d, appr=%d\n",
	     me, op, numOpts-1, prms[op], opt[op].kind,
	     nprm[op], appr[op]);
    switch(opt[op].kind) {
    case 1:
      /* -------- (no-parameter) boolean flags -------- */
      /* default is always ignored */
      udflt[op] = 0;
      break;
    case 2:
    case 3:
      /* -------- one required parameter -------- */
      /* -------- multiple required parameters -------- */
      /* we'll used defaults if the flag didn't appear */
      udflt[op] = opt[op].flag && !appr[op];
      break;
    case 4:
      /* -------- optional single variables -------- */
      /* if the flag appeared (if there is a flag) but the paramter didn't,
	 we'll "invert" the default; if the flag didn't appear (or if there
	 isn't a flag) and the parameter also didn't appear, we'll use the
	 default.  In either case, nprm[op] will be zero, and in both cases,
	 we need to use the default information. */
      udflt[op] = (0 == nprm[op]);
      break;
    case 5:
      /* -------- multiple optional parameters -------- */
      /* we'll use the default if the flag didn't appear (if there is a
	 flag) Otherwise, if nprm[op] is zero, then the user is saying,
	 I want zero parameters */
      udflt[op] = opt[op].flag && !appr[op];
      break;
    }
    if (!udflt[op])
      continue;
    prms[op] = airStrdup(opt[op].dflt);
    if (prms[op]) {
      airMopAdd(mop, prms[op], airFree, airMopAlways);
      airOneLinify(prms[op]);
      tmpS = airStrdup(prms[op]);
      nprm[op] = airStrntok(tmpS, " ");
      tmpS = airFree(tmpS);
      /* printf("!%s: nprm[%d] in default = %d\n", me, op, nprm[op]); */
      if (opt[op].min < _hestMax(opt[op].max)) {
	if (!( AIR_INSIDE(opt[op].min, nprm[op], _hestMax(opt[op].max)) )) {
	  sprintf(err, "%s# parameters (in default) for %s is %d, "
		  "but need between %d and %d", 
		  ME, _hestIdent(ident, opt+op, parm), nprm[op],
		  opt[op].min, _hestMax(opt[op].max));
	  return 1;
	}
      }
    }
  }
  return 0;
}

int
_hestSetValues(char **prms, int *udflt, int *nprm, int *appr,
	       hestOpt *opt,
	       char *err, hestParm *parm, airArray *pmop) {
  char ident[AIR_STRLEN_HUGE], me[]="_hestSetValues: ",
    cberr[AIR_STRLEN_HUGE], *tok, *last, *prmsCopy;
  double tmpD;
  int op, type, numOpts, p, ret;
  void *vP;
  char *cP;
  size_t size;

  numOpts = _hestNumOpts(opt);
  for (op=0; op<=numOpts-1; op++) {
    _hestIdent(ident, opt+op, parm);
    type = opt[op].type;
    size = airTypeOther == type ? opt[op].CB->size : airTypeSize[type];
    cP = vP = opt[op].valueP;
    if (parm->verbosity) {
      printf("%s %d of %d: \"%s\": |%s| --> kind=%d, type=%d, size=%d\n", 
	     me, op, numOpts-1, prms[op], ident, opt[op].kind, type, size);
    }
    /* we may over-write these */
    opt[op].alloc = 0;
    if (opt[op].sawP) *(opt[op].sawP) = 0;
    switch(opt[op].kind) {
    case 1:
      /* -------- parameter-less boolean flags -------- */
      if (vP)
	*((int*)vP) = appr[op];
      break;
    case 2:
      /* -------- one required parameter -------- */
      if (prms[op] && vP) {
	if (airTypeOther != type) {
	  if (1 != airParseStr[type](vP, prms[op], " ", 1)) {
	    sprintf(err, "%scouldn't parse %s\"%s\" as %s for %s", 
		    ME, udflt[op] ? "(default) " : "", prms[op],
		    airTypeStr[type], ident);
	    return 1;
	  }
	  if (airTypeString == type) {
	    /* vP is the address of a char* (a char **), but what we
	       manage with airMop is the char * */
	    opt[op].alloc = 1;
	    airMopMem(pmop, vP, airMopOnError);
	  }
	}
	else {
	  /* we are parsing an "other" (exactly one of them) */
	  strcpy(cberr, "");
	  ret = opt[op].CB->parse(vP, prms[op], cberr);
	  if (ret) {
	    if (strlen(cberr))
	      sprintf(err, "%serror parsing \"%s\" as %s:%s%s", 
		      ME, prms[op], opt[op].CB->type, 
		      strchr(cberr, '\n') ? "\n" : "", cberr);
	    else 
	      sprintf(err, "%serror parsing \"%s\" as %s: return val %d", 
		      ME, prms[op], opt[op].CB->type, ret);
	    return 1;
	  }
	  if (opt[op].CB->delete) {
	    /* vP is the address of a void*, we manage the void * */
	    opt[op].alloc = 1;
	    airMopAdd(pmop, (void**)vP, (airMopper)airSetNull, airMopOnError);
	    airMopAdd(pmop, *((void**)vP), opt[op].CB->delete, airMopOnError);
	  }
	}
      }
      break;
    case 3:
      /* -------- multiple required parameters -------- */
      if (prms[op] && vP) {
	if (airTypeOther != type) {
	  if (opt[op].min !=   /* min == max */
	      airParseStr[type](vP, prms[op], " ", opt[op].min)) {
	    sprintf(err, "%scouldn't parse %s\"%s\" as %d %s%s for %s",
		    ME, udflt[op] ? "(default) " : "", prms[op],
		    opt[op].min, airTypeStr[type], 
		    opt[op].min > 1 ? "s" : "", ident);
	    return 1;
	  }
	  if (airTypeString == type) {
	    /* vP is an array of char*s, (a char**), and what we manage
	       with airMop are the individual vP[p]. */
	    opt[op].alloc = 2;
	    for (p=0; p<=opt[op].min-1; p++) {
	      airMopMem(pmop, &(((char**)vP)[p]), airMopOnError);
	    }
	  }
	}
	else {
	  /* we are parsing "other"s (exactly min (==max) of them) */
	  prmsCopy = airStrdup(prms[op]);
	  for (p=0; p<=opt[op].min-1; p++) {
	    tok = airStrtok(!p ? prmsCopy : NULL, " ", &last);
	    strcpy(cberr, "");
	    ret = opt[op].CB->parse(cP + p*size, tok, cberr);
	    if (ret) {
	      if (strlen(cberr))
		sprintf(err, "%serror parsing \"%s\" (in \"%s\") as %s:%s%s", 
			ME, tok, prms[op], opt[op].CB->type,
			strchr(cberr, '\n') ? "\n" : "", cberr);
	      else 
		sprintf(err, "%serror parsing \"%s\" (in \"%s\") as %s: "
			"return val %d", 
			ME, tok, prms[op], opt[op].CB->type, ret);
	      free(prmsCopy);
	      return 1;
	    }
	  }
	  free(prmsCopy);
	  if (opt[op].CB->delete) {
	    /* vP is an array of void*s, we manage the individual void*s */
	    opt[op].alloc = 2;
	    for (p=0; p<=opt[op].min-1; p++) {
	      airMopAdd(pmop, ((void**)vP)+p, (airMopper)airSetNull,
			airMopOnError);
	      airMopAdd(pmop, *(((void**)vP)+p), opt[op].CB->delete,
			airMopOnError);
	    }
	  }
	}
      }
      break;
    case 4:
      /* -------- optional single variables -------- */
      if (prms[op] && vP) {
	if (airTypeOther != type) {
	  if (1 != airParseStr[type](vP, prms[op], " ", 1)) {
	    sprintf(err, "%scouldn't parse %s\"%s\" as %s for %s",
		    ME, udflt[op] ? "(default) " : "", prms[op],
		    airTypeStr[type], ident);
	    return 1;
	  }
	  opt[op].alloc = (type == airTypeString ? 1 : 0);
	  if (1 == _hestCase(opt, udflt, nprm, appr, op)) {
	    /* we just parsed the default, but now we want to "invert" it */
	    if (airTypeString == type) {
	      *((char**)vP) = airFree(*((char**)vP));
	      opt[op].alloc = 0;
	    }
	    else {
	      tmpD = airDLoad(vP, type);
	      airIStore(vP, type, tmpD ? 0 : 1);
	    }
	  }
	  if (airTypeString == type && opt[op].alloc) {
	    /* vP is the address of a char* (a char**), and what we
	       manage with airMop is the char * */
	    airMopMem(pmop, vP, airMopOnError);
	  }
	}
	else {
	  /* we're parsing an "other".  We will not perform the special
	     flagged single variable parameter games as done above, so
	     whether this option is flagged or unflagged, we're going
	     to treat it like an unflagged single variable parameter option:
	     if the parameter didn't appear, we'll parse it from the default,
	     if it did appear, we'll parse it from the command line.  Setting
	     up prms[op] thusly has already been done by _hestDefaults() */
	  strcpy(cberr, "");
	  ret = opt[op].CB->parse(vP, prms[op], cberr);
	  if (ret) {
	    if (strlen(cberr))
	      sprintf(err, "%serror parsing \"%s\" as %s:%s%s", 
		      ME, prms[op], opt[op].CB->type,
		      strchr(cberr, '\n') ? "\n" : "", cberr);
	    else 
	      sprintf(err, "%serror parsing \"%s\" as %s: return val %d", 
		      ME, prms[op], opt[op].CB->type, ret);
	    return 1;
	  }
	  if (opt[op].CB->delete) {
	    /* vP is the address of a void*, we manage the void* */
	    opt[op].alloc = 1;
	    airMopAdd(pmop, vP, (airMopper)airSetNull, airMopOnError);
	    airMopAdd(pmop, *((void**)vP), opt[op].CB->delete, airMopOnError);
	  }
	}
      }
      break;
    case 5:
      /* -------- multiple optional parameters -------- */
      if (prms[op] && vP) {
	if (1 == _hestCase(opt, udflt, nprm, appr, op)) {
	  *((void**)vP) = NULL;
	  /* alloc and sawP set above */
	}
	else {
	  if (airTypeString == type) {
	    /* this is sneakiness: we allocate one more element so that
	       the resulting char** is, like argv, NULL-terminated */
	    *((void**)vP) = calloc(nprm[op], size+1);
	  }
	  else {
	    *((void**)vP) = calloc(nprm[op], size);
	  }
	  if (parm->verbosity) {
	    printf("!%s: nprm[%d] = %d\n", me, op, nprm[op]);
	    printf("!%s: new array is at 0x%lx\n", me, 
		   (unsigned long)(*((void**)vP)));
	  }
	  airMopMem(pmop, vP, airMopOnError);
	  *(opt[op].sawP) = nprm[op];
	  /* so far everything we've done is regardless of whether or
	     not we're parsing "other"s */
	  if (airTypeOther != type) {
	    opt[op].alloc = (airTypeString == type ? 3 : 1);
	    if (nprm[op] != 
		airParseStr[type](*((void**)vP), prms[op], " ", nprm[op])) {
	      sprintf(err, "%scouldn't parse %s\"%s\" as %d %s%s for %s",
		      ME, udflt[op] ? "(default) " : "", prms[op],
		      nprm[op], airTypeStr[type], 
		      nprm[op] > 1 ? "s" : "", ident);
	      return 1;
	    }
	    if (airTypeString == type) {
	      /* vP is the address of an array of char*s (a char ***), and
		 what we manage with airMop is the individual (*vP)[p],
		 as well as vP itself (above). */
	      for (p=0; p<=nprm[op]-1; p++) {
		airMopAdd(pmop, (*((char***)vP))[p], airFree, airMopOnError);
	      }
	      /* do the NULL-termination described above */
	      (*((char***)vP))[nprm[op]] = NULL;
	    }
	  }
	  else {
	    /* we're parsing nprm[op] "other"s */
	    cP = *((void**)vP);
	    prmsCopy = airStrdup(prms[op]);
	    opt[op].alloc = (opt[op].CB->delete ? 3 : 1);
	    for (p=0; p<=nprm[op]-1; p++) {
	      tok = airStrtok(!p ? prmsCopy : NULL, " ", &last);
	      strcpy(cberr, "");
	      ret = opt[op].CB->parse(cP + p*size, tok, cberr);
	      if (ret) {
		if (strlen(cberr))
		  sprintf(err,"%serror parsing \"%s\" (in \"%s\") as %s:%s%s", 
			  ME, tok, prms[op], opt[op].CB->type,
			  strchr(cberr, '\n') ? "\n" : "", cberr);

		else 
		  sprintf(err, "%serror parsing \"%s\" (in \"%s\") as %s: "
			  "return val %d", 
			  ME, tok, prms[op], opt[op].CB->type, ret);
		free(prmsCopy);
		return 1;
	      }
	    }
	    free(prmsCopy);
	    if (opt[op].CB->delete) {
	      for (p=0; p<=nprm[op]-1; p++) {
		/* avert your eyes.  vP is the address of an array of void*s.
		   We manage the void*s */
		airMopAdd(pmop, (*((void***)vP))+p, (airMopper)airSetNull,
			  airMopOnError);
		airMopAdd(pmop, *((*((void***)vP))+p), opt[op].CB->delete,
			  airMopOnError);
	      }
	    }
	  }
	}
      }
      break;
    }
  }
  return 0;
}

int
hestParse(hestOpt *opt, int _argc, char **_argv,
	  char **_errP, hestParm *_parm) {
  char me[]="hestParse: ";
  char **argv, **prms, *err;
  int a, argc, argr, *nprm, *appr, *udflt, nrf, numOpts, big;
  airArray *mop;
  hestParm *parm;
  
  numOpts = _hestNumOpts(opt);

  /* -------- initialize the mop! */
  mop = airMopInit();

  /* -------- either copy given _parm, or allocate one */
  if (_parm) {
    parm = _parm;
  }
  else {
    parm = hestParmNew();
    airMopAdd(mop, parm, (airMopper)hestParmFree, airMopAlways);
  }

  /* -------- allocate the err string.  To determine its size with total
     ridiculous safety we have to find the biggest things which can appear
     in the string. */
  big = _hestErrStrlen(opt, _argc, _argv);
  if (!(err = calloc(big, sizeof(char)))) {
    fprintf(stderr, "%s PANIC: couldn't allocate error message "
	    "buffer (size %d)\n", me, big);
    exit(1);
  }
  if (_errP) {
    /* if they care about the error string, than it is mopped only
       when there _wasn't_ an error */
    *_errP = err;
    airMopAdd(mop, _errP, (airMopper)airSetNull, airMopOnOkay);
    airMopAdd(mop, err, airFree, airMopOnOkay);
  }
  else {
    /* otherwise, we're making the error string just for our own
       convenience, and we'll always clean it up on exit */
    airMopAdd(mop, err, airFree, airMopAlways);
  }

  /* -------- check on validity of the hestOpt array */
  if (_hestPanic(opt, err, parm)) {
    airMopError(mop); return 1;
  }

  /* -------- Create all the local arrays used to save state during
     the processing of all the different options */
  nprm = calloc(numOpts, sizeof(int));   airMopMem(mop, &nprm, airMopAlways);
  appr = calloc(numOpts, sizeof(int));   airMopMem(mop, &appr, airMopAlways);
  udflt = calloc(numOpts, sizeof(int));  airMopMem(mop, &udflt, airMopAlways);
  prms = calloc(numOpts, sizeof(char*)); airMopMem(mop, &prms, airMopAlways);
  for (a=0; a<=numOpts-1; a++) {
    prms[a] = NULL;
  }

  /* -------- find out how big the argv array needs to be, first
     by seeing how many args are in the response files, and then adding
     on the args from the actual argv (getting this right the first time
     greatly simplifies the problem of eliminating memory leaks) */
  if (_hestArgsInResponseFiles(&argr, &nrf, _argv, err, parm)) {
    airMopError(mop); return 1;
  }
  argc = argr + _argc - nrf;

  if (parm->verbosity) {
    printf("!%s: nrf = %d; argr = %d; _argc = %d --> argc = %d\n", 
	   me, nrf, argr, _argc, argc);
  }
  argv = calloc(argc+1, sizeof(char*));
  airMopMem(mop, &argv, airMopAlways);

  /* -------- process response files (if any) and set the remaining
     elements of argv */
  if (parm->verbosity) printf("%s: #### calling hestResponseFiles\n", me);
  if (_hestResponseFiles(argv, _argv, nrf, err, parm, mop)) {
    airMopError(mop); return 1;
  }
  if (parm->verbosity) printf("%s: #### hestResponseFiles done!\n", me);
  /*
  _hestPrintArgv(argc, argv);
  */
  /* -------- extract flags and their associated parameters from argv */
  if (parm->verbosity) printf("%s: #### calling hestExtractFlagged\n", me);
  if (_hestExtractFlagged(prms, nprm, appr, 
			   &argc, argv, 
			   opt,
			   err, parm, mop)) {
    airMopError(mop); return 1;
  }
  if (parm->verbosity) printf("%s: #### hestExtractFlagged done!\n", me);
  /*
  _hestPrintArgv(argc, argv);
  */
  /* -------- extract args for unflagged options */
  if (parm->verbosity) printf("%s: #### calling hestExtractUnflagged\n", me);
  if (_hestExtractUnflagged(prms, nprm,
			    &argc, argv,
			    opt,
			    err, parm, mop)) {
    airMopError(mop); return 1;
  }
  if (parm->verbosity) printf("%s: #### hestExtractUnflagged done!\n", me);

  /* currently, any left over arguments indicate error */
  if (argc) {
    sprintf(err, "%sunexpected arg: \"%s\"", ME, argv[0]);
    airMopError(mop); return 1;
  }


  /* -------- learn defaults */
  if (parm->verbosity) printf("%s: #### calling hestDefaults\n", me);
  if (_hestDefaults(prms, udflt, nprm, appr,
		    opt,
		    err, parm, mop)) {
    airMopError(mop); return 1;
  }
  if (parm->verbosity) printf("%s: #### hestDefaults done!\n", me);
  
  /* -------- now, the actual parsing of values */
  if (parm->verbosity) printf("%s: #### calling hestSetValues\n", me);
  if (_hestSetValues(prms, udflt, nprm, appr,
		     opt,
		     err, parm, mop)) {
    airMopError(mop); return 1;
  }
  if (parm->verbosity) printf("%s: #### hestSetValues done!\n", me);

  airMopOkay(mop);
  return 0;
}

/*
******** hestParseFree()
**
** free()s whatever was allocated by hestParse()
** 
** returns NULL only to facilitate use with the airMop functions.
** You should probably ignored this quirk.
*/
void *
hestParseFree(hestOpt *opt) {
  int op, i, numOpts;
  void **vP;
  void ***vAP;
  char **str;
  char ***strP;

  numOpts = _hestNumOpts(opt);
  for (op=0; op<=numOpts-1; op++) {
    vP = opt[op].valueP;
    vAP = opt[op].valueP;
    str = opt[op].valueP;
    strP = opt[op].valueP;
    switch (opt[op].alloc) {
    case 0:
      /* nothing was allocated */
      break;
    case 1:
      *vP = airFree(*vP);
      break;
    case 2:
      if (airTypeString == opt[op].type) {
	for (i=0; i<=opt[op].min-1; i++) {
	  str[i] = airFree(str[i]);
	}
      }
      else {
	for (i=0; i<=opt[op].min-1; i++) {
	  vP[i] = opt[op].CB->delete(vP[i]);
	}
      }
      break;
    case 3:
      if (airTypeString == opt[op].type) {
	for (i=0; i<=*(opt[op].sawP)-1; i++) {
	  (*strP)[i] = airFree((*strP)[i]);
	}
	*strP = airFree(*strP);
      }
      else {
	for (i=0; i<=*(opt[op].sawP)-1; i++) {
	  (*vAP)[i] = airFree((*vAP)[i]);
	}
	*vAP = airFree(*vAP);
      }
      break;
    }
  }
  return NULL;
}
