#ifndef CGBLDEC_H
#define CGBLDEC_H

// ============================================================================
//        __
//   \\__/ o\    (C) 2012-2016  Robert Finch, Stratford
//    \  __ /    All rights reserved.
//     \/_//     robfinch<remove>@finitron.ca
//       ||
//
// C64 - 'C' derived language compiler
//  - 64 bit CPU
//
// This source file is free software: you can redistribute it and/or modify 
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or     
// (at your option) any later version.                                      
//                                                                          
// This source file is distributed in the hope that it will be useful,      
// but WITHOUT ANY WARRANTY; without even the implied warranty of           
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            
// GNU General Public License for more details.                             
//                                                                          
// You should have received a copy of the GNU General Public License        
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    
//                                                                          
// ============================================================================
//
/*
 *	68000 C compiler
 *
 *	Copyright 1984, 1985, 1986 Matthew Brandt.
 *  all commercial rights reserved.
 *
 *	This compiler is intended as an instructive tool for personal use. Any
 *	use for profit without the written consent of the author is prohibited.
 *
 *	This compiler may be distributed freely for non-commercial use as long
 *	as this notice stays intact. Please forward any enhancements or questions
 *	to:
 *
 *		Matthew Brandt
 *		Box 920337
 *		Norcross, Ga 30092
 */

/*      global ParseSpecifierarations     */
#define THOR		0
#define TABLE888	888
#define RAPTOR64	64
#define W65C816     816
#define FISA64      164
#define DSD7        7
#define isThor		(gCpu==THOR)
#define isTable888	(gCpu==TABLE888)
#define isRaptor64	(gCpu==RAPTOR64)
#define is816       (gCpu==W65C816)
#define isFISA64    (gCpu==FISA64)
#define isDSD7      (gCpu==DSD7)
//#define DOTRACE	1
#ifdef DOTRACE
#define TRACE(x)	x
#else
#define TRACE(x)
#endif

extern int maxPn;
extern int hook_predreg;
extern int gCpu;
extern int regGP;
extern int regSP;
extern int regBP;
extern int regLR;
extern int regXLR;
extern int regPC;
extern int regCLP;
extern int farcode;
extern int wcharSupport;
extern int verbose;
extern int use_gp;
extern int address_bits;
extern std::ifstream *ifs;
extern txtoStream ofs;
extern txtoStream lfs;
extern txtoStream dfs;
extern int mangledNames;
extern int sizeOfWord;

/*
extern FILE             *input,
                        *list,
                        *output;
*/
extern FILE *outputG;
extern int incldepth;
extern int              lineno;
extern int              nextlabel;
extern int              lastch;
extern int              lastst;
extern char             lastid[128];
extern char             lastkw[128];
extern char             laststr[MAX_STLP1];
extern int	ival;
extern double           rval;
extern Float128			rval128;
extern int parseEsc;
//extern FloatTriple      FAC1,FAC2;

extern TABLE            gsyms[257],
                        lsyms;
extern TABLE            tagtable;
extern SYM              *lasthead;
extern struct slit      *strtab;
extern Float128		    *quadtab;
extern int              lc_static;
extern int              lc_auto;
extern int				lc_thread;
extern struct snode     *bodyptr;       /* parse tree for function */
extern int              global_flag;
extern TABLE            defsyms;
extern int64_t          save_mask;      /* register save mask */
extern int64_t          fpsave_mask;
extern int				bsave_mask;
extern int uctran_off;
extern int isKernel;
extern int isPascal;
extern int isOscall;
extern int isInterrupt;
extern int isTask;
extern int isNocall;
extern bool isRegister;
extern int asmblock;
extern int optimize;
extern int opt_noregs;
extern int opt_nopeep;
extern int opt_noexpr;
extern int exceptions;
extern int mixedSource;
extern SYM *currentFn;
extern int iflevel;
extern int regmask;
extern int bregmask;
extern Statement *currentStmt;
extern bool dogen;

extern TYP stdint;
extern TYP stduint;
extern TYP stdlong;
extern TYP stdulong;
extern TYP stdshort;
extern TYP stdushort;
extern TYP stdchar;
extern TYP stduchar;
extern TYP stdbyte;
extern TYP stdubyte;
extern TYP stdstring;
extern TYP stddbl;
extern TYP stdtriple;
extern TYP stdflt;
extern TYP stddouble;
extern TYP stdfunc;
extern TYP stdexception;
extern TYP stdconst;
extern TYP stdquad;

