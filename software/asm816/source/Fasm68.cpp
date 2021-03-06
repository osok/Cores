#include <stdio.h>
#include <stdlib.h>
#include <share.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <time.h>
#include <fcntl.h>
#include <fwlib.h>
#include "sym.h"
#include "asmbuf.h"
#include "fstreamS19.h"
#define ALLOC
#include "fasm68.h"
#include "err.h"
#include "macro.h"

/* -----------------------------------------------------------------------------
   
   (C) 1999 Bird Computer

   Description :

      stderr is used for assembler messages
      stdout is used for listing

      The only function with access to the output file is emitb.

   Output areas

   const - constant data information is stored in read only area
   code  - code data is stored in read / execute area
   data  - data is stored in read / write area
   stack - data is stored in stack area

   Changes
           Author      : R. Finch
           Date        : 91
           Release     :
           Description : new module

----------------------------------------------------------------------------- */

extern "C" {
   jmp_buf jbFatalErr;
}

main(int argc, char *argv[])
{
   char sfname[300];
   int x;

//   setbuf(stdout, NULL);    // for debugging
   
   if (setjmp(jbFatalErr))    // Fatal error exit point
      exit(0);

   // Initialize vars
   FileNum = -1;              // Working file number (will be incremented on open)
   CurFileNum = -1;
   InputLine = 1;             // Overall input line number
   OutputLine = 1;

   page = 1;                  // page number    Listing variables
   col = 1;                   // column number
   PageLength = 60;           // page length
   PageWidth = 80;            // page width

   Debug = 0;
   liston = 0;
   fListing = FALSE;
   fBinOut = TRUE;
   fSOut = FALSE;
   fErrOut = FALSE;
   fpErr = stderr;
   ShowLines = 0;
   verbose = 0;
   Processor = 0;
   // default to 68000/68008 support with floating point support
   giProcessor = PL_0 | PL_F | PL_M;

   fp_cpid = (1 << 9);
   mmu_cpid = (0 << 9);

   WarnLevel = 0;
   errcount = 0;              // number of errors
   errtype = TRUE;            // global error flag

   CollectingMacro = FALSE;
   macrobufndx = 0;
   MacroCounter =0;           // a count of macros used for macro locals
   InComment = 0;
   InComment2 = 0;
   InQuote = 0;
   CommentChar = '~';
   pass = 1;                  // assembler's current pass

   gSzChar = 0;               // operand size character
   DoingDef = FALSE;

//   ProgramCounter = 0;        // current program location during assembly
//   DataCounter = 0;           // current initialized data address during assembly
//   BSSCounter = 0;            // current uninitialized data area during assembly
   CurrentArea = CODE_AREA;   // Current output area.

   fpFormat = FP_IEEE;			// default to IEEE format

   gOpType.k_factor = FALSE;

   // Blank out operand pointers
   for (x = 0; x < MAX_OPERANDS; x++)
      gOperand[x] = NULL;

   // Allocate storage for symbols.
   SymbolTbl = new CSymbolTbl(1000);   // Global symbol table
   ExtRefTbl = new CExtRefTbl(500);    // External reference table

   MacroTbl = new CMacroTbl(200);
//   StructTbl = new CStructTbl(100);

   // set up input/working buffers
   ibuf.set(inbuf, sizeof(inbuf));        // input buffer which can contain macro expansion

   // begin processing
   fprintf(stderr, verstr2);
   memset(ofname, '\0', sizeof(ofname));
   memset(fnameObj, '\0', sizeof(fnameObj));
   memset(fnameBin, '\0', sizeof(fnameBin));
   memset(fnameList, '\0', sizeof(fnameList));
   memset(fnameSym, '\0', sizeof(fnameSym));
   memset(fnameS, '\0', sizeof(fnameS));
   memset(fnameErr, '\0', sizeof(fnameErr));

   if(argc == 1)
   {
      fprintf(stderr,
         "asm68 <source file> [options]\n\n\r"
         "   /o[[-][b][s][-][l][y][:<filename>] - set output option\n\r"
		 "      - - indicates disable option\n\r"
		 "      b - binary output\r\n"
		 "      e - error output file\r\n"
		 "      s - S19 file format output\r\n"
		 "      l - listing file\r\n"
		 "      y - symbol table\r\n"
		 "      : - override ouput file name\r\n"
//       "   /l           - generate listing file\n\r"
//       "   /r - cross reference file name\n\r"
         "   /Gxx - processor level for assembler\r\n"
		 "      00 = 68000/08\r\n"
		 "      10 = 68010\r\n"
		 "      20 = 68020\r\n"
		 "      30 = 68030\r\n"
		 "      40 = 68040\r\n"
		 "      CPU32\r\n"
		 "      EC030 = 68EC030\r\n"
		 "      EC040 = 68EC040\r\n"
		 "      LC040 = 68LC040\r\n"
		 "Press a key\r\n"
		 );
	  getchar();
	  fprintf(stderr,
		  "\r\n"
		  "* This program is distributed WITHOUT ANY WARENTEE; without even the implied\r\n"
		  "warentee of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\r\n\r\n");
      exit(0);
   }

   fprintf(stderr,
	   "If you find this program useful please send a cheque or money order \r\n"
	   "in the amount of $30 Cdn to:\r\n\r\n"
	   "\tBird Computer\r\n"
	   "\t198 Church St.\r\n"
	   "\tStratford, Ontario, Canada\r\n"
	   "\tN5A 2R5\r\n\r\n");
	fprintf(stderr,
		"email comments, suggestions, bug reports to:\r\n\r\n"
		"\temail@birdcomputer.on.ca\r\n\r\n");
	  fprintf(stderr,
		  "* This program is distributed WITHOUT ANY WARENTEE; without even the implied\r\n"
		  "warentee of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\r\n\r\n");

   // Parse command line switches.
   for(x = 2;  x < argc; x++)
      if (strchr("-/+", *argv[x]))
         ParseCmdLine(argv[x]);

   // Get source file name.
   strcpy(sfname, argv[1]);
   if (!strchr(sfname, '.'))
      strcat(sfname, ".s");   // Add extension ?

   // Get object file name. If there is no output file name set
   // then copy the source name to the output name and change
   // the extension. This is somewhat tricky because of new
   // filename conventions. You must search for the last dot
   // that is part of the filename, not a directory path.
   if(!strlen(ofname))
   {
	   char *p1, *p2, *p3;
	   int xx;
	   // Search for the last '.'
	   p1 = strrchr(sfname, '.');
	   p2 = strrchr(sfname, '\\');
	   p3 = strrchr(sfname, '/');
	   p2 = (p3 > p2) ? p3 : p2;
	   p1 = (p2 > p1) ? p2 : p1;
	   xx = (p1 > sfname) ? (p1 - sfname) : strlen(sfname);
		strncpy(ofname, sfname, xx);
   }

	strcpy(fnameObj, ofname);
	strcpy(fnameBin, ofname);
	strcpy(fnameList, ofname);
	strcpy(fnameSym, ofname);
	strcpy(fnameS, ofname);
	strcpy(fnameErr, ofname);
	strcat(fnameObj, ".o");
	strcat(fnameBin, ".bin");
	strcat(fnameList, ".lst");
	strcat(fnameSym, ".sym");
	strcat(fnameS, ".S19");
	strcat(fnameErr, ".err");

	// Open output streams.
	if (fBinOut)
		if ((fpBin = fopen(fnameBin, "wb")) == NULL)
			err(jbFatalErr, E_OPEN, fnameBin);
	if (fListing)
		if ((fpList = fopen(fnameList, "wb")) == NULL)
			err(jbFatalErr, E_OPEN, fnameList);
	if (fSOut)
		if (!gSOut.open(fnameS, ios::out))
			err(jbFatalErr, E_OPEN, fnameS);
	if (fErrOut)
	{
		if ((fpErr = fopen(fnameErr, "wb")) == NULL)
			err(jbFatalErr, E_OPEN, fnameErr);
	}
	else
		fpErr = stderr;

   if (ObjOut) {
      if (ObjFile.open(fnameObj, O_WRONLY | O_CREAT | O_TRUNC, SH_DENYRW) < 0) {
         err(jbFatalErr, E_OPEN, fnameObj);
      }
   }

   //  Main assembling loop.
   for(pass = 1; pass <= 2; pass++) {
      // Initialize for pass
		StartAddress = 0;
		fStartDefined = FALSE;
		fFirstCode = TRUE;
		gProcessor = giProcessor;
      InputLine = 1;
      OutputLine = 1;
      errcount = 0;
      errors = 0;
      warnings = 0;
      ProgramCounter.reset();// = 0;
      DataCounter.reset(); // = 0;
      BSSCounter.reset();// = 0;
      MacroCounter = 0;
      FirstObj = 1;
      FileNum = -1;
      FileLevel = 0;
      LocalSymTbl = NULL;     // Local Symbol table
      ForceErr = 0;
      fprintf(fpErr, "\r\nPass %d\r\n",pass);
      PrcFile(sfname);
   }

   // Flush object buffer.
   if (ObjOut) {
      ObjFile.flush();
      ObjFile.close();
   }

   // Close streams
   if (fBinOut)
	   fclose(fpBin);
   if (fListing)
	   fclose(fpList);
   if (fSOut)
	   gSOut.close(StartAddress);

   // Display symbol table.
   if(fSymOut) {
      fpSym = fopen(fnameSym, "w");
      if (fpSym) {
         int ii;

         fprintf(fpSym, "Global symbols:\n");
         SymbolTbl->print(fpSym, 1);

         // Print local symbol tables
         fprintf(fpSym, "\n\nLocal symbols:\n");
         for (ii = 0; ii < FileNum; ii++)
            if (File[ii].lst) {
               fprintf(fpSym, "File:%s\n", File[ii].name);
               File[ii].lst->print(fpSym, 1);
               fprintf(fpSym, "\n\n");
            }

         MacroTbl->print(fpSym, 1);
         fclose(fpSym);
      }
      else
         err(NULL, E_OPEN, fnameSym);
   }
   fprintf(fpErr, "Errors: %d\r\n", errors);
   fprintf(fpErr, "Warnings: %d\r\n", warnings);
   if (fpErr != stderr)
		fclose(fpErr);
   return FALSE;
}


