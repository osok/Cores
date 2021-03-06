// ============================================================================
//        __
//   \\__/ o\    (C) 2012-2015  Robert Finch, Stratford
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
#include        <stdio.h>
#include <string.h>
#include        "c.h"
#include        "expr.h"
#include "Statement.h"
#include        "gen.h"
#include        "cglbdec.h"

#define EXPR_DEBUG
static int isscalar(TYP *tp);
static unsigned char sizeof_flag = 0;
static TYP *ParseCastExpression(ENODE **node);
TYP *forcefit(ENODE **node1,TYP *tp1,ENODE **node2,TYP *tp2);
extern void backup();
extern char *inpline;
extern int parsingParameterList;

// Tells subsequent levels that ParseCastExpression already fetched a token.
//static unsigned char expr_flag = 0;

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

TYP             stdint = { bt_long, bt_long, 0, FALSE, FALSE, FALSE, FALSE, FALSE, 0,0,8, {0, 0}, 0, 0 };
TYP             stduint = { bt_long, bt_long, 0, FALSE, TRUE, FALSE, FALSE, FALSE, 0,0,8, {0, 0}, 0, 0 };
TYP             stdlong = { bt_long, bt_long, 0, FALSE, FALSE, FALSE, FALSE, FALSE, 0,0,8, {0, 0}, 0, 0 };
TYP             stdulong = { bt_long, bt_long, 0, FALSE, TRUE, FALSE, FALSE, FALSE, 0,0,8, {0, 0}, 0, 0 };
TYP             stdshort = { bt_short, bt_short, 0, FALSE, FALSE, FALSE, FALSE, FALSE, 0,0,4, {0, 0}, 0, 0 };
TYP             stdushort = { bt_short, bt_short, 0, FALSE, TRUE, FALSE, FALSE, FALSE, 0,0,4, {0, 0}, 0, 0 };
TYP             stdchar = {bt_char, bt_char, 0, FALSE, FALSE, FALSE, FALSE, FALSE, 0,0,2, {0, 0}, 0, 0 };
TYP             stduchar = {bt_char, bt_char, 0, FALSE, TRUE, FALSE, FALSE, FALSE, 0,0,2, {0, 0}, 0, 0 };
TYP             stdbyte = {bt_byte, bt_byte, 0, FALSE, FALSE, FALSE, FALSE, FALSE, 0,0,1, {0, 0}, 0, 0 };
TYP             stdubyte = {bt_byte, bt_byte, 0, FALSE, TRUE, FALSE, FALSE, FALSE, 0,0,1, {0, 0}, 0, 0 };
TYP             stdstring = {bt_pointer, bt_pointer, 1, FALSE, FALSE, FALSE, FALSE, FALSE, 0,0,4, {0, 0}, &stdchar, 0};
TYP				stddbl = {bt_double, bt_double, 0, FALSE, FALSE, FALSE, FALSE, FALSE, 0,0,8, {0, 0}, 0, 0};
TYP				stdtriple = {bt_triple, bt_triple, 0, FALSE, FALSE, FALSE, FALSE, FALSE, 0,0,12, {0, 0}, 0, 0};
TYP				stdflt = {bt_float, bt_float, 0, FALSE, FALSE, FALSE, FALSE, FALSE, 0,0,4, {0, 0}, 0, 0};
TYP				stddouble = {bt_double, bt_double, 0, FALSE, FALSE, FALSE, FALSE, FALSE, 0,0,8, {0, 0}, 0, 0};
TYP             stdfunc = {bt_func, bt_func, 1, FALSE, FALSE, FALSE, FALSE, FALSE, 0,0,0, {0, 0}, &stdint, 0};
TYP             stdexception = { bt_exception, bt_exception, 0, FALSE, TRUE, FALSE, FALSE, FALSE, 0,0,8, {0, 0}, 0, 0 };
extern TYP      *head;          /* shared with ParseSpecifier */
extern TYP	*tail;

/*
 *      expression evaluation
 *
 *      this set of routines builds a parse tree for an expression.
 *      no code is generated for the expressions during the build,
 *      this is the job of the codegen module. for most purposes
 *      expression() is the routine to call. it will allow all of
 *      the C operators. for the case where the comma operator is
 *      not valid (function parameters for instance) call NonCommaExpression().
 *
 *      each of the routines returns a pointer to a describing type
 *      structure. each routine also takes one parameter which is a
 *      pointer to an expression node by reference (address of pointer).
 *      the completed expression is returned in this pointer. all
 *      routines return either a pointer to a valid type or NULL if
 *      the hierarchy of the next operator is too low or the next
 *      symbol is not part of an expression.
 */

TYP     *expression();  /* forward ParseSpecifieraration */
TYP     *NonCommaExpression();      /* forward ParseSpecifieraration */
TYP     *ParseUnaryExpression();       /* forward ParseSpecifieraration */

int nest_level = 0;

void Enter(char *p)
{
/*
     int nn;
     
     for (nn = 0; nn < nest_level; nn++)
         printf("   ");
     printf("%s: %d\r\n", p, lineno);
     nest_level++;
*/
}
void Leave(char *p, int n)
{
/*
     int nn;
     
     nest_level--;
     for (nn = 0; nn < nest_level; nn++)
         printf("   ");
     printf("%s (%d)\r\n", p, n);
*/
}

/*
 *      build an expression node with a node type of nt and values
 *      v1 and v2.
 */
ENODE *makenode(int nt, ENODE *v1, ENODE *v2)
{
	ENODE *ep;
    ep = (ENODE *)xalloc(sizeof(ENODE));
    ep->nodetype = (enum e_node)nt;
    ep->constflag = FALSE;
	ep->isUnsigned = FALSE;
	ep->etype = bt_void;
	ep->esize = -1;
	ep->p[0] = v1;
	ep->p[1] = v2;
    return ep;
}

ENODE *makesnode(int nt, char *v1, int64_t i)
{
	ENODE *ep;
    ep = (ENODE *)xalloc(sizeof(ENODE));
    ep->nodetype = (enum e_node)nt;
    ep->constflag = FALSE;
	ep->isUnsigned = FALSE;
	ep->etype = bt_void;
	ep->esize = -1;
	ep->sp = v1;
	ep->i = i;
    return ep;
}

ENODE *makenodei(int nt, ENODE *v1, int64_t i)
{
	ENODE *ep;
    ep = (ENODE *)xalloc(sizeof(ENODE));
    ep->nodetype = (enum e_node)nt;
    ep->constflag = FALSE;
	ep->isUnsigned = FALSE;
	ep->etype = bt_void;
	ep->esize = -1;
	ep->i = i;
	ep->p[0] = v1;
	ep->p[1] = (ENODE *)NULL;
    return ep;
}

ENODE *makeinode(int nt, int64_t v1)
{
	ENODE *ep;
    ep = (ENODE *)xalloc(sizeof(ENODE));
    ep->nodetype = (enum e_node)nt;
    ep->constflag = TRUE;
	ep->isUnsigned = FALSE;
	ep->etype = bt_void;
	ep->esize = -1;
    ep->i = v1;
    return ep;
}

ENODE *makefnode(int nt, double v1)
{
	ENODE *ep;
    ep = (ENODE *)xalloc(sizeof(ENODE));
    ep->nodetype = (enum e_node)nt;
    ep->constflag = TRUE;
	ep->isUnsigned = FALSE;
	ep->etype = bt_void;
	ep->esize = -1;
	ep->f = v1;
    ep->f1 = v1;
//    ep->f2 = v2;
    return ep;
}

void PromoteConstFlag(ENODE *ep)
{
	ep->constflag = ep->p[0]->constflag && ep->p[1]->constflag;
}

/*
 *      build the proper dereference operation for a node using the
 *      type pointer tp.
 */
