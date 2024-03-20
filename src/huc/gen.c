/*	File gen.c: 2.1 (83/03/20,16:02:06) */
/*% cc -O -c %
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include "defs.h"
#include "data.h"
#include "code.h"
#include "primary.h"
#include "sym.h"
#include "gen.h"
#include "expr.h"
#include "const.h"
#include "c_data_bank.h"
#include "error.h"

/*
 *	return next available internal label number
 *
 */
intptr_t getlabel (void)
{
	return (nxtlab++);
}

/*
 *	fetch a static memory cell into the primary register
 */
void getmem (SYMBOL *sym)
{
	char *data;

	if ((sym->ident != POINTER) && (sym->type == CCHAR || sym->type == CUCHAR)) {
		int op = I_LDB;
		if (sym->type & CUNSIGNED)
			op = I_LDUB;
		if ((sym->storage & ~WRITTEN) == LSTATIC)
			out_ins(op, T_LABEL, glint(sym));
		else
			out_ins(op, T_SYMBOL, (intptr_t)(sym->name));
	}
	else {
		if ((sym->storage & ~WRITTEN) == LSTATIC)
			out_ins(I_LDW, T_LABEL, glint(sym));
		else if ((sym->storage & ~WRITTEN) == CONST && (data = get_const(sym)))
			out_ins(I_LDWI, T_LITERAL, (intptr_t)data);
		else
			out_ins(I_LDW, T_SYMBOL, (intptr_t)(sym->name));
	}
}

/*
 *	fetch a hardware register into the primary register
 */
void getio (SYMBOL *sym)
{
	out_ins(I_CALL, T_LIB, (intptr_t)"getvdc");
}
void getvram (SYMBOL *sym)
{
	out_ins(I_CALL, T_LIB, (intptr_t)"readvram");
}

/*
 *	fetch the address of the specified symbol into the primary register
 *
 */
void getloc (SYMBOL *sym)
{
	intptr_t value;

	if ((sym->storage & ~WRITTEN) == LSTATIC)
		out_ins(I_LDWI, T_LABEL, glint(sym));
	else {
		value = glint(sym);
		if (norecurse && value < 0) {
			/* XXX: bit of a memory leak, but whatever... */
			SYMBOL * locsym = copysym(sym);
			if (NAMEALLOC <=
				sprintf(locsym->name, "_%s_lend-%ld", current_fn, (long) -value))
				error("norecurse local name too intptr_t");
			out_ins(I_LDWI, T_SYMBOL, (intptr_t)locsym);
		}
		else {
			value -= stkp;
			out_ins_sym(X_LEA_S, T_STACK, value, sym);
		}
	}
}

/*
 *	store the primary register into the specified static memory cell
 *
 */
void putmem (SYMBOL *sym)
{
	intptr_t code;

	/* XXX: What about 1-byte structs? */
	if ((sym->ident != POINTER) & (sym->type == CCHAR || sym->type == CUCHAR))
		code = I_STB;
	else
		code = I_STW;

	if ((sym->storage & ~WRITTEN) == LSTATIC)
		out_ins(code, T_LABEL, glint(sym));
	else
		out_ins(code, T_SYMBOL, (intptr_t)(sym->name));
}

/*
 *	store the specified object type in the primary register
 *	at the address on the top of the stack
 *
 */
void putstk (char typeobj)
{
	if (typeobj == CCHAR || typeobj == CUCHAR)
		out_ins(I_STBPS, (intptr_t)NULL, (intptr_t)NULL);
	else
		out_ins(I_STWPS, (intptr_t)NULL, (intptr_t)NULL);
	stkp = stkp + INTSIZE;
}

/*
 *	store the primary register
 *	at the address on the top of the stack
 *
 */
void putio (SYMBOL *sym)
{
	out_ins(I_JSR, T_LIB, (intptr_t)"setvdc");
	stkp = stkp + INTSIZE;
}
void putvram (SYMBOL *sym)
{
	out_ins(I_JSR, T_LIB, (intptr_t)"writevram");
	stkp = stkp + INTSIZE;
}

/*
 *	fetch the specified object type indirect through the primary
 *	register into the primary register
 *
 */
void indirect (char typeobj)
{
	out_ins(I_STW, T_PTR, (intptr_t)NULL);
	if (typeobj == CCHAR)
		out_ins(I_LDBP, T_PTR, (intptr_t)NULL);
	else if (typeobj == CUCHAR)
		out_ins(I_LDUBP, T_PTR, (intptr_t)NULL);
	else
		out_ins(I_LDWP, T_PTR, (intptr_t)NULL);
}