/* -----------------------------------------------------------------------------
   Description :
      Parse command line switches used to set assembler options.
----------------------------------------------------------------------------- */

void ParseCmdLine(char *s)
{
   switch(s[1]) {
      case 'l':
         if (s[0] == '-') {
            liston = FALSE;
            fprintf(stderr, "Listing disabled.\n");
         }
         else {
            liston = TRUE;
            fprintf(stderr, "Listing enabled.\n");
         }
         break;

      case 'D':
         Debug = TRUE;
         break;

		 /* Output option
				'o' may be followed by b,-b,s in any order.
				Default is to produce binary output. This can be 
				disabled by specifying -b. 's' indicates to
				produce S format file.
				Next a ':' indicates to override the output file
				name.
		 */
      case 'o':
		  {
			  int xx;
			  __int8 not = 0;
			  
			  for (xx = 2; s[xx]; xx++)
			  {
				  switch(s[xx])
				  {
				  case '-':
					  not = TRUE;
					  break;
				  case 'b':
					  fBinOut = !not;
					  not = FALSE;
					  if (fBinOut)
						  fSOut = FALSE;
					  break;
				  case 'e':
					  fErrOut = !not;
					  not = FALSE;
					  break;
				  case 's':
					  fSOut = !not;
					  not = FALSE;
					  if (fSOut)
						  fBinOut = FALSE;
					  break;
				  case 'l':
					  fListing = !not;
					  liston = !not;
					  if (fListing)
				            fprintf(stderr, "Listing enabled.\n");
						else
							fprintf(stderr, "Listing disabled.\n");
					  not = FALSE;
					  break;
				  case 'y':
					  fSymOut = !not;
					  not = FALSE;
					  break;
				  case ':':
					  xx++;
					  goto exitfor;
				  }
			  }
exitfor:
		  if (s[xx])
			strcpy(ofname, &s[xx]);
		  }
         break;

      case 'G':
		  if (!stricmp("cpu32", &s[2]))
		  {
				giProcessor = giProcessor & ~PL_ALL;
			  giProcessor |= PL_CPU32;
			  break;
		  }
		  if (!stricmp("EC030", &s[2]))
		  {
				giProcessor = giProcessor & ~PL_ALL;
			  giProcessor |= PL_EC30;
			  break;
		  }
		  if (!stricmp("EC040", &s[2]))
		  {
				giProcessor = giProcessor & ~PL_ALL;
			  giProcessor |= PL_EC40;
			  break;
		  }
		  if (!stricmp("LC040", &s[2]))
		  {
				giProcessor = giProcessor & ~PL_ALL;
			  giProcessor |= PL_LC40;
			  break;
		  }
         Processor = atoi(&s[2]);
		 switch (Processor % 100)
		 {
		 case 8:
		 case 00:
				giProcessor = giProcessor & ~PL_ALL;
			 giProcessor |= PL_0; break;
		 case 10:
			giProcessor = giProcessor & ~PL_ALL;
			 giProcessor |= PL_1; break;
		 case 20:
			giProcessor = giProcessor & ~PL_ALL;
			 giProcessor |= PL_2; break;
		 case 30:
			giProcessor = giProcessor & ~PL_ALL;
			 giProcessor |= PL_3; break;
		 case 40:
			giProcessor = giProcessor & ~PL_ALL;
			 giProcessor |= PL_4; break;
		 }
         break;

	  case '4':
		  Processor = 40;
			giProcessor = giProcessor & ~PL_ALL;
		  giProcessor |= PL_4;
		  break;

      case '3':
         Processor = 30;
			giProcessor = giProcessor & ~PL_ALL;
		 giProcessor |= PL_3;
         break;

      case '2':
         Processor = 20;
			giProcessor = giProcessor & ~PL_ALL;
		 giProcessor |= PL_2;
         break;

      case '1':
         Processor = 10;
			giProcessor = giProcessor & ~PL_ALL;
		 giProcessor |= PL_1;
         break;

/*      case 'O':
         ObjOut = TRUE;
         break; */

      default:
         err(NULL, E_CMD, s);
         break;
   }
}