TYP *deref(ENODE **node, TYP *tp)
{
	switch( tp->type ) {
		case bt_byte:
			if (tp->isUnsigned) {
				*node = makenode(en_ub_ref,*node,(ENODE *)NULL);
				(*node)->isUnsigned = TRUE;
			}
			else {
				*node = makenode(en_b_ref,*node,(ENODE *)NULL);
			}
			(*node)->esize = tp->size;
			(*node)->etype = (enum e_bt)tp->type;
			if (tp->isUnsigned)
	            tp = &stdubyte;//&stduint;
			else
	            tp = &stdbyte;//&stdint;
            break;
		case bt_ubyte:
			*node = makenode(en_ub_ref,*node,(ENODE *)NULL);
			(*node)->isUnsigned = TRUE;
			(*node)->esize = tp->size;
			(*node)->etype = (enum e_bt)tp->type;
            tp = &stdubyte;//&stduint;
            break;
		case bt_uchar:
		case bt_char:
        case bt_enum:
			if (tp->isUnsigned) {
				*node = makenode(en_uc_ref,*node,(ENODE *)NULL);
				(*node)->isUnsigned = TRUE;
			}
			else
				*node = makenode(en_c_ref,*node,(ENODE *)NULL);
			(*node)->esize = tp->size;
			(*node)->etype = (enum e_bt)tp->type;
			if (tp->isUnsigned)
	            tp = &stduchar;
			else
	            tp = &stdchar;
            break;
		case bt_ushort:
		case bt_short:
			if (tp->isUnsigned) {
				*node = makenode(en_uh_ref,*node,(ENODE *)NULL);
				(*node)->esize = tp->size;
				(*node)->etype = (enum e_bt)tp->type;
				(*node)->isUnsigned = TRUE;
				tp = &stduint;
			}
			else {
				*node = makenode(en_h_ref,*node,(ENODE *)NULL);
				(*node)->esize = tp->size;
				(*node)->etype = (enum e_bt)tp->type;
				tp = &stdint;
			}
            break;
		case bt_exception:
			(*node)->esize = tp->size;
			(*node)->etype = (enum e_bt)tp->type;
			(*node)->isUnsigned = TRUE;
			*node = makenode(en_uw_ref,*node,(ENODE *)NULL);
            break;
		case bt_ulong:
		case bt_long:
			(*node)->esize = tp->size;
			(*node)->etype = (enum e_bt)tp->type;
			if (tp->isUnsigned) {
				(*node)->isUnsigned = TRUE;
				*node = makenode(en_uw_ref,*node,(ENODE *)NULL);
			}
			else {
				*node = makenode(en_w_ref,*node,(ENODE *)NULL);
			}
            break;
		case bt_pointer:
			(*node)->esize = tp->size;
			(*node)->etype = (enum e_bt)tp->type;
            *node = makenode(en_uw_ref,*node,(ENODE *)NULL);
			(*node)->isUnsigned = TRUE;
            break;
		case bt_unsigned:
			(*node)->esize = tp->size;
			(*node)->etype = (enum e_bt)tp->type;
            *node = makenode(en_uw_ref,*node,(ENODE *)NULL);
			(*node)->isUnsigned = TRUE;
            break;
        case bt_triple:
            *node = makenode(en_triple_ref,*node,(ENODE *)NULL);
			(*node)->esize = tp->size;
			(*node)->etype = (enum e_bt)tp->type;
            tp = &stdtriple;
            break;
        case bt_double:
            *node = makenode(en_dbl_ref,*node,(ENODE *)NULL);
			(*node)->esize = tp->size;
			(*node)->etype = (enum e_bt)tp->type;
			(*node)->isDouble = TRUE;
            tp = &stddbl;
            break;
        case bt_float:
            *node = makenode(en_flt_ref,*node,(ENODE *)NULL);
			(*node)->esize = tp->size;
			(*node)->etype = (enum e_bt)tp->type;
            tp = &stdflt;
            break;
		case bt_bitfield:
			if (tp->isUnsigned){
				if (tp->size==1)
					*node = makenode(en_ubfieldref, *node, (ENODE *)NULL);
				else if (tp->size==2)
					*node = makenode(en_ucfieldref, *node, (ENODE *)NULL);
				else if (tp->size==4)
					*node = makenode(en_uhfieldref, *node, (ENODE *)NULL);
				else
					*node = makenode(en_wfieldref, *node, (ENODE *)NULL);
				(*node)->isUnsigned = TRUE;
			}
			else {
				if (tp->size==1)
					*node = makenode(en_bfieldref, *node, (ENODE *)NULL);
				else if (tp->size==2)
					*node = makenode(en_cfieldref, *node, (ENODE *)NULL);
				else if (tp->size==4)
					*node = makenode(en_hfieldref, *node, (ENODE *)NULL);
				else
					*node = makenode(en_wfieldref, *node, (ENODE *)NULL);
			}
			(*node)->bit_width = tp->bit_width;
			(*node)->bit_offset = tp->bit_offset;
			/*
			* maybe it should be 'unsigned'
			*/
			(*node)->etype = (enum e_bt)stdint.type;
			(*node)->esize = tp->size;
			tp = &stdint;
			break;
		//case bt_func:
		//case bt_ifunc:
		//	(*node)->esize = tp->size;
		//	(*node)->etype = tp->type;
		//	(*node)->isUnsigned = TRUE;
		//	*node = makenode(en_uw_ref,*node,NULL);
  //          break;
		case bt_struct:
		case bt_union:
			(*node)->esize = tp->size;
			(*node)->etype = tp->type;
            *node = makenode(en_struct_ref,*node,NULL);
			(*node)->isUnsigned = TRUE;
            break;
		default:
			error(ERR_DEREF);
			break;
    }
	(*node)->isVolatile = tp->isVolatile;
    return tp;
}

/*
* dereference the node if val_flag is zero. If val_flag is non_zero and
* tp->type is bt_pointer (array reference) set the size field to the
* pointer size if this code is not executed on behalf of a sizeof
* operator
*/
TYP *CondDeref(ENODE **node, TYP *tp)
{
    TYP *tp1;

    if (tp->val_flag == 0)
		return deref(node, tp);
    if (tp->type == bt_pointer && (sizeof_flag == 0)) {
		tp1 = tp->btp;
		if (tp1==NULL)
		    printf("DIAG: CondDeref: tp1 is NULL\r\n");
		tp = maketype(bt_pointer, 8);
		tp->btp = tp1;
    }
    else if (tp->type==bt_pointer)
        return tp;
    else if (tp->type==bt_struct || tp->type==bt_union)
        return deref(node, tp);
    return tp;
}


/*
 *      nameref will build an expression tree that references an
 *      identifier. if the identifier is not in the global or
 *      local symbol table then a look-ahead to the next character
 *      is done and if it indicates a function call the identifier
 *      is coerced to an external function name. non-value references
 *      generate an additional level of indirection.
 */
TYP *nameref(ENODE **node)
{
	SYM             *sp;
    TYP             *tp;
	char stnm[200];
    sp = gsearch(lastid);
    if( sp == NULL ) {
        while( my_isspace(lastch) )
            getch();
        if( lastch == '(') {
            ++global_flag;
            sp = allocSYM();
            sp->tp = &stdfunc;
            sp->name = litlate(lastid);
            sp->storage_class = sc_external;
            insert(sp,&gsyms[0]);
            --global_flag;
            tp = &stdfunc;
            *node = makesnode(en_cnacon,sp->name,sp->value.i);
            (*node)->constflag = TRUE;
			if (sp->tp->isUnsigned)
				(*node)->isUnsigned = TRUE;
			(*node)->esize = 8;
        }
        else {
            tp = (TYP *)NULL;
            error(ERR_UNDEFINED);
        }
    }
    else    {
            if( (tp = sp->tp) == NULL ) {
                error(ERR_UNDEFINED);
                return (TYP *)NULL;       /* guard against untyped entries */
            }
            switch( sp->storage_class ) {
                    case sc_static:
						if (sp->tp->type==bt_func || sp->tp->type==bt_ifunc) {
								//strcpy(stnm,GetNamespace());
								//strcat(stnm,"_");
								stnm[0] = '\0';
								strcat(stnm,sp->name);
								*node = makesnode(en_cnacon,litlate(stnm),sp->value.i);
								(*node)->constflag = TRUE;
								(*node)->esize = 8;
								//*node = makesnode(en_nacon,sp->name);
								//(*node)->constflag = TRUE;
							}
							else {
								*node = makeinode(en_labcon,sp->value.i);
								(*node)->constflag = TRUE;
								(*node)->esize = 8;
							}
							if (sp->tp->isUnsigned) {
								(*node)->isUnsigned = TRUE;
								(*node)->esize = sp->tp->size;
							}
							(*node)->etype = bt_pointer;//sp->tp->type;
							(*node)->isDouble = sp->tp->type==bt_double;
                            break;
					case sc_thread:
							*node = makeinode(en_labcon,sp->value.i);
							(*node)->constflag = TRUE;
							(*node)->esize = sp->tp->size;
							(*node)->etype = bt_pointer;//sp->tp->type;
							if (sp->tp->isUnsigned)
								(*node)->isUnsigned = TRUE;
							(*node)->isDouble = sp->tp->type==bt_double;
							break;
                    case sc_global:
                    case sc_external:
							if (sp->tp->type==bt_func || sp->tp->type==bt_ifunc)
	                            *node = makesnode(en_cnacon,sp->name,sp->value.i);
							else
	                            *node = makesnode(en_nacon,sp->name,sp->value.i);
                            (*node)->constflag = TRUE;
							(*node)->esize = sp->tp->size;
							(*node)->etype = bt_pointer;//sp->tp->type;
							(*node)->isUnsigned = sp->tp->isUnsigned;
							(*node)->isDouble = sp->tp->type==bt_double;
                            break;
                    case sc_const:
							if (sp->tp->type==bt_float || sp->tp->type==bt_double || sp->tp->type==bt_triple)
								*node = makefnode(en_fcon,sp->value.f);
							else {
								*node = makeinode(en_icon,sp->value.i);
								if (sp->tp->isUnsigned)
									(*node)->isUnsigned = TRUE;
							}
                            (*node)->constflag = TRUE;
							(*node)->esize = sp->tp->size;
							(*node)->isDouble = sp->tp->type==bt_double;
                            break;
                    default:        /* auto and any errors */
                            if( sp->storage_class != sc_auto)
                                    error(ERR_ILLCLASS);
							if (sp->tp->type==bt_float || sp->tp->type==bt_double || sp->tp->type==bt_triple)
								*node = makeinode(en_autofcon,sp->value.i);
							else {
								*node = makeinode(en_autocon,sp->value.i);
								if (sp->tp->isUnsigned)
									(*node)->isUnsigned = TRUE;
							}
							(*node)->esize = sp->tp->size;
							(*node)->etype = bt_pointer;//sp->tp->type;
							(*node)->isDouble = sp->tp->type==bt_double;
                            break;
                    }
                    (*node)->tp = sp->tp;
                    tp = CondDeref(node,tp);
            }
    NextToken();
    return tp;
}