void farpeek (SYMBOL *ptr)
{
	if (ptr->type == CCHAR)
		out_ins(I_FGETB, T_SYMBOL, (intptr_t)ptr);
	else if (ptr->type == CUCHAR)
		out_ins(I_FGETUB, T_SYMBOL, (intptr_t)ptr);
	else
		out_ins(I_FGETW, T_SYMBOL, (intptr_t)ptr);
}

/*
 *	print partial instruction to get an immediate value into
 *	the primary register
 *
 */
void immed (intptr_t type, intptr_t data)
{
	if (type == T_VALUE && (data < -32768 || data > 65535))
		warning(W_GENERAL, "large integer truncated");
	out_ins(I_LDWI, type, data);
}

/*
 *	push the primary register onto the stack
 *
 */
void gpush (void)
{
//	out_ins(I_PUSHWZ, T_VALUE, zpstkp);
//	zpstkp = zpstkp - INTSIZE;

	out_ins(I_PUSHW, T_VALUE, INTSIZE);
	stkp = stkp - INTSIZE;
}

/*
 *	push the primary register onto the stack
 *
 */
void gpusharg (intptr_t size)
{
	out_ins(I_PUSHW, T_SIZE, size);
	stkp = stkp - size;
}

/*
 *	pop the top of the stack into the secondary register
 *
 */
void gpop (void)
{
	out_ins(I_POPW, (intptr_t)NULL, (intptr_t)NULL);
	stkp = stkp + INTSIZE;
}

/*
 *	swap the primary register and the top of the stack
 *
 */
void swapstk (void)
{
	out_ins(I_SWAPW, (intptr_t)NULL, (intptr_t)NULL);
}

/*
 *	call the specified subroutine name
 *
 */
void gcall (char *sname, intptr_t nargs)
{
	if (need_map_call_bank)
		out_ins_ex(I_MAPCBANK, T_SYMBOL, (intptr_t)sname, T_VALUE, nargs);
	out_ins_ex(I_CALL, T_SYMBOL, (intptr_t)sname, T_VALUE, nargs);
	if (need_map_call_bank)
		out_ins_ex(I_UNMAPCBANK, T_SYMBOL, (intptr_t)sname, T_VALUE, nargs);
}

/*
 *         generate a bank pseudo instruction
 *
 */
void gbank (unsigned char bank, unsigned short offset)
{
	out_ins(I_BANK, T_VALUE, bank);
	out_ins(I_OFFSET, T_VALUE, offset);
}

/*
 *	return from subroutine
 *
 */
void gret (void)
{
	out_ins(I_RTS, (intptr_t)NULL, (intptr_t)NULL);
}

/*
 *	perform subroutine call to value on top of stack
 *
 */
void callstk (intptr_t nargs)
{
	if (nargs <= INTSIZE)
		out_ins(I_CALLS, T_STACK, 0);
	else
		out_ins(I_CALLS, T_STACK, nargs - INTSIZE);

	stkp = stkp + INTSIZE;
}

/*
 *	jump to specified internal label number
 *
 */
void jump (intptr_t label)
{
	out_ins(I_LBRA, T_LABEL, label);
}

/*
 *	test the primary register and jump if false to label
 *
 */
void testjump (intptr_t label, intptr_t ft)
{
	out_ins(I_TSTW, (intptr_t)NULL, (intptr_t)NULL);
	if (ft)
		out_ins(I_LBNE, T_LABEL, label);
	else
		out_ins(I_LBEQ, T_LABEL, label);
}

/*
 *	modify the stack pointer to the new value indicated
 *      Is it there that we decrease the value of the stack to add local vars ?
 */
intptr_t modstk (intptr_t newstkp)
{
	intptr_t k;

//	k = galign(newstkp - stkp);
	k = newstkp - stkp;
	if (k) {
		gtext();
		out_ins(I_ADDMI, T_STACK, k);
	}
	return (newstkp);
}

/*
 *	multiply the primary register by INTSIZE
 */
void gaslint (void)
{
	out_ins(I_ASLW, (intptr_t)NULL, (intptr_t)NULL);
}

/*
 *	divide the primary register by INTSIZE
 */
void gasrint (void)
{
	out_ins(I_ASRW, (intptr_t)NULL, (intptr_t)NULL);
}

/*
 *	Case jump instruction
 */
void gjcase (void)
{
	out_ins(I_JMP, T_SYMBOL, (intptr_t)"__case");
}