/* -----------------------------------------------------------------------------
   Description:
      look for a symbol, store with appropriate counter

   Parameters :
    (char *) pointer to line containing label

----------------------------------------------------------------------------- */

void label(char *label, int oclass)
{
   CSymbol *p, *q = NULL;
   CSymbol tdef;

   /* -----------------------------------------------------------------
         See if the symbol exists in the symbol table already. If it
      does then the label is being multiply defined.
   ------------------------------------------------------------------ */
//   printf("label:%s|\n", label);
   p = NULL;
   tdef.SetName(label);
   if (oclass == PUB || FileLevel == 0)
      p = SymbolTbl->find(&tdef);
   else
      if (LocalSymTbl)
         p = LocalSymTbl->find(&tdef);

   if (pass == 1)
   {
      // Check if label has been defined already.
      if(p != NULL && p->Defined() == 1) {
         ForceErr = 1;
         err(NULL, E_DEFINED, label);
         ForceErr = 0;
         return;
      }

      // Allocate storage and insert into symbol table.
      if (p == NULL) {
         if (oclass == PUB || FileLevel == 0)
            q = SymbolTbl->allocsym();
         else {
            if (LocalSymTbl)
               q = LocalSymTbl->allocsym();
			else
				return;
         }
         if (q == NULL) {
            err(NULL, E_MEMORY);
            return;
         }
         q->SetName(label);
         if (oclass == PUB || FileLevel == 0)
            p = SymbolTbl->insert(q);
         else {
            if (LocalSymTbl)
               p = LocalSymTbl->insert(q);
			else
				return;
         }
      }

      // Set section and offset value
      p->define(oclass); // was q->define
      
      return;
   }

	// On subsequent pass label must already exist in symbol table.
	if (p == NULL) {
		err(NULL, E_LABEL);
		return;
	}

	// Special symbol
	if (!stricmp(p->Name(), "start")) {
		StartAddress = (__int32)p->Value();
		fStartDefined = TRUE;
	}

	//    On subsequent pass address of label must coincide with
    // the section counter or there is an error.
	if (p->Value() != Counter().val)
		err(NULL, E_PHASE, label);

	emitnull();
}