/*
 *      ArgumentList will build a list of parameter expressions in
 *      a function call and return a pointer to the last expression
 *      parsed. since parameters are generally pushed from right
 *      to left we get just what we asked for...
 */
ENODE *ArgumentList()
{
	ENODE *ep1, *ep2;
    ep1 = 0;
    while( lastst != closepa)
	{
        NonCommaExpression(&ep2);          /* evaluate a parameter */
        ep1 = makenode(en_void,ep2,ep1);
        if( lastst != comma)
            break;
        NextToken();
    }
    return ep1;
}

/*
 *      return 1 if st in set of [ kw_char, kw_short, kw_long, kw_int,
 *      kw_float, kw_double, kw_struct, kw_union ]
 */
static int IsIntrinsicType(int st)
{
	return  st == kw_byte || st==kw_char || st == kw_short || st == kw_int || st==kw_void ||
				st == kw_int16 || st == kw_int8 || st == kw_int32 || st == kw_int16 ||
                st == kw_long || st == kw_float || st == kw_double || st == kw_triple || 
                st == kw_enum || st == kw_struct || st == kw_union ||
                st== kw_unsigned || st==kw_signed || st==kw_exception ||
				st == kw_const;
}

int IsBeginningOfTypecast(int st)
{
	SYM *sp;
	if (st==id) {
		sp = search(lastid,&gsyms[0]);
		if (sp)
			return sp->storage_class==sc_typedef;
		return FALSE;
	}
	else
		return IsIntrinsicType(st) || st==kw_volatile;
}

// ----------------------------------------------------------------------------
//      primary will parse a primary expression and set the node pointer
//      returning the type of the expression parsed. primary expressions
//      are any of:
//                      id
//                      constant
//                      string
//                      ( expression )
// ----------------------------------------------------------------------------
TYP *ParsePrimaryExpression(ENODE **node, int got_pa)
{
	ENODE    *pnode, *qnode, *rnode, *snode, *rnode1, *pnode1, *qnode1, *qnode2;
	int64_t i ;
    SYM *sp;
    TYP *tptr;

	qnode1 = (ENODE *)NULL;
	qnode2 = (ENODE *)NULL;
	pnode = (ENODE *)NULL;
    *node = (ENODE *)NULL;
    Enter("ParsePrimary ");
    if (got_pa) {
        tptr = expression(&pnode);
        needpunc(closepa);
        *node = pnode;
        if (pnode==NULL)
           printf("pnode is NULL\r\n");
        else
           (*node)->tp = tptr;
        if (tptr)
        Leave("ParsePrimary", tptr->type);
        else
        Leave("ParsePrimary", 0);
        return tptr;
    }
    switch( lastst ) {
	case ellipsis:
    case id:
        tptr = nameref(&pnode);
        break;
    case iconst:
        tptr = &stdint;
        tptr->isConst = TRUE;
        pnode = makeinode(en_icon,ival);
        pnode->constflag = TRUE;
		if (ival >= -128 && ival < 128)
			pnode->esize = 1;
		else if (ival >= -32768 && ival < 32768)
			pnode->esize = 2;
		else if (ival >= -2147483648LL && ival < 2147483648LL)
			pnode->esize = 4;
		else
			pnode->esize = 8;
        pnode->tp = tptr;
        NextToken();
        break;
    case rconst:
        tptr = &stddouble;
        tptr->isConst = TRUE;
        pnode = makefnode(en_fcon,rval);
        pnode->constflag = TRUE;
        pnode->isDouble = TRUE;
        pnode->tp = tptr;
        NextToken();
        break;
    case sconst:
		if (sizeof_flag) {
			tptr = maketype(bt_pointer, 0);
			tptr->size = strlen(laststr) + 1;
			tptr->btp = &stdchar;
            tptr->btp->isConst = TRUE;
			tptr->val_flag = 1;
			tptr->isConst = TRUE;
		}
		else {
            tptr = &stdstring;
		}
        pnode = makenodei(en_labcon,(ENODE *)NULL,0);
		if (sizeof_flag == 0)
			pnode->i = stringlit(laststr);
		pnode->etype = bt_pointer;
		pnode->esize = 8;
        pnode->constflag = TRUE;
        pnode->tp = tptr;
     	tptr->isConst = TRUE;
        NextToken();
        break;

    case openpa:
        NextToken();
j1:
//        if( !IsBeginningOfTypecast(lastst) ) {
//		expr_flag = 0;
        tptr = expression(&pnode);
        pnode->tp = tptr;
        needpunc(closepa);
//        }
        //else {			/* cast operator */
        //    ParseSpecifier(0); /* do cast ParseSpecifieraration */
        //    ParseDeclarationPrefix(FALSE);
        //    tptr = head;
        //    needpunc(closepa);
        //    if( ParseUnaryExpression(&pnode) == NULL ) {
        //        error(ERR_IDEXPECT);
        //        tptr = NULL;
        //    }
        //}
        break;

    default:
        Leave("ParsePrimary", 0);
        return (TYP *)NULL;
    }
fini:   *node = pnode;
    if (*node)
       (*node)->tp = tptr;
    if (tptr)
    Leave("ParsePrimary", tptr->type);
    else
    Leave("ParsePrimary", 0);
    return tptr;
}

/*
 *      this function returns true if the node passed is an IsLValue.
 *      this can be qualified by the fact that an IsLValue must have
 *      one of the dereference operators as it's top node.
 */