extern std::string *declid;
extern Compiler compiler;

// Analyze.c
extern int equalnode(ENODE *node1, ENODE *node2);
extern int bsort(CSE **list);
extern int OptimizationDesireability(CSE *csp);
extern int opt1(Statement *stmt);
extern CSE *olist;         /* list of optimizable expressions */
// CMain.c
extern void closefiles();

extern void error(int n);
extern void needpunc(enum e_sym p,int);
// Memmgt.c
extern void *allocx(int);
extern char *xalloc(int);
extern SYM *allocSYM();
extern TYP *allocTYP();
extern AMODE *allocAmode();
extern ENODE *allocEnode();
extern CSE *allocCSE();
extern void ReleaseGlobalMemory();
extern void ReleaseLocalMemory();

// NextToken.c
extern void initsym();
extern void NextToken();
extern int getch();
extern int my_isspace(char c);
extern void getbase(int64_t);
extern void SkipSpaces();

// Stmt.c
extern int GetTypeHash(TYP *p);
extern struct snode *ParseCompoundStatement();

extern void GenerateDiadic(int op, int len, struct amode *ap1,struct amode *ap2);
// Symbol.c
extern SYM *gsearch(std::string na);
extern SYM *search(std::string na,TABLE *thead);
extern void insert(SYM* sp, TABLE *table);

// ParseFunction.c
extern SYM *BuildParameterList(SYM *sp, int *);

extern char *my_strdup(char *);
// Decl.c
extern int imax(int i, int j);
extern TYP *maketype(int bt, int siz);
extern void dodecl(int defclass);
extern int ParseParameterDeclarations(int);
extern void ParseAutoDeclarations(SYM *sym, TABLE *table);
extern int ParseSpecifier(TABLE *table);
extern SYM* ParseDeclarationPrefix(char isUnion);
extern int ParseStructDeclaration(int);
extern void ParseEnumerationList(TABLE *table);
extern int ParseFunction(SYM *sp);
extern int declare(SYM *sym,TABLE *table,int al,int ilc,int ztype);
extern void initstack();
extern int getline(int listflag);
extern void compile();