/* -----------------------------------------------------------------------------
   PrcMneumonic()
   
   Description :
      Processes a mneumonic in the input file.
----------------------------------------------------------------------------- */
   
PrcMneumonic(SOp *optr)
{
	int nops, ret, ii;

//    printf("Prc mneu:");
	nops = 0;

	// search for acceptable processor level
	if ((gProcessor & optr->PrcLevel) == 0)
	{
		err(NULL, E_PROCLEVEL, optr->mneu);
		ibuf.ScanToEOL();
		return TRUE;	// return TRUE to suppress additional msgs
	}

   // If size code is present then save it and increment pointer
   // to operand.
   gSzChar = (char)GetSzChar();
   ibuf.SkipSpaces();
//               xx = expeval(inptr, &inptr);

   // Split operand field into separate operands.
   if (optr->NumOps >= 0)
      g_nops = nops = GetOperands();
//   printf("op0=%s, op1=%s|", gOperand[0], gOperand[1]);
   if (optr->NumOps > 0)
   {
      if (nops != optr->NumOps)
      {
         err(NULL, E_NOPERAND, optr->NumOps);
         ret = FALSE;
         goto errxit1;
      }
   }
   ret = (*(optr->func))(optr);
errxit1:
   // Free operand buffers
   for (ii = 0; ii < nops; ii++) {
      if (gOperand[ii]) {
         free(gOperand[ii]);
         gOperand[ii] = NULL;
      }
   }
   return (ret);
errxit:
   ibuf.ScanToEOL();
   return FALSE;
}