/*
 *	add the primary and secondary registers
 *	if lval2 is int pointer and lval is int, scale lval
 */
void gadd (LVALUE *lval, LVALUE *lval2)
{
	/* XXX: isn't this done in expr.c already? */
	if (dbltest(lval2, lval))
		out_ins(I_ASLWS, (intptr_t)NULL, (intptr_t)NULL);
	if (lval && lval2 && is_byte(lval) && is_byte(lval2))
		out_ins(I_ADDBS, (intptr_t)NULL, (intptr_t)NULL);
	else
		out_ins(I_ADDWS, (intptr_t)NULL, (intptr_t)NULL);
	stkp = stkp + INTSIZE;
}

/*
 *	subtract the primary register from the secondary
 *
 */
void gsub (void)
{
	out_ins(I_SUBWS, (intptr_t)NULL, (intptr_t)NULL);
	stkp = stkp + INTSIZE;
}

/*
 *	multiply the primary and secondary registers
 *	(result in primary)
 *
 */
void gmult (int is_unsigned)
{
	if (is_unsigned)
		out_ins(I_JSR, T_LIB, (intptr_t)"umul");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"smul");
	stkp = stkp + INTSIZE;
}

void gmult_imm (int value)
{
	out_ins(I_MULWI, T_VALUE, (intptr_t)value);
}

/*
 *	divide the secondary register by the primary
 *	(quotient in primary, remainder in secondary)
 *
 */
void gdiv (int is_unsigned)
{
	if (is_unsigned)
		out_ins(I_JSR, T_LIB, (intptr_t)"udiv");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"sdiv");
	stkp = stkp + INTSIZE;
}

void gdiv_imm (int value)
{
	gpush();
	immed(T_VALUE, value);
	gdiv(1);
}

/*
 *	compute the remainder (mod) of the secondary register
 *	divided by the primary register
 *	(remainder in primary, quotient in secondary)
 *
 */
void gmod (int is_unsigned)
{
	if (is_unsigned)
		out_ins(I_JSR, T_LIB, (intptr_t)"umod");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"smod");
	stkp = stkp + INTSIZE;
}

/*
 *	inclusive 'or' the primary and secondary registers
 *
 */
void gor (void)
{
	out_ins(I_ORWS, (intptr_t)NULL, (intptr_t)NULL);
	stkp = stkp + INTSIZE;
}

/*
 *	exclusive 'or' the primary and secondary registers
 *
 */
void gxor (void)
{
	out_ins(I_EORWS, (intptr_t)NULL, (intptr_t)NULL);
	stkp = stkp + INTSIZE;
}

/*
 *	'and' the primary and secondary registers
 *
 */
void gand (void)
{
	out_ins(I_ANDWS, (intptr_t)NULL, (intptr_t)NULL);
	stkp = stkp + INTSIZE;
}

/*
 *	arithmetic shift right the secondary register the number of
 *	times in the primary register
 *	(results in primary register)
 *
 */
void gasr (int is_unsigned)
{
	if (is_unsigned)
		out_ins(I_JSR, T_LIB, (intptr_t)"lsr");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"asr");
	stkp = stkp + INTSIZE;
}

/*
 *	arithmetic shift left the secondary register the number of
 *	times in the primary register
 *	(results in primary register)
 *
 */
void gasl (void)
{
	out_ins(I_JSR, T_LIB, (intptr_t)"asl");
	stkp = stkp + INTSIZE;
}

/*
 *	two's complement of primary register
 *
 */
void gneg (void)
{
	out_ins(I_NEGW, (intptr_t)NULL, (intptr_t)NULL);
}

/*
 *	one's complement of primary register
 *
 */
void gcom (void)
{
	out_ins(I_COMW, (intptr_t)NULL, (intptr_t)NULL);
}

/*
 *	convert primary register into logical value
 *
 */
void gbool (void)
{
	out_ins(I_BOOLW, (intptr_t)NULL, (intptr_t)NULL);
}

/*
 *	logical complement of primary register
 *
 */
void glneg (void)
{
	out_ins(I_NOTW, (intptr_t)NULL, (intptr_t)NULL);
}

/*
 *	increment the primary register by 1 if char, INTSIZE if
 *      intptr_t
 */