int IsLValue(ENODE *node)
{
	switch( node->nodetype ) {
    case en_b_ref:
	case en_c_ref:
	case en_h_ref:
    case en_w_ref:
	case en_ub_ref:
	case en_uc_ref:
	case en_uh_ref:
    case en_uw_ref:
	case en_wfieldref:
	case en_uwfieldref:
	case en_bfieldref:
	case en_ubfieldref:
	case en_cfieldref:
	case en_ucfieldref:
	case en_hfieldref:
	case en_uhfieldref:
    case en_triple_ref:
	case en_dbl_ref:
	case en_flt_ref:
	case en_struct_ref:
            return TRUE;
	case en_cbc:
	case en_cbh:
    case en_cbw:
	case en_cch:
	case en_ccw:
	case en_chw:
	case en_cfd:
	case en_cubw:
	case en_cucw:
	case en_cuhw:
	case en_cbu:
	case en_ccu:
	case en_chu:
	case en_cubu:
	case en_cucu:
	case en_cuhu:
            return IsLValue(node->p[0]);
    }
    return FALSE;
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
TYP *Autoincdec(TYP *tp, ENODE **node, int flag)
{
	ENODE *ep1, *ep2;
	int su;

	ep1 = *node;
	if( IsLValue(ep1) ) {
		if (tp->type == bt_pointer) {
			ep2 = makeinode(en_icon,tp->btp->size);
			ep2->esize = 8;
		}
		else {
			ep2 = makeinode(en_icon,1);
			ep2->esize = 1;
		}
		ep2->constflag = TRUE;
		ep2->isUnsigned = tp->isUnsigned;
		su = ep1->isUnsigned;
		ep1 = makenode(flag ? en_assub : en_asadd,ep1,ep2);
		ep1->isUnsigned = tp->isUnsigned;
		ep1->esize = tp->size;
	}
    else
        error(ERR_LVALUE);
	*node = ep1;
	if (*node)
    	(*node)->tp = tp;
	return tp;
}

// ----------------------------------------------------------------------------
// A Postfix Expression is:
//		primary
//		postfix_expression[expression]
//		postfix_expression()
//		postfix_expression(argument expression list)
//		postfix_expression.ID
//		postfix_expression->ID
//		postfix_expression++
//		postfix_expression--
// ----------------------------------------------------------------------------

TYP *ParsePostfixExpression(ENODE **node, int got_pa)
{
	TYP *tp1;
	ENODE *ep1;
	ENODE *rnode,*qnode;
	SYM *sp;
	int iu;

    ep1 = (ENODE *)NULL;
    Enter("ParsePostfix ");
    *node = (ENODE *)NULL;
	tp1 = ParsePrimaryExpression(&ep1, got_pa);
//	if (ep1==NULL)
//	   printf("DIAG: ParsePostFix: ep1 is NULL\r\n");
	if (tp1 == NULL) {
        Leave("ParsePostfix",0);
		return (TYP *)NULL;
    }
	while(1) {
		switch(lastst) {
		case openbr:
			if (tp1==NULL) {
				error(ERR_UNDEFINED);
				goto j1;
			}
            if( tp1->type != bt_pointer )
                error(ERR_NOPOINTER);
            else
                tp1 = tp1->btp;
            NextToken();
			if ((tp1->val_flag && (tp1->size==1 || tp1->size==2 || tp1->size==4 || tp1->size==8))) {
				expression(&rnode);
				ep1 = makenode(en_add,rnode,ep1);
				ep1->constflag = rnode->constflag && ep1->p[1]->constflag;
				ep1->isUnsigned = rnode->isUnsigned && ep1->p[1]->isUnsigned;
				ep1->scale = tp1->size;
				ep1->esize = 8;             // was 8
			}
			else {
				qnode = makeinode(en_icon,tp1->size);
				qnode->constflag = TRUE;
				qnode->isUnsigned = tp1->isUnsigned;
				expression(&rnode);
/*
 *      we could check the type of the expression here...
									
 */
				if (rnode==NULL) {
					error(ERR_EXPREXPECT);
					break;
				}
				qnode = makenode(en_mulu,rnode,qnode);
				qnode->constflag = rnode->constflag && qnode->p[0]->constflag;
				ep1 = makenode(en_add,qnode,ep1);
				ep1->constflag = qnode->constflag && ep1->p[1]->constflag;
				ep1->isUnsigned = qnode->isUnsigned && ep1->p[1]->isUnsigned;
				ep1->scale = 1;
				ep1->esize = 8;
			}
            tp1 = CondDeref(&ep1,tp1);
            needpunc(closebr);
            break;

		case openpa:
            NextToken();
            if( tp1->type != bt_func && tp1->type != bt_ifunc )
                error(ERR_NOFUNC);
            else
                tp1 = tp1->btp;
			if (currentFn==NULL)
				error(ERR_SYNTAX);
			else
				currentFn->IsLeaf = FALSE;
			if (lastst==closepa) {
				NextToken();
				ep1 = makenode(en_fcall,ep1,(ENODE *)NULL);
			}
			else {
				ep1 = makenode(en_fcall,ep1,ArgumentList());
				needpunc(closepa);
			}
			ep1->esize = 8;
            break;

		case pointsto:
			if (tp1==NULL) {
				error(ERR_UNDEFINED);
				goto j1;
			}
            if( tp1->type != bt_pointer ) {
                /*
                printf("\r\n **********");
                printf("\r\n **********");
                printf("NO pointer (%d)\r\n", tp1->type);
                printf("\r\n **********");
                printf("\r\n **********"); 
                */
                error(ERR_NOPOINTER);
            }
            else
                tp1 = tp1->btp;
            if( tp1->val_flag == FALSE )
                ep1 = makenode(en_w_ref,ep1,(ENODE *)NULL);
		 // fall through to dot operation
		case dot:
			if (tp1==NULL) {
				error(ERR_UNDEFINED);
				goto j1;
			}
            NextToken();       /* past -> or . */
            if( lastst != id )
                error(ERR_IDEXPECT);
            else {
                sp = search(lastid,&tp1->lst);
                if( sp == NULL ) {
                    error(ERR_NOMEMBER);
                }
                else {
                    tp1 = sp->tp;
                    qnode = makeinode(en_icon,sp->value.i);
                    qnode->constflag = TRUE;
					iu = ep1->isUnsigned;
                    ep1 = makenode(en_add,ep1,qnode);
                    ep1->constflag = ep1->p[0]->constflag;
					ep1->isUnsigned = iu;
					ep1->esize = 8;
                   tp1 = CondDeref(&ep1,tp1);
                }
                NextToken();       /* past id */
            }
            break;
		case autodec:
			NextToken();
			Autoincdec(tp1,&ep1,1);
			break;
		case autoinc:
			NextToken();
			Autoincdec(tp1,&ep1,0);
			break;
		default:	goto j1;
		}
	}
j1:
	*node = ep1;
	if (ep1)
    	(*node)->tp = tp1;
	if (tp1)
	Leave("ParsePostfix", tp1->type);
	else
	Leave("ParsePostfix", 0);
	return tp1;
}

/*
 *      ParseUnaryExpression evaluates unary expressions and returns the type of the
 *      expression evaluated. unary expressions are any of:
 *
 *                      postfix expression
 *                      ++unary
 *                      --unary
 *                      !cast_expression
 *                      ~cast_expression
 *                      -cast_expression
 *                      +cast_expression
 *                      *cast_expression
 *                      &cast_expression
 *                      sizeof(typecast)
 *                      sizeof unary
 *                      typenum(typecast)
 *
 */
TYP *ParseUnaryExpression(ENODE **node, int got_pa)
{
	TYP *tp, *tp1;
    ENODE *ep1, *ep2;
    int flag2;
	SYM *sp;

    Enter("ParseUnary");
    ep1 = NULL;
    *node = (ENODE *)NULL;
	flag2 = FALSE;
	if (got_pa) {
        tp = ParsePostfixExpression(&ep1, got_pa);
		*node = ep1;
        if (ep1)
    		(*node)->tp = tp;
        if (tp)
        Leave("ParseUnary", tp->type);
        else
        Leave("ParseUnary", 0);
		return tp;
	}
    switch( lastst ) {
    case autodec:
		NextToken();
		tp = ParseUnaryExpression(&ep1, got_pa);
		Autoincdec(tp,&ep1,1);
		break;
    case autoinc:
		NextToken();
		tp = ParseUnaryExpression(&ep1, got_pa);
		Autoincdec(tp,&ep1,0);
		break;
	case plus:
        NextToken();
        tp = ParseCastExpression(&ep1);
        if( tp == NULL ) {
            error(ERR_IDEXPECT);
            return (TYP *)NULL;
        }
        break;
    case minus:
        NextToken();
        tp = ParseCastExpression(&ep1);
        if( tp == NULL ) {
            error(ERR_IDEXPECT);
            return (TYP *)NULL;
        }
        ep1 = makenode(en_uminus,ep1,(ENODE *)NULL);
        ep1->constflag = ep1->p[0]->constflag;
		ep1->isUnsigned = ep1->p[0]->isUnsigned;
		ep1->esize = tp->size;
		ep1->etype = tp->type;
        break;
    case nott:
        NextToken();
        tp = ParseCastExpression(&ep1);
        if( tp == NULL ) {
            error(ERR_IDEXPECT);
            return (TYP *)NULL;
        }
        ep1 = makenode(en_not,ep1,(ENODE *)NULL);
        ep1->constflag = ep1->p[0]->constflag;
		ep1->isUnsigned = ep1->p[0]->isUnsigned;
		ep1->esize = tp->size;
        break;
    case cmpl:
        NextToken();
        tp = ParseCastExpression(&ep1);
        if( tp == NULL ) {
            error(ERR_IDEXPECT);
            return 0;
        }
        ep1 = makenode(en_compl,ep1,(ENODE *)NULL);
        ep1->constflag = ep1->p[0]->constflag;
		ep1->isUnsigned = ep1->p[0]->isUnsigned;
		ep1->esize = tp->size;
        break;
    case star:
        NextToken();
        tp = ParseCastExpression(&ep1);
        if( tp == NULL ) {
            error(ERR_IDEXPECT);
            return (TYP *)NULL;
        }
        if( tp->btp == NULL )
			error(ERR_DEREF);
        else
            tp = tp->btp;
		tp1 = tp;
		//Autoincdec(tp,&ep1);
		tp = CondDeref(&ep1,tp);
        break;
    case bitandd:
        NextToken();
        tp = ParseCastExpression(&ep1);
        if( tp == NULL ) {
            error(ERR_IDEXPECT);
            return (TYP *)NULL;
        }
        if( IsLValue(ep1))
            ep1 = ep1->p[0];
        ep1->esize = 8;     // converted to a pointer so size is now 8
        tp1 = allocTYP();
        tp1->size = 8;
        tp1->type = bt_pointer;
        tp1->btp = tp;
        tp1->val_flag = FALSE;
        tp1->lst.head = (SYM *)NULL;
        tp1->sname = (char *)NULL;
        tp = tp1;
//        printf("tp %p: %d\r\n", tp, tp->type);
/*
		sp = search("ta_int",&tp->btp->lst);
		if (sp) {
			printf("bitandd: ta_int\r\n");
		}
*/
        break;
    case kw_sizeof:
        NextToken();
		if (lastst==openpa) {
			flag2 = TRUE;
			NextToken();
		}
		if (flag2 && IsBeginningOfTypecast(lastst)) {
			tp = head;
			tp1 = tail;
			ParseSpecifier(0);
			ParseDeclarationPrefix(FALSE);
			if( head != NULL )
				ep1 = makeinode(en_icon,head->size);
			else {
				error(ERR_IDEXPECT);
				ep1 = makeinode(en_icon,1);
			}
			head = tp;
			tail = tp1;
		}
		else {
			sizeof_flag++;
			tp = ParseUnaryExpression(&ep1, got_pa);
			sizeof_flag--;
			if (tp == 0) {
				error(ERR_SYNTAX);
				ep1 = makeinode(en_icon,1);
			} else
				ep1 = makeinode(en_icon, (long) tp->size);
		}
		if (flag2)
			needpunc(closepa);
		ep1->constflag = TRUE;
		ep1->esize = 8;
		tp = &stdint;
        break;
    case kw_typenum:
        NextToken();
        needpunc(openpa);
		tp = head;
		tp1 = tail;
        ParseSpecifier(0);
        ParseDeclarationPrefix(FALSE);
        if( head != NULL )
            ep1 = makeinode(en_icon,GetTypeHash(head));
		else {
            error(ERR_IDEXPECT);
            ep1 = makeinode(en_icon,1);
        }
		head = tp;
		tail = tp1;
        ep1->constflag = TRUE;
		ep1->esize = 8;
        tp = &stdint;
        needpunc(closepa);
        break;
    default:
        tp = ParsePostfixExpression(&ep1, got_pa);
        break;
    }
    *node = ep1;
    if (ep1)
	    (*node)->tp = tp;
    if (tp)
    Leave("ParseUnary", tp->type);
    else
    Leave("ParseUnary", 0);
    return tp;
}

// ----------------------------------------------------------------------------
// A cast_expression is:
//		unary_expression
//		(type name)cast_expression
// ----------------------------------------------------------------------------
static TYP *ParseCastExpression(ENODE **node)
{
	TYP *tp, *tp1, *tp2;
	ENODE *ep1, *ep2;

    Enter("ParseCast ");
    *node = (ENODE *)NULL;
	switch(lastst) {
 /*
	case openpa:
		NextToken();
        if(IsBeginningOfTypecast(lastst) ) {
            ParseSpecifier(0); // do cast declaration
            ParseDeclarationPrefix(FALSE);
            tp = head;
			tp1 = tail;
            needpunc(closepa);
            if((tp2 = ParseCastExpression(&ep1)) == NULL ) {
                error(ERR_IDEXPECT);
                tp = (TYP *)NULL;
            }
			ep2 = makenode(en_void,ep1,(ENODE *)NULL);
			ep2->constflag = ep1->constflag;
			ep2->isUnsigned = ep1->isUnsigned;
			ep2->etype = ep1->etype;
			ep2->esize = ep1->esize;
			forcefit(&ep2,tp2,&ep1,tp);
			head = tp;
			tail = tp1;
        }
		else {
			tp = ParseUnaryExpression(&ep1,1);
		}
		break;
*/
	case openpa:
		NextToken();
        if(IsBeginningOfTypecast(lastst) ) {
            ParseSpecifier(0); // do cast declaration
            ParseDeclarationPrefix(FALSE);
            tp = head;
			tp1 = tail;
            needpunc(closepa);
            if((tp2 = ParseCastExpression(&ep1)) == NULL ) {
                error(ERR_IDEXPECT);
                tp = (TYP *)NULL;
            }
            if (tp2->isConst) {
                *node = ep1;
                return tp;
            }
            if (tp->type == bt_double)
                ep2 = makenode(en_tempfpref,(ENODE *)NULL,(ENODE *)NULL);
            else
                ep2 = makenode(en_tempref,(ENODE *)NULL,(ENODE *)NULL);
			ep2 = makenode(en_void,ep2,ep1);
			ep2->constflag = ep1->constflag;
			ep2->isUnsigned = ep1->isUnsigned;
			ep2->etype = ep1->etype;
			ep2->esize = ep1->esize;
			forcefit(&ep2,tp2,&ep1,tp);
			head = tp;
			tail = tp1;
			*node = ep2;
      		(*node)->tp = tp;
			return tp;
        }
		else {
			tp = ParseUnaryExpression(&ep1,1);
		}
		break;

	default:
		tp = ParseUnaryExpression(&ep1,0);
		break;
	}
	*node = ep1;
	if (ep1)
	    (*node)->tp = tp;
	if (tp)
	Leave("ParseCast", tp->type);
	else
	Leave("ParseCast", 0);
	return tp;
}

// ----------------------------------------------------------------------------
//      forcefit will coerce the nodes passed into compatible
//      types and return the type of the resulting expression.
// ----------------------------------------------------------------------------
TYP *forcefit(ENODE **node1,TYP *tp1,ENODE **node2,TYP *tp2)
{
	ENODE *n2;

	if (tp1==tp2)	// duh
		return tp1;
	if (node2)
		n2 = *node2;
	else
		n2 = (ENODE *)NULL;
	switch( tp1->type ) {
	case bt_ubyte:
		switch(tp2->type) {
		case bt_long:	*node1 = makenode(en_cubw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ulong:	*node1 = makenode(en_cubu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_short:	*node1 = makenode(en_cubw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ushort:	*node1 = makenode(en_cubu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_char:	*node1 = makenode(en_cubw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_uchar:	*node1 = makenode(en_cubu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_byte:	*node1 = makenode(en_cubw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ubyte:	*node1 = makenode(en_cubu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_enum:	*node1 = makenode(en_cubw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_pointer:*node1 = makenode(en_cubu,*node1,*node2); (*node1)->esize = 8; return tp2;
		case bt_exception:	*node1 = makenode(en_cubu,*node1,*node2); (*node1)->esize = 8; return &stdexception;
		}
		return tp1;
	case bt_byte:
		switch(tp2->type) {
		case bt_long:	*node1 = makenode(en_cbw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ulong:	*node1 = makenode(en_cbu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_short:	*node1 = makenode(en_cbw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ushort:	*node1 = makenode(en_cbu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_char:	*node1 = makenode(en_cbw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_uchar:	*node1 = makenode(en_cbu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_byte:	*node1 = makenode(en_cbw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ubyte:	*node1 = makenode(en_cbu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_enum:	*node1 = makenode(en_cbw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_pointer:*node1 = makenode(en_cbu,*node1,*node2); (*node1)->esize = 8; return tp2;
		case bt_exception:	*node1 = makenode(en_cbu,*node1,*node2); (*node1)->esize = 8; return &stdexception;
		}
		return tp1;
	case bt_enum:
		switch(tp2->type) {
		case bt_long:	*node1 = makenode(en_ccw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ulong:	*node1 = makenode(en_ccu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_short:	*node1 = makenode(en_ccw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ushort:	*node1 = makenode(en_ccu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_char:	*node1 = makenode(en_ccw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_uchar:	*node1 = makenode(en_ccu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_byte:	*node1 = makenode(en_ccw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ubyte:	*node1 = makenode(en_ccu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_enum:	*node1 = makenode(en_ccw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_pointer:*node1 = makenode(en_ccu,*node1,*node2); (*node1)->esize = 8; return tp2;
		case bt_exception:	*node1 = makenode(en_ccu,*node1,*node2); (*node1)->esize = 8; return &stdexception;
		}
		return tp1;
	case bt_uchar:
		switch(tp2->type) {
		case bt_long:	*node1 = makenode(en_cucw,*node1,n2); (*node1)->esize = 8; return &stdlong;
		case bt_ulong:	*node1 = makenode(en_cucu,*node1,n2); (*node1)->esize = 8; return &stdulong;
		case bt_short:	*node1 = makenode(en_cucw,*node1,n2); (*node1)->esize = 8; return &stdlong;
		case bt_ushort:	*node1 = makenode(en_cucu,*node1,n2); (*node1)->esize = 8; return &stdulong;
		case bt_char:	*node1 = makenode(en_cucw,*node1,n2); (*node1)->esize = 8; return &stdlong;
		case bt_uchar:	*node1 = makenode(en_cucu,*node1,n2); (*node1)->esize = 8; return &stdulong;
		case bt_byte:	*node1 = makenode(en_cucw,*node1,n2); (*node1)->esize = 8; return &stdlong;
		case bt_ubyte:	*node1 = makenode(en_cucu,*node1,n2); (*node1)->esize = 8; return &stdulong;
		case bt_enum:	*node1 = makenode(en_cucw,*node1,n2); (*node1)->esize = 8; return &stdlong;
		case bt_pointer:*node1 = makenode(en_cucu,*node1,n2); (*node1)->esize = 8; return tp2;
		case bt_exception:	*node1 = makenode(en_cucu,*node1,*node2); (*node1)->esize = 8; return &stdexception;
		}
		return tp1;
	case bt_char:
		switch(tp2->type) {
		case bt_long:	*node1 = makenode(en_ccw,*node1,n2); (*node1)->esize = 8; return &stdlong;
		case bt_ulong:	*node1 = makenode(en_ccu,*node1,n2); (*node1)->esize = 8; return &stdulong;
		case bt_short:	*node1 = makenode(en_ccw,*node1,n2); (*node1)->esize = 8; return &stdlong;
		case bt_ushort:	*node1 = makenode(en_ccu,*node1,n2); (*node1)->esize = 8; return &stdulong;
		case bt_char:	*node1 = makenode(en_ccw,*node1,n2); (*node1)->esize = 8; return &stdlong;
		case bt_uchar:	*node1 = makenode(en_ccu,*node1,n2); (*node1)->esize = 8; return &stdulong;
		case bt_byte:	*node1 = makenode(en_ccw,*node1,n2); (*node1)->esize = 8; return &stdlong;
		case bt_ubyte:	*node1 = makenode(en_ccu,*node1,n2); (*node1)->esize = 8; return &stdulong;
		case bt_enum:	*node1 = makenode(en_ccw,*node1,n2); (*node1)->esize = 8; return &stdlong;
		case bt_pointer:*node1 = makenode(en_ccu,*node1,n2); (*node1)->esize = 8; return tp2;
		case bt_exception:	*node1 = makenode(en_ccu,*node1,*node2); (*node1)->esize = 8; return &stdexception;
		}
		return tp1;
	case bt_ushort:
		switch(tp2->type) {
		case bt_long:	*node1 = makenode(en_cuhw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ulong:	*node1 = makenode(en_cuhu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_short:	*node1 = makenode(en_cuhw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ushort:	*node1 = makenode(en_cuhu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_char:	*node1 = makenode(en_cuhw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_uchar:	*node1 = makenode(en_cuhu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_byte:	*node1 = makenode(en_cuhw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ubyte:	*node1 = makenode(en_cuhu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_enum:	*node1 = makenode(en_cuhw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_pointer:*node1 = makenode(en_cuhu,*node1,*node2); (*node1)->esize = 8; return tp2;
		case bt_exception:	*node1 = makenode(en_cuhu,*node1,*node2); (*node1)->esize = 8; return &stdexception;
		}
		return tp1;
	case bt_short:
		switch(tp2->type) {
		case bt_long:	*node1 = makenode(en_chw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ulong:	*node1 = makenode(en_chu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_short:	*node1 = makenode(en_chw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ushort:	*node1 = makenode(en_chu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_char:	*node1 = makenode(en_chw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_uchar:	*node1 = makenode(en_chu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_byte:	*node1 = makenode(en_chw,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_ubyte:	*node1 = makenode(en_chu,*node1,*node2); (*node1)->esize = 8; return &stdulong;
		case bt_enum:	*node1 = makenode(en_chu,*node1,*node2); (*node1)->esize = 8; return &stdlong;
		case bt_pointer:*node1 = makenode(en_chu,*node1,*node2); (*node1)->esize = 8; return tp2;
		case bt_exception:	*node1 = makenode(en_chu,*node1,*node2); (*node1)->esize = 8; return &stdexception;
		}
		return tp1;
	case bt_exception:
    case bt_long:
    case bt_ulong:
        if( tp2->type == bt_pointer	)
			return tp2;
        if (tp2->type==bt_double) {
            *node1 = makenode(en_i2d,*node1,*node2);
            return tp2;
        }
		//if (tp2->type == bt_double || tp2->type == bt_float) {
		//	*node1 = makenode(en_i2d,*node1,*node2);
		//	return tp2;
		//}
		return tp1;
    case bt_pointer:
            if( isscalar(tp2) || tp2->type == bt_pointer)
                return tp1;
			// pointer to function was really desired.
			if (tp2->type == bt_func || tp2->type==bt_ifunc)
				return tp1;
            break;
    case bt_unsigned:
            if( tp2->type == bt_pointer )
                return tp2;
            else if( isscalar(tp2) )
                return tp1;
            break;
	case bt_float:
			if (tp2->type == bt_double)
				return &stddbl;
			return tp1;
	case bt_double:
			if (tp2->type == bt_long)
				*node1 = makenode(en_d2i,*node1,*node2);
			return tp2;
	case bt_triple:
			if (tp2->type == bt_long)
				*node2 = makenode(en_i2t,*node2,*node1);
			return tp1;
	case bt_struct:
	case bt_union:
			if (tp2->size > tp1->size)
				return tp2;
			return tp1;
	// Really working with pointers to functions.
	case bt_func:
	case bt_ifunc:
			return tp1;
    }
    error( ERR_MISMATCH );
    return tp1;
}

/*
 *      this function returns true when the type of the argument is
 *      one of char, short, unsigned, or long.
 */
static int isscalar(TYP *tp)
{
	return
			tp->type == bt_byte ||
			tp->type == bt_char ||
            tp->type == bt_short ||
            tp->type == bt_long ||
			tp->type == bt_ubyte ||
			tp->type == bt_uchar ||
            tp->type == bt_ushort ||
            tp->type == bt_ulong ||
            tp->type == bt_exception ||
            tp->type == bt_unsigned;
}

/*
 *      multops parses the multiply priority operators. the syntax of
 *      this group is:
 *
 *              unary
 *              unary * unary
 *              unary / unary
 *              unary % unary
 */
TYP *multops(ENODE **node)
{
	ENODE *ep1, *ep2;
	TYP *tp1, *tp2;
	int	oper;
    
    Enter("Mulops");
    ep1 = (ENODE *)NULL;
    *node = (ENODE *)NULL;
	tp1 = ParseCastExpression(&ep1);
	if( tp1 == 0 ) {
        Leave("Mulops NULL",0);
		return 0;
    }
        while( lastst == star || lastst == divide || lastst == modop) {
                oper = lastst;
                NextToken();       /* move on to next unary op */
                tp2 = ParseCastExpression(&ep2);
                if( tp2 == 0 ) {
                        error(ERR_IDEXPECT);
                        *node = ep1;
                        if (ep1)
                            (*node)->tp = tp1;
                        return tp1;
                        }
                tp1 = forcefit(&ep2,tp2,&ep1,tp1);
                switch( oper ) {
                        case star:
                                if (tp1->type==bt_triple) {
									ep1 = makenode(en_ftmul,ep1,ep2);
                                }  
								else if (tp1->type==bt_double)
									ep1 = makenode(en_fdmul,ep1,ep2);
								else if (tp1->type==bt_float)
									ep1 = makenode(en_fsmul,ep1,ep2);
								else if( tp1->isUnsigned )
                                        ep1 = makenode(en_mulu,ep1,ep2);
                                else
                                        ep1 = makenode(en_mul,ep1,ep2);
								ep1->esize = tp1->size;
								ep1->etype = tp1->type;
                                break;
                        case divide:
                                if (tp1->type==bt_triple)
									ep1 = makenode(en_ftdiv,ep1,ep2);
								else if (tp1->type==bt_double)
									ep1 = makenode(en_fddiv,ep1,ep2);
								else if (tp1->type==bt_float)
									ep1 = makenode(en_fsdiv,ep1,ep2);
                                else if( tp1->isUnsigned )
                                    ep1 = makenode(en_udiv,ep1,ep2);
                                else
                                    ep1 = makenode(en_div,ep1,ep2);
                                break;
								ep1->esize = tp1->size;
								ep1->etype = tp1->type;
                        case modop:
                                if( tp1->isUnsigned )
                                        ep1 = makenode(en_umod,ep1,ep2);
                                else
                                        ep1 = makenode(en_mod,ep1,ep2);
								ep1->esize = tp1->size;
								ep1->etype = tp1->type;
                                break;
                        }
                PromoteConstFlag(ep1);
                }
        *node = ep1;
        if (ep1)
		    (*node)->tp = tp1;
    Leave("Mulops",0);
        return tp1;
}

// ----------------------------------------------------------------------------
// Addops handles the addition and subtraction operators.
// ----------------------------------------------------------------------------

static TYP *addops(ENODE **node)
{
	ENODE    *ep1, *ep2, *ep3, *ep4;
    TYP             *tp1, *tp2;
    int             oper;
	int sz1, sz2;

    Enter("Addops");
    ep1 = (ENODE *)NULL;
    *node = (ENODE *)NULL;
	sz1 = sz2 = 0;
	tp1 = multops(&ep1);
    if( tp1 == (TYP *)NULL )
        goto xit;
	if (tp1->type == bt_pointer) {
        if (tp1->btp==NULL) {
            printf("DIAG: pointer to NULL type.\r\n");
            goto xit;    
        }
        else
		    sz1 = tp1->btp->size;
    }
    while( lastst == plus || lastst == minus ) {
            oper = (lastst == plus);
            NextToken();
            tp2 = multops(&ep2);
            if( tp2 == 0 ) {
                    error(ERR_IDEXPECT);
                    *node = ep1;
                    goto xit;
                    }
			if (tp2->type == bt_pointer)
				sz2 = tp2->btp->size;
			// Difference of two pointers to the same type of object...
			// Divide the result by the size of the pointed to object.
			if (!oper && (tp1->type == bt_pointer) && (tp2->type == bt_pointer) && (sz1==sz2))
			{
  				ep1 = makenode( en_sub,ep1,ep2);
				ep4 = makeinode(en_icon, sz1);
				ep1 = makenode(en_udiv,ep1,ep4);
			}
			else {
                if( tp1->type == bt_pointer ) {
                        tp2 = forcefit(&ep2,tp2,0,&stdint);
                        ep3 = makeinode(en_icon,tp1->btp->size);
                        ep3->constflag = TRUE;
    					ep3->esize = tp2->size;
                        ep2 = makenode(en_mulu,ep3,ep2);
                        ep2->constflag = ep2->p[1]->constflag;
    					ep2->esize = tp2->size;
                        }
                else if( tp2->type == bt_pointer ) {
                        tp1 = forcefit(&ep1,tp1,0,&stdint);
                        ep3 = makeinode(en_icon,tp2->btp->size);
                        ep3->constflag = TRUE;
    					ep3->esize = tp2->size;
                        ep1 = makenode(en_mulu,ep3,ep1);
                        ep1->constflag = ep1->p[1]->constflag;
    					ep2->esize = tp2->size;
                        }
                tp1 = forcefit(&ep2,tp2,&ep1,tp1);
                if (tp1->type==bt_triple) {
    				ep1 = makenode( oper ? en_ftadd : en_ftsub,ep1,ep2);
                }
    			else if (tp1->type==bt_double)
    				ep1 = makenode( oper ? en_fdadd : en_fdsub,ep1,ep2);
    			else if (tp1->type==bt_float)
    				ep1 = makenode( oper ? en_fsadd : en_fssub,ep1,ep2);
    			else
    				ep1 = makenode( oper ? en_add : en_sub,ep1,ep2);
            }
            PromoteConstFlag(ep1);
			ep1->esize = tp1->size;
			ep1->etype = tp1->type;
            }
    *node = ep1;
xit:
    if (*node)
    	(*node)->tp = tp1;
    Leave("Addops",0);
    return tp1;
}

// ----------------------------------------------------------------------------
// Shiftop handles the shift operators << and >>.
// ----------------------------------------------------------------------------
TYP *shiftop(ENODE **node)
{
	ENODE    *ep1, *ep2;
    TYP             *tp1, *tp2;
    int             oper;

    Enter("Shiftop");
    *node = NULL;
	tp1 = addops(&ep1);
	if( tp1 == 0)
        goto xit;
    while( lastst == lshift || lastst == rshift) {
            oper = (lastst == lshift);
            NextToken();
            tp2 = addops(&ep2);
            if( tp2 == 0 )
                    error(ERR_IDEXPECT);
            else    {
                    tp1 = forcefit(&ep2,tp2,&ep1,tp1);
					if (tp1->type==bt_float||tp1->type==bt_double)
						error(ERR_UNDEF_OP);
					else {
						if (tp1->isUnsigned)
							ep1 = makenode(oper ? en_shlu : en_shru,ep1,ep2);
						else
							ep1 = makenode(oper ? en_shl : en_asr,ep1,ep2);
						ep1->esize = tp1->size;
						PromoteConstFlag(ep1);
						}
                    }
            }
    *node = ep1;
 xit:
     if (*node)
     	(*node)->tp = tp1;
    Leave("Shiftop",0);
    return tp1;
}

/*
 *      relation handles the relational operators < <= > and >=.
 */
TYP     *relation(ENODE **node)
{       ENODE    *ep1, *ep2;
        TYP             *tp1, *tp2;
        int             nt;
        Enter("Relation");
        *node = (ENODE *)NULL;
        tp1 = shiftop(&ep1);
        if( tp1 == 0 )
                goto xit;
        for(;;) {
                switch( lastst ) {

                        case lt:
                                if (tp1->type==bt_double)
                                    nt = en_flt;
                                else if( tp1->isUnsigned )
                                        nt = en_ult;
                                else
                                        nt = en_lt;
                                break;
                        case gt:
                                if (tp1->type==bt_double)
                                    nt = en_fgt;
                                else if( tp1->isUnsigned )
                                        nt = en_ugt;
                                else
                                        nt = en_gt;
                                break;
                        case leq:
                                if (tp1->type==bt_double)
                                    nt = en_fle;
                                else if( tp1->isUnsigned )
                                        nt = en_ule;
                                else
                                        nt = en_le;
                                break;
                        case geq:
                                if (tp1->type==bt_double)
                                    nt = en_fge;
                                else if( tp1->isUnsigned )
                                        nt = en_uge;
                                else
                                        nt = en_ge;
                                break;
                        default:
                                goto fini;
                        }
                NextToken();
                tp2 = shiftop(&ep2);
                if( tp2 == 0 )
                        error(ERR_IDEXPECT);
                else    {
                        tp1 = forcefit(&ep2,tp2,&ep1,tp1);
                        ep1 = makenode(nt,ep1,ep2);
						ep1->esize = 8;
						PromoteConstFlag(ep1);
                        }
                }
fini:   *node = ep1;
xit:
     if (*node)
		(*node)->tp = tp1;
        Leave("Relation",0);
        return tp1;
}

/*
 *      equalops handles the equality and inequality operators.
 */
TYP     *equalops(ENODE **node)
{
	ENODE    *ep1, *ep2;
    TYP             *tp1, *tp2;
    int             oper;
    Enter("EqualOps");
    *node = (ENODE *)NULL;
    tp1 = relation(&ep1);
    if( tp1 == (TYP *)NULL )
        goto xit;
    while( lastst == eq || lastst == neq ) {
        oper = (lastst == eq);
        NextToken();
        tp2 = relation(&ep2);
        if( tp2 == NULL )
                error(ERR_IDEXPECT);
        else {
            tp1 = forcefit(&ep2,tp2,&ep1,tp1);
            if (tp1->type==bt_double)
                ep1 = makenode( oper ? en_feq : en_fne,ep1,ep2);
            else
                ep1 = makenode( oper ? en_eq : en_ne,ep1,ep2);
			ep1->esize = 8;
			ep1->etype = tp1->type;
            PromoteConstFlag(ep1);
        }
	}
    *node = ep1;
 xit:
     if (*node)
     	(*node)->tp = tp1;
    Leave("EqualOps",0);
    return tp1;
}

/*
 *      binop is a common routine to handle all of the legwork and
 *      error checking for bitand, bitor, bitxor, andop, and orop.
 */
TYP *binop(ENODE **node, TYP *(*xfunc)(ENODE **),int nt, int sy)
{
	ENODE    *ep1, *ep2;
        TYP             *tp1, *tp2;
        Enter("Binop");
        *node = (ENODE *)NULL;
        tp1 = (*xfunc)(&ep1);
        if( tp1 == 0 )
            goto xit;
        while( lastst == sy ) {
                NextToken();
                tp2 = (*xfunc)(&ep2);
                if( tp2 == 0 )
                        error(ERR_IDEXPECT);
                else    {
                        tp1 = forcefit(&ep2,tp2,&ep1,tp1);
                        ep1 = makenode(nt,ep1,ep2);
						ep1->esize = tp1->size;
						ep1->etype = tp1->type;
		                PromoteConstFlag(ep1);
                        }
                }
        *node = ep1;
xit:
     if (*node)
		(*node)->tp = tp1;
        Leave("Binop",0);
        return tp1;
}

TYP *bitwiseand(ENODE **node)
/*
 *      the bitwise and operator...
 */
{       return binop(node,equalops,en_and,bitandd);
}

TYP     *bitwisexor(ENODE **node)
{       return binop(node,bitwiseand,en_xor,uparrow);
}

TYP     *bitwiseor(ENODE **node)
{       return binop(node,bitwisexor,en_or,bitorr);
}

TYP     *andop(ENODE **node)
{       return binop(node,bitwiseor,en_land,land);
}

TYP *orop(ENODE **node)
{
	return binop(node,andop,en_lor,lor);
}

/*
 *      this routine processes the hook operator.
 */
TYP *conditional(ENODE **node)
{
	TYP             *tp1, *tp2, *tp3;
    ENODE    *ep1, *ep2, *ep3;
    Enter("Conditional");
    *node = (ENODE *)NULL;
    tp1 = orop(&ep1);       /* get condition */
    if( tp1 == (TYP *)NULL )
        goto xit;
    if( lastst == hook ) {
			iflevel++;
            NextToken();
            if( (tp2 = conditional(&ep2)) == NULL) {
                    error(ERR_IDEXPECT);
                    goto cexit;
                    }
            needpunc(colon);
            if( (tp3 = conditional(&ep3)) == NULL) {
                    error(ERR_IDEXPECT);
                    goto cexit;
                    }
            tp1 = forcefit(&ep3,tp3,&ep2,tp2);
            ep2 = makenode(en_void,ep2,ep3);
			ep2->esize = tp1->size;
            ep1 = makenode(en_cond,ep1,ep2);
			ep1->esize = tp1->size;
			iflevel--;
            }
cexit:  *node = ep1;
xit:
     if (*node)
     	(*node)->tp = tp1;
    Leave("Conditional",0);
    return tp1;
}

// ----------------------------------------------------------------------------
//      asnop handles the assignment operators. currently only the
//      simple assignment is implemented.
// ----------------------------------------------------------------------------
TYP *asnop(ENODE **node)
{      
	ENODE    *ep1, *ep2, *ep3;
    TYP             *tp1, *tp2;
    int             op;

    Enter("Assignop");
    *node = (ENODE *)NULL;
    tp1 = conditional(&ep1);
    if( tp1 == 0 )
        goto xit;
    for(;;) {
        switch( lastst ) {
            case assign:
                op = en_assign;
ascomm:         NextToken();
                tp2 = asnop(&ep2);
ascomm2:        if( tp2 == 0 || !IsLValue(ep1) )
                    error(ERR_LVALUE);
				else {
					tp1 = forcefit(&ep2,tp2,&ep1,tp1);
					ep1 = makenode(op,ep1,ep2);
					ep1->esize = tp1->size;
					ep1->etype = tp1->type;
					ep1->isUnsigned = tp1->isUnsigned;
					// Struct assign calls memcpy, so function is no
					// longer a leaf routine.
					if (tp1->size > 8)
						currentFn->IsLeaf = FALSE;
				}
				break;
			case asplus:
				op = en_asadd;
ascomm3:        tp2 = asnop(&ep2);
				if( tp1->type == bt_pointer ) {
					ep3 = makeinode(en_icon,tp1->btp->size);
					ep3->esize = 8;
					ep2 = makenode(en_mul,ep2,ep3);
					ep2->esize = 8;
				}
				goto ascomm;
			case asminus:
				op = en_assub;
				goto ascomm3;
			case astimes:
                
				if (tp1->isUnsigned)
					op = en_asmulu;
				else
					op = en_asmul;
				goto ascomm;
			case asdivide:
				if (tp1->isUnsigned)
					op = en_asdivu;
				else
					op = en_asdiv;
				goto ascomm;
			case asmodop:
				if (tp1->isUnsigned)
					op = en_asmodu;
				else
					op = en_asmod;
				goto ascomm;
			case aslshift:
				op = en_aslsh;
				goto ascomm;
			case asrshift:
				if (tp1->isUnsigned)
					op = en_asrshu;
				else
					op = en_asrsh;
				goto ascomm;
			case asand:
				op = en_asand;
				goto ascomm;
			case asor:
				op = en_asor;
				goto ascomm;
			case asxor:
				op = en_asxor;
				goto ascomm;
			default:
				goto asexit;
			}
	}
asexit: *node = ep1;
xit:
     if (*node)
     	(*node)->tp = tp1;
    Leave("Assignop",0);
        return tp1;
}

// ----------------------------------------------------------------------------
// Evaluate an expression where the comma operator is not legal.
// Externally visible entry point for GetIntegerExpression() and
// ArgumentList().
// ----------------------------------------------------------------------------
TYP *NonCommaExpression(ENODE **node)
{
	TYP *tp;
	Enter("NonCommaExpression");
    *node = (ENODE *)NULL;
    tp = asnop(node);
    if( tp == (TYP *)NULL )
        *node =(ENODE *)NULL;
    Leave("NonCommaExpression",tp ? tp->type : 0);
     if (*node)
     	(*node)->tp = tp;
    return tp;
}

/*
 *      evaluate the comma operator. comma operators are kept as
 *      void nodes.
 */
TYP *commaop(ENODE **node)
{
	TYP *tp1,*tp2;
	ENODE *ep1,*ep2;

    *node = (ENODE *)NULL;
	tp1 = NonCommaExpression(&ep1);
	if (tp1==(TYP *)NULL)
		return (TYP *)NULL;
	while(1) {
		if (lastst==comma) {
			NextToken();
			tp2 = NonCommaExpression(&ep2);
            ep1 = makenode(en_void,ep1,ep2);
			ep1->esize = tp1->size;
		}
		else
			break;
	}
	*node = ep1;
     if (*node)
     	(*node)->tp = tp1;
	return tp1;
}

//TYP *commaop(ENODE **node)
//{  
//	TYP             *tp1;
//        ENODE    *ep1, *ep2;
//        tp1 = asnop(&ep1);
//        if( tp1 == NULL )
//                return NULL;
//        if( lastst == comma ) {
//				NextToken();
//                tp1 = commaop(&ep2);
//                if( tp1 == NULL ) {
//                        error(ERR_IDEXPECT);
//                        goto coexit;
//                        }
//                ep1 = makenode(en_void,ep1,ep2);
//                }
//coexit: *node = ep1;
//        return tp1;
//}

// ----------------------------------------------------------------------------
// Evaluate an expression where all operators are legal.
// ----------------------------------------------------------------------------
TYP *expression(ENODE **node)
{
	TYP *tp;
	Enter("expression");
    *node = (ENODE *)NULL;
    tp = commaop(node);
    if( tp == (TYP *)NULL )
        *node = (ENODE *)NULL;
    TRACE(printf("leave exp\r\n"));
    if (tp) {
       if (*node)
          (*node)->tp = tp;
        Leave("Expression",tp->type);
    }
    else
    Leave("Expression",0);
    return tp;
}