/* -----------------------------------------------------------------------------
   Description :
----------------------------------------------------------------------------- */
int opcmp(void const *a, void const *b)
{
   return stricmp((char *)a, ((SOp *)b)->mneu);
}

int CopyChToMacro(char ch) {
   if (macrobufndx >= MAX_MACRO_EXP) {
       err(NULL, E_MACSIZE);
       return FALSE;
   }
   macrobuf[macrobufndx] = ch;
   macrobufndx++;
   return TRUE;
}

/* -----------------------------------------------------------------------------
   Description :
----------------------------------------------------------------------------- */
int CollectMacroLine()
{
   char ch;

   // Loop until newline or end of buffer encountered.
   while(1)
   {
StartOfLoop:
      // If end of buffer then reset quote flag
      if (ibuf.PeekCh() < 1)
         break;

      //    If end of line detected then copy newline to macro buffer
      if (ibuf.PeekCh() == '\n') {
         CopyChToMacro(ibuf.NextCh());
         break;
      }

      // Check for end of comments
      if (InComment2) {
         if ((ibuf.PeekCh() == '*') && (ibuf.Ptr()[1] == '/')) {
            if (CopyChToMacro(ibuf.NextCh()) == FALSE)
               break;
            InComment2--;
            goto EndOfLoop;
         }
      }

      if (InComment) {
         if (ibuf.PeekCh() == CommentChar) {
            InComment--;
            goto EndOfLoop;
         }
      }

      // If we're not already in a comment look for one.
      if (!InComment && !InComment2) {
         if ((strncmp(ibuf.Ptr(), "comment", 7) == 0) && !IsIdentChar(ibuf.Ptr()[7]))
         {
            int count = 7;

            // Copy 'comment' to macro buffer
            while(count)
            {
               if (CopyChToMacro(ibuf.NextCh()) == FALSE)
                  goto ExitPt;
               --count;
            }
            // Get the comment char and copy to macro buffer.
            while(1) {
               if (CopyChToMacro(ibuf.NextCh()) == FALSE)
                  goto ExitPt;
               if (!isspace(ch)) {
                  CommentChar = ch;
                  break;
               }
            }
            InComment++;   // We're now in comment
            continue;
         }
      }
      
      if (!InComment && !InComment2)
      {
         if (ibuf.PeekCh() == '/' && ibuf.Ptr()[1] == '*') {
            int count = 2;
            // Copy '/ *' to macro buffer
            while(count)
            {
               if (CopyChToMacro(ibuf.NextCh()) == FALSE)
                  goto ExitPt;
               --count;
            }
            InComment2++;
            continue;
         }
      }

      // look for quote
      if (ibuf.PeekCh() == '"') {
         while(1) {
            if (macrobufndx >= MAX_MACRO_EXP) {
               err(NULL, E_MACSIZE);
               goto ExitPt;
            }
            ch = ibuf.NextCh();
            if (ch < 1) goto ExitPt;
            macrobuf[macrobufndx] = ch;
            macrobufndx++;
            if (ch == '"') goto StartOfLoop;
            if (ch =='\n') goto ExitPt;
         }
      }

      // look for quote
      if (ibuf.PeekCh() == '\'') {
         while(1) {
            if (macrobufndx >= MAX_MACRO_EXP) {
               err(NULL, E_MACSIZE);
               goto ExitPt;
            }
            ch = ibuf.NextCh();
            if (ch < 1) goto ExitPt;
            macrobuf[macrobufndx] = ch;
            macrobufndx++;
            if (ch == '\'') goto StartOfLoop;
            if (ch =='\n') goto ExitPt;
         }
      }

EndOfLoop:
      // Skip over comment
      if (ibuf.PeekCh() == ';') {
         if (ibuf.Ptr()[1] != ';') {
            ibuf.ScanToEOL();
            rtrim(macrobuf);
            macrobufndx = strlen(macrobuf);
            CopyChToMacro('\n');
            goto ExitPt;
         }
         // Preserve comment
         else {
            while(1) {
               if (macrobufndx >= MAX_MACRO_EXP) {
                  err(NULL, E_MACSIZE);
                  goto ExitPt;
               }
               ch = ibuf.NextCh();
               if (ch < 1) goto ExitPt;
               macrobuf[macrobufndx] = ch;
               macrobufndx++;
               if (ch =='\n') goto ExitPt;
            }
         }
      }
      // Copy character to macro buffer.
      CopyChToMacro(ibuf.NextCh());
   }
ExitPt:
   return TRUE;
}