void ginc (LVALUE *lval)
{
	SYMBOL *sym = lval->symbol;

	if (lval->ptr_type == CINT || lval->ptr_type == CUINT ||
	    (sym && (sym->ptr_order > 1 || (sym->ident == ARRAY && sym->ptr_order > 0))))
		out_ins(I_ADDWI, T_VALUE, 2);
	else if (lval->ptr_type == CSTRUCT)
		out_ins(I_ADDWI, T_VALUE, lval->tagsym->size);
	else
		out_ins(I_ADDWI, T_VALUE, 1);
}

/*
 *	decrement the primary register by one if char, INTSIZE if
 *	intptr_t
 */
void gdec (LVALUE *lval)
{
	SYMBOL *sym = lval->symbol;

	if (lval->ptr_type == CINT || lval->ptr_type == CUINT ||
	    (sym && (sym->ptr_order > 1 || (sym->ident == ARRAY && sym->ptr_order > 0))))
		out_ins(I_SUBWI, T_VALUE, 2);
	else if (lval->ptr_type == CSTRUCT)
		out_ins(I_SUBWI, T_VALUE, lval->tagsym->size);
	else
		out_ins(I_SUBWI, T_VALUE, 1);
}

/*
 *	following are the conditional operators.
 *	they compare the secondary register against the primary register
 *	and put a literl 1 in the primary if the condition is true,
 *	otherwise they clear the primary register
 *
 */

/*
 *	equal
 *
 */
void geq (int is_byte)
{
	if (is_byte)
		out_ins(I_JSR, T_LIB, (intptr_t)"eqb");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"eq");
	stkp = stkp + INTSIZE;
}

/*
 *	not equal
 *
 */
void gne (int is_byte)
{
	if (is_byte)
		out_ins(I_JSR, T_LIB, (intptr_t)"neb");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"ne");
	stkp = stkp + INTSIZE;
}

/*
 *	less than (signed)
 *
 */
/*void glt (intptr_t lvl) */
void glt (int is_byte)
{
	if (is_byte)
		out_ins(I_JSR, T_LIB, (intptr_t)"ltb");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"lt");
	stkp = stkp + INTSIZE;
}

/*
 *	less than or equal (signed)
 *
 */
void gle (int is_byte)
{
	if (is_byte)
		out_ins(I_JSR, T_LIB, (intptr_t)"leb");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"le");
	stkp = stkp + INTSIZE;
}

/*
 *	greater than (signed)
 *
 */
void ggt (int is_byte)
{
	if (is_byte)
		out_ins(I_JSR, T_LIB, (intptr_t)"gtb");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"gt");
	stkp = stkp + INTSIZE;
}

/*
 *	greater than or equal (signed)
 *
 */
void gge (int is_byte)
{
	if (is_byte)
		out_ins(I_JSR, T_LIB, (intptr_t)"geb");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"ge");
	stkp = stkp + INTSIZE;
}

/*
 *	less than (unsigned)
 *
 */
/*void gult (intptr_t lvl)*/
void gult (int is_byte)
{
	if (is_byte)
		out_ins(I_JSR, T_LIB, (intptr_t)"ublt");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"ult");
	stkp = stkp + INTSIZE;
}

/*
 *	less than or equal (unsigned)
 *
 */
void gule (int is_byte)
{
	if (is_byte)
		out_ins(I_JSR, T_LIB, (intptr_t)"uble");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"ule");
	stkp = stkp + INTSIZE;
}

/*
 *	greater than (unsigned)
 *
 */
void gugt (int is_byte)
{
	if (is_byte)
		out_ins(I_JSR, T_LIB, (intptr_t)"ubgt");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"ugt");
	stkp = stkp + INTSIZE;
}

/*
 *	greater than or equal (unsigned)
 *
 */
void guge (int is_byte)
{
	if (is_byte)
		out_ins(I_JSR, T_LIB, (intptr_t)"ubge");
	else
		out_ins(I_JSR, T_LIB, (intptr_t)"uge");
	stkp = stkp + INTSIZE;
}

void scale_const (int type, int otag, intptr_t *size)
{
	switch (type) {
	case CINT:
	case CUINT:
		*size += *size;
		break;
	case CSTRUCT:
		*size *= tag_table[otag].size;
		break;
	default:
		break;
	}
}

void gcast (int type)
{
	switch (type) {
	case CCHAR:
		out_ins(I_EXTW, 0, 0);
		break;
	case CUCHAR:
		out_ins(I_EXTUW, 0, 0);
		break;
	case CINT:
	case CUINT:
	case CVOID:
		break;
	default:
		abort();
	}
	;
}

void gsei (void)
{
	out_ins(I_SEI, 0, 0);
}
void gcli (void)
{
	out_ins(I_CLI, 0, 0);
}