// Init.c
extern void doinit(SYM *sp);
// Func.c
extern SYM *makeint(char *);
extern void funcbody(SYM *sp);
// Intexpr.c
extern int GetIntegerExpression(ENODE **p);
// Expr.c
extern ENODE *makenode(int nt, ENODE *v1, ENODE *v2);
extern ENODE *makeinode(int nt, int v1);
extern ENODE *makesnode(int nt, std::string *v1, std::string *v2, int i);
extern TYP *nameref(ENODE **node,int);
extern TYP *forcefit(ENODE **node1,TYP *tp1,ENODE **node2,TYP *tp2);
extern TYP *expression(ENODE **node);
extern int IsLValue(ENODE *node);
extern AMODE *GenerateExpression(ENODE *node, int flags, int size);
extern int GetNaturalSize(ENODE *node);
extern TYP *asnop(ENODE **node);
extern TYP *NonCommaExpression(ENODE **);
// Optimize.c
extern void opt_const(ENODE **node);
// GenerateStatement.c
extern void GenerateStatement(struct snode *stmt);
//extern void GenerateFunction(struct snode *stmt);
extern void GenerateIntoff(struct snode *stmt);
extern void GenerateInton(struct snode *stmt);
extern void GenerateStop(struct snode *stmt);
extern void GenerateAsm(struct snode *stmt);
extern void GenerateFirstcall(struct snode *stmt);
extern void gen_regrestore();
extern AMODE *make_direct(int i);
extern AMODE *makereg(int r);
extern AMODE *makefpreg(int t);
extern AMODE *makebreg(int r);
extern AMODE *makepred(int r);
extern int bitsset(int64_t mask);
extern int popcnt(int64_t m);
// Outcode.c
extern void GenerateByte(int val);
extern void GenerateChar(int val);
extern void genhalf(int val);
extern void GenerateWord(int val);
extern void GenerateLong(int val);
extern void genstorage(int nbytes);
extern void GenerateReference(SYM *sp,int offset);
extern void GenerateLabelReference(int n);
extern void gen_strlab(char *s);
extern void dumplits();
extern int  stringlit(char *s);
extern int quadlit(Float128 *f128);
extern void nl();
extern void seg(int sg, int algn);
extern void cseg();
extern void dseg();
extern void tseg();
//extern void put_code(int op, int len,AMODE *aps, AMODE *apd, AMODE *);
extern void put_code(struct ocode *);
extern char *put_label(int lab, char*, char*, char);
extern char *opstr(int op);
// Peepgen.c
extern void flush_peep();
extern int equal_address(AMODE *ap1, AMODE *ap2);
extern void GenerateLabel(int labno);
extern void GenerateZeradic(int op);
extern void GenerateMonadic(int op, int len, AMODE *ap1);
extern void GenerateDiadic(int op, int len, AMODE *ap1, AMODE *ap2);
extern void GenerateTriadic(int op, int len, AMODE *ap1, AMODE *ap2, AMODE *ap3);
extern void Generate4adic(int op, int len, AMODE *ap1, AMODE *ap2, AMODE *ap3, AMODE *ap4);
extern void GeneratePredicatedMonadic(int pr, int pop, int op, int len, AMODE *ap1);
extern void GeneratePredicatedDiadic(int pop, int pr, int op, int len, AMODE *ap1, AMODE *ap2);
// Gencode.c
extern AMODE *make_label(int lab);
extern AMODE *make_clabel(int lab);
extern AMODE *make_immed(int i);
extern AMODE *make_indirect(int i);
extern AMODE *make_offset(ENODE *node);
extern void swap_nodes(ENODE *node);
extern int isshort(ENODE *node);
// IdentifyKeyword.c
extern int IdentifyKeyword();
// Preproc.c
extern int preprocess();
// CodeGenerator.c
extern AMODE *make_indirect(int i);
extern AMODE *make_indexed(int o, int i);
extern AMODE *make_indx(ENODE *node, int reg);
extern AMODE *make_string(char *s);
extern void GenerateFalseJump(ENODE *node,int label,int predreg);
extern void GenerateTrueJump(ENODE *node,int label,int predreg);
extern char *GetNamespace();
extern char nmspace[20][100];
extern AMODE *GenerateDereference(ENODE *, int, int, int);
extern void MakeLegalAmode(AMODE *ap,int flags, int size);
extern void GenLoad(AMODE *, AMODE *, int size, int);
extern void GenStore(AMODE *, AMODE *, int size);
// List.c
extern void ListTable(TABLE *t, int i);
// Register.c
extern AMODE *GetTempRegister();
extern AMODE *GetTempBrRegister();
extern AMODE *GetTempFPRegister();
extern void ReleaseTempRegister(AMODE *ap);
extern void ReleaseTempReg(AMODE *ap);
extern int TempInvalidate();
extern void TempRevalidate(int sp);
// Table888.c
extern void GenerateTable888Function(SYM *sym, Statement *stmt);
extern void GenerateTable888Return(SYM *sym, Statement *stmt);
extern AMODE *GenerateTable888FunctionCall(ENODE *node, int flags);
extern AMODE *GenTable888Set(ENODE *node);
// Raptor64.c
extern void GenerateRaptor64Function(SYM *sym, Statement *stmt);
extern void GenerateRaptor64Return(SYM *sym, Statement *stmt);
extern AMODE *GenerateRaptor64FunctionCall(ENODE *node, int flags);
extern AMODE *GenerateFunctionCall(ENODE *node, int flags);

extern void GenerateFunction(SYM *sym);
extern void GenerateReturn(Statement *stmt);

extern AMODE *GenerateShift(ENODE *node,int flags, int size, int op);
extern AMODE *GenerateAssignShift(ENODE *node,int flags,int size,int op);
extern AMODE *GenerateBitfieldDereference(ENODE *node, int flags, int size);
extern AMODE *GenerateBitfieldAssign(ENODE *node, int flags, int size);
// err.c
extern void fatal(char *str);

extern int tmpVarSpace();
extern void tmpFreeAll();
extern void tmpReset();
extern int tmpAlloc(int);
extern void tmpFree(int);

extern int GetReturnBlockSize();

enum e_sg { noseg, codeseg, dataseg, stackseg, bssseg, idataseg, tlsseg, rodataseg };

#endif