/* -----------------------------------------------------------------------------
   PrcLine()
   
   Description :
      Processes a line of the input file.
----------------------------------------------------------------------------- */

PrcLine()
{
   char idbuf[NAME_MAX+1];
   int oldline = -1, lbl = FALSE;
   SOp *optr;
   int idlen, sz;
   char
      *mptr,
      *bptr,
      *eptr,
      *sptr;   // pointer to start of text on line
   char ch;
   
   errtype = TRUE;
//   printf("Line %05d: |%s", InputLine, ibuf.Ptr());

   // Save off pointer to start of line (for macro processing)
   mptr = ibuf.Ptr();
   
   // skip any leading spaces on the line
//   ibuf.SkipSpaces();
 
   // Substitute any macros into the input buffer
   SearchAndSub();

   //    If collecting a macro search for the endm statement (which
   // must be the first statement on the line). If not found then
   // copy the input line to the macro buffer.
   if (CollectingMacro)
   {
      if (!InComment && !InComment2)
      {
         bptr = ibuf.Ptr();
         idlen = ibuf.GetIdentifier(&sptr, &eptr); // will skip leading spaces
         if (idlen == 4)
         {
            if (strnicmp(sptr, "endm", 4) == 0)
            {
               m_endm(0);
               goto LoopStart;   // Allows subsequent commands on line
            }
         }
         else
            // reset pointer to as if GetIdentifier never occurred.
            ibuf.setptr(bptr);
      }
   }

   if (CollectingMacro) {
      ibuf.setptr(mptr);   // reset to start of line
      return (CollectMacroLine());
   }

LoopStart:
   while(1)
   {
      // Check for end of buffer.
      if (ibuf.PeekCh() < 1)
         break;

      // Check for end of line
      if (ibuf.PeekCh() == '\n') {
         ibuf.NextCh();
         break;
      }
//      printf("*");
//      printf("%d ", ibuf.PeekCh());

      // look for quote
//      if (ibuf.PeekCh() == '"') {
//         while(1) {
//            ch = NextCh();
//            if (ch < 1 || ch == '\n')
//               goto ExitPt;
//            if (ch == '"')
//               goto StartOfLoop;
//         }
//      }

      // look for quote
//      if (ibuf.PeekCh() == '\'') {
//         while(1) {
//            ch = NextCh();
//            if (ch < 1 || ch == '\n')
//               goto ExitPt;
//            if (ch == '\'')
//               goto StartOfLoop;
//         }
//      }

      // look for end of comments
      if (InComment2) {
         if (ibuf.PeekCh() == '*' && ibuf.Ptr()[1] == '/') {
            --InComment2;
            ibuf.NextCh();
            goto EndOfLoop;
         }
      }
      else if (InComment) {
         if (ibuf.PeekCh() == CommentChar) {
            --InComment;
            goto EndOfLoop;
         }
      }

      // Check for first type of block comment
      if (!InComment && !InComment2) {
         if ((strncmp(ibuf.Ptr(), "comment", 7) == 0) && !IsIdentChar(ibuf.Ptr()[7])) {
            ibuf.setptr(ibuf.Ptr() + 7);
            CommentChar = ibuf.NextNonSpace();
            InComment++;
            goto EndOfLoop;
         }
      }

      // Check for second type of block comment
      if (!InComment)
      {
         if (ibuf.Ptr()[0] == '/' && ibuf.Ptr()[1] == '*') {
            InComment2++;
            ibuf.NextCh();
            goto EndOfLoop;
         }
      }

      if (InComment || InComment2)
         goto EndOfLoop;

      // Skip any leading spaces on the line
      ibuf.SkipSpaces();

      // As soon as we hit a semicolon ignore the remainder of the line
      if (ibuf.PeekCh() == ';') {
         ibuf.ScanToEOL(); // To advance ptr;
         break;
      }

      idlen = ibuf.GetIdentifier(&sptr, &eptr);
      if (idlen > 0)
      {
         memset(idbuf, '\0', sizeof(idbuf));
         strncpy(idbuf, sptr, min(idlen, NAME_MAX));
         sz = sizeof(SOp);
         optr = (SOp *)bsearch(idbuf, (void *)optab, NOPS, sz, opcmp);
         if (optr)
         {
//            printf("Calling prcmneu:%s|\n", optr->mneu);
            if (PrcMneumonic(optr) == FALSE)
               err(NULL, E_INV);
            continue;
         }
         // If an identifier was found, but it's not an op or psuedo
		 // op then it must be some sort of label
         else
         {
            ibuf.SkipSpaces();         // skip any trailing spaces
            if (ibuf.PeekCh() == ':')
            {
               ibuf.NextCh();          // skip over ':'
               label(idbuf, PRI);
            }
//*** m_equ();?
            else
            {
				// See if it's an equate. If not an equate then
				// assume a label without a following ':'
				if (m_equ(idbuf)) {
					ibuf.ScanToEOL();
					break;
				}
				else {
					label(idbuf, PRI);
					continue;	// We don't want to hit NextCh at EndOfLoop
				}
            }
         }
      }

      // If first character is a dot then process a potential psuedo
	  // op.
      else if (ibuf.PeekCh() == '.')
      {
         ibuf.NextCh();
         if (isalpha(ibuf.PeekCh()))
            continue;
         err(NULL, E_EXTRADOT);
         continue;
      }

      // Check for statement separator
      else if (ibuf.PeekCh() == ':')   // ignore
         ;

      else if (isspace(ibuf.PeekCh()))   // ignore
         ;

      // Garbage on the input line
      else
      {
         err(NULL, E_CHAR);
         ibuf.ScanToEOL();    // Dump the line from input
         break;
      }
EndOfLoop:
      ch = ibuf.NextCh();
      if (ch == '\n' || ch < 1)
         break;
   }
   return TRUE;
}


/* -----------------------------------------------------------------------------
   PrcFile(name);
   char *name;    // filename

   Description :
      Assembles a file.
----------------------------------------------------------------------------- */

PrcFile(char *fname)
{
   int slen;
   int nargs = 0;
   time_t tim;
   FILE *ifp;

   lineno = 0;

   // record file in table
   FileNum++;
   CurFileNum = FileNum;
   if (FileNum >= 255) {
      err(NULL, E_FILES);
      return FALSE;
   }
   File[FileNum].errors = 0;
   File[FileNum].warnings = 0;
   File[FileNum].LastLine = 0;
   if (pass < 2) {
      File[FileNum].name = strdup(fname);
      if (FileLevel > 0)	// was == 1
         File[FileNum].lst = new CSymbolTbl(200);
      else
         File[FileNum].lst = NULL;
   }
   if (FileLevel > 0)	// was == 1
      LocalSymTbl = File[FileNum].lst;

   // echo filename
   fprintf(fpErr, "File: %s\r\n", fname);
   ifp = fopen(fname, "r");
   File[FileNum].fp = ifp;
   if (ifp == NULL) {
      err(NULL, E_OPEN, fname); // Unable to open file.
      return FALSE;
   }

   fprintf(fpErr, verstr2);
   fputs("\r\n", fpErr);
   page = 1;
   col = 1;

   if (pass == 2) {
      FirstObj = 1;

      if(fListing == TRUE) {
         time(&tim);
         fprintf(fpList, verstr, ctime(&tim), page);
         fputs(fname, fpList);
         fputs("\r\n\r\n", fpList);
      }
   }

   ibuf.clear();
   
   // Loop processing lines from file.
   while(!feof(ifp))
   {
      col = 1;
      // If there's nothing in the input buffer then get another line.
      if (ibuf.Buf()[0] == '\0') {
         if (fgets(ibuf.Buf(), MAXLINE, ifp) == NULL)
            break;
//         printf("Got line:%s|\n", ibuf.Buf());
         InputLine++;
         lineno++;
#ifdef DEMO
		 if (lineno > 200)
		 {
			 err(NULL, E_DEMOL);
			 break;
		 }
#endif
         File[CurFileNum].LastLine = lineno;
      }

      if(PrcLine() != TRUE)
         err(NULL, E_INV);

      sol = ibuf.Buf();
      // Only output on second pass
      if (pass >= 2 && fListing) {
         if (col < 43)  // && col > 1)
         {
            fprintf(fpList, "%*s", 43-col, ""); // Tab out listing area
            OutListLine();
         }
      }
      // Purge line from input buffer
      slen = ibuf.Ptr() - ibuf.Buf();
      memmove(ibuf.Buf(), ibuf.Ptr(), ibuf.Size() - slen);
      ibuf.rewind();
   }
   if (pass >= 2 && fListing)
      fputc('\f', fpList); // form feed
   fclose(ifp);   // close file
   return TRUE;
}


/* ---------------------------------------------------------------
   	Description :
		Searches and substitutes macro text for macro
	identifiers. We don't want to perform substitutions while
	inside comments	or quotes.
--------------------------------------------------------------- */

void SearchAndSub()
{
   CMacro tmacr, *mp;
   char *sptr, *eptr, *sptr1;
   char *plist[MAX_MACRO_PARMS];
   char nbuf[NAME_MAX+1];
   int na;
   int idlen = 0;
   int slen, tomove;
   int ic1, ic2, iq;
   int SkipNextIdentifier = 0;
   char ch;

//   printf("Search and Sub:");
   // Copy global comment indicators
   ic1 = InComment;
   ic2 = InComment2;
   // iq should be 0 coming in since we SearchAndSub at start of line processing.
   iq = 0;
   while (ibuf.PeekCh()) {

      if (ic2) {
         while(1) {
            if (ibuf.PeekCh() == '*' && ibuf.Ptr()[1] == '/') {
               ic2--;
               ibuf.NextCh();
               goto EndOfLoop;
            }
            ch = ibuf.NextCh();
            if (ch < 1 || ch == '\n')
               goto EndOfLoop2;
         }
      }

      if (ic1) {
         while(1) {
            if (ibuf.PeekCh() == CommentChar) {
               --ic1;
               goto EndOfLoop;
            }
            ch = ibuf.NextCh();
            if (ch < 1 || ch == '\n')
               goto EndOfLoop2;
         }
      }

      // Comment to EOL ?
      if ((ibuf.PeekCh() == '/' && ibuf.Ptr()[1] == '/') || ibuf.PeekCh() == ';') {
         while(1) {
            ch = ibuf.NextCh();
            if (ch < 1 || ch == '\n')
               goto EndOfLoop2;
         }
      }

      if (ibuf.PeekCh() == '"') {
         ibuf.NextCh();
         while(1) {
            ch = ibuf.NextCh();
            if (ch < 1 || ch == '\n')
               goto EndOfLoop2;
            if (ch == '"')
               goto EndOfLoop;
         }
      }
      
      if (ibuf.PeekCh() == '\'') {
         ibuf.NextCh();
         while(1) {
            ch = ibuf.NextCh();
            if (ch < 1 || ch == '\n')
               goto EndOfLoop2;
            if (ch == '\'')
               goto EndOfLoop;
         }
      }

      if (ibuf.PeekCh() == '\n') {
         ibuf.NextCh();
         goto EndOfLoop2;
      }

      // Block comment
      if (ibuf.PeekCh() == '/' && ibuf.Ptr()[1] == '*') {
         ic2++;
         ibuf.NextCh();
         ibuf.NextCh();
         continue;
      }

      sptr1 = ibuf.Ptr();
      idlen = ibuf.GetIdentifier(&sptr, &eptr); // look for an identifier
      if (idlen) {
         if ((strncmp(sptr, "comment", 7)==0) && !IsIdentChar(sptr[7])) {
            ic1++;
            CommentChar = ibuf.NextNonSpace();
            continue;
         }
         //    If macro definition found, we want to skip over the macro name
         // otherwise the macro will substitute for the macro name during the
         // second pass.
         if ((strncmp(sptr, "macro", 5) == 0) && !IsIdentChar(sptr[5]))
            SkipNextIdentifier = TRUE;
         else {
            if (SkipNextIdentifier == TRUE)
               SkipNextIdentifier = FALSE;
            else {
               memset(nbuf, '\0', sizeof(nbuf));
               strncpy(nbuf, sptr, min(NAME_MAX, idlen));
               tmacr.SetName(nbuf);
               mp = MacroTbl->find(&tmacr);// if the identifier is a macro
               if (mp) {
                  if (mp->Nargs() > 0) {
                     na = ibuf.GetParmList(plist);
                     if (na != mp->Nargs())
                        err(NULL, E_MACROARG);
                  }
                  else
                     na = 0;
                  // slen = length of text substituted for
                  slen = ibuf.Ptr() - sptr1;
                  // tomove = number of characters to move
                  //        = buffer size - current pointer position
                  tomove = ibuf.Size() - (ibuf.Ptr() - ibuf.Buf());
                  // sptr = where to begin substitution
//                  printf("sptr:%.*s|,slen=%d,tomove=%d\n", slen, sptr,slen,tomove);
                  mp->sub(plist, sptr1, slen, tomove);
               }
            }
         }
      }
EndOfLoop:
      ibuf.NextCh();
   }
EndOfLoop2:
   ibuf.rewind();
}
