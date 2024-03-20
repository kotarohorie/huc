/*	File primary.c: 2.4 (84/11/27,16:26:07) */
/*% cc -O -c %
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "defs.h"
#include "data.h"
#include "enum.h"
#include "error.h"
#include "expr.h"
#include "gen.h"
#include "io.h"
#include "lex.h"
#include "primary.h"
#include "sym.h"
#include "struct.h"

extern char current_fn[];

int match_type (struct type *t, int do_ptr, int allow_unk_compound)
{
	char n[NAMESIZE];
	int have_sign = 0;
	int sflag;
	int i;
	static int anon_struct_cnt = 0;

	t->type = 0;
	t->flags = 0;
	t->otag = -1;

	for (i = 0; i < typedef_ptr; i++) {
		if (amatch(typedefs[i].sname, strlen(typedefs[i].sname))) {
			*t = typedefs[i];
			goto ret_do_ptr;
		}
	}

	if (amatch("register", 8))
		t->flags |= F_REGISTER;
	if (amatch("volatile", 8))
		t->flags |= F_VOLATILE;
	if (amatch("const", 5))
		t->flags |= F_CONST;
	if (amatch("C_DATA_BANK", 11))
		t->flags |= F_CDB;

	if ((sflag = amatch("struct", 6)) || amatch("union", 5)) {
		/* compound */
		if (symname(n)) {
			t->otag = find_tag(n);
			if (t->otag < 0 && !allow_unk_compound) {
				error("unknown struct name");
				junk();
				return (0);
			}
			t->type = CSTRUCT;
			if (sflag)
				t->flags |= F_STRUCT;
			strcpy(t->sname, n);
		}
		else {
			blanks();
			if (ch() == '{') {
				sprintf(t->sname, "__anon_struct%d", anon_struct_cnt++);
				t->otag = define_struct(t->sname, DEFAUTO, sflag);
				t->type = CSTRUCT;
				if (sflag)
					t->flags |= F_STRUCT;
			}
			else {
				error("illegal struct name");
				junk();
				return (0);
			}
		}
	}
	else if (amatch("enum", 4)) {
		t->type = CENUM;
		if (symname(n)) {
			/* This may or may not find an enum type, but if
			   it doesn't it's not necessarily an error. */
			t->otag = find_enum_type(n);
			strcpy(t->sname, n);
		}
		else {
			blanks();
			if (ch() == '{') {
				/* anonymous enum */
				t->otag = define_enum(NULL, DEFAUTO);
				t->sname[0] = 0;
			}
			else {
				illname();
				junk();
				return (0);
			}
		}
	}
	else {
		/* scalar */
		if (amatch("unsigned", 8)) {
			t->type |= CUNSIGNED;
			have_sign = 1;
		}
		else if (amatch("signed", 6))
			have_sign = 1;

		if (amatch("char", 4)) {
			t->type |= CCHAR;
			if ((have_sign == 0) && (user_signed_char == 0))
				t->type |= CUNSIGNED;
		}
		else if (amatch("int", 3))
			t->type |= CINT;
		else if (amatch("short", 5)) {
			amatch("int", 3);
			t->type |= CINT;
		}
		else if (amatch("void", 4)) {
			if (have_sign)
				goto invalid_cast;
			t->type |= CVOID;
		}
		else {
			if (have_sign)
				t->type |= CINT;
			else	/* not a cast */
				return (0);
		}
	}

	t->ident = VARIABLE;
	t->ptr_order = 0;

ret_do_ptr:
	if (do_ptr)
		while (match("*")) {
			t->ident = POINTER;
			t->ptr_order++;
		}

	return (1);

invalid_cast:
	error("invalid type cast");
	return (0);
}

intptr_t primary (LVALUE *lval, int comma)
{
	SYMBOL *ptr;
	char sname[NAMESIZE];
	intptr_t num[1];
	intptr_t k;

	lval->ptr_type = 0;	/* clear pointer/array type */
	lval->ptr_order = 0;
	lval->symbol2 = 0;
	if (match("(")) {
		struct type t;
		if (match_type(&t, YES, NO)) {
			needbrack(")");
			k = heir10(lval, comma);
			if (k)
				rvalue(lval);
			if (t.ident != POINTER) {
				gcast(t.type);
				lval->ptr_type = 0;
			}
			else
				lval->ptr_type = t.type;
			lval->type = t.type;
			lval->ptr_order = t.ptr_order;
			return (0);
		}
		else {
			indflg = 0;
			/* need to use expression_ex() (not heir1()), otherwise
			   the comma operator is not handled */
			k = expression_ex(lval, comma, YES);
			needbrack(")");
			return (k);
		}
	}
	if (amatch("sizeof", 6)) {
		int have_paren;
		struct type t;
		indflg = 0;
		have_paren = match("(");
		if (match_type(&t, YES, NO)) {
			if (t.ident == POINTER)
				immed(T_VALUE, INTSIZE);
			else if (t.type == CSTRUCT)
				immed(T_VALUE, tag_table[t.otag].size);
			else if (t.type == CINT || t.type == CUINT)
				immed(T_VALUE, INTSIZE);
			else if (t.type == CCHAR || t.type == CUCHAR)
				immed(T_VALUE, 1);
			else {
				error("internal error: sizeof type unknown");
				return (0);
			}
		}
		else if (symname(sname)) {
			if (!strcmp("__func__", sname) ||
			    !strcmp("__FUNCTION__", sname))
				immed(T_VALUE, strlen(current_fn) + 1);
			else if (!strcmp("__FILE__", sname))
				immed(T_VALUE, strlen(fname_copy) + 1);
			else if ((ptr = findloc(sname)) ||
				 (ptr = findglb(sname))) {
				k = ptr->size;
				immed(T_VALUE, k);
			}
			else {
				error("sizeof undeclared variable");
				immed(T_VALUE, 0);
			}
		}
		else if (readqstr())
			immed(T_VALUE, strlen(litq2));
		else
			error("sizeof only on type or variable");
		if (have_paren)
			needbrack(")");
		lval->symbol = 0;
		lval->indirect = 0;
		return (0);
	}
	if (amatch("__FUNCTION__", 12) || amatch("__func__", 8)) {
		const_str(num, current_fn);
		immed(T_STRING, num[0]);
		indflg = 0;
		lval->value = num[0];
		lval->symbol = 0;
		lval->indirect = 0;
		return (0);
	}
	else if (amatch("__FILE__", 8)) {
		const_str(num, fname_copy);
		immed(T_STRING, num[0]);
		indflg = 0;
		lval->value = num[0];
		lval->symbol = 0;
		lval->indirect = 0;
		return (0);
	}
	else if (amatch("__sei", 5) && match("(") && match(")")) {
		gsei();
		return (0);
	}
	else if (amatch("__cli", 5) && match("(") && match(")")) {
		gcli();
		return (0);
	}

	if (symname(sname)) {
		if (find_enum(sname, num)) {
			indflg = 0;
			lval->value = num[0];
			lval->symbol = 0;
			lval->indirect = 0;
			immed(T_VALUE, *num);
			return (0);
		}
		ptr = findloc(sname);
		if (ptr) {
			/* David, patched to support
			 *        local 'static' variables
			 */
			lval->symbol = ptr;
			lval->indirect = ptr->type;
			lval->tagsym = 0;
			if (ptr->type == CSTRUCT)
				lval->tagsym = &tag_table[ptr->tagidx];
			if (ptr->ident == POINTER) {
				if ((ptr->storage & ~WRITTEN) == LSTATIC)
					lval->indirect = 0;
				else {
					lval->indirect = CUINT;
					getloc(ptr);
				}
				lval->ptr_type = ptr->type;
				lval->ptr_order = ptr->ptr_order;
				return (1);
			}
			if (ptr->ident == ARRAY ||
			    (ptr->ident == VARIABLE && ptr->type == CSTRUCT)) {
				getloc(ptr);
				lval->ptr_type = ptr->type;
				lval->ptr_order = ptr->ptr_order;
//				lval->ptr_type = 0;
				if (ptr->type == CSTRUCT && ptr->ident == VARIABLE)
					return (1);
				else
					return (0);
			}
			if ((ptr->storage & ~WRITTEN) == LSTATIC)
				lval->indirect = 0;
			else
				getloc(ptr);
			return (1);
		}
		ptr = findglb(sname);
		if (ptr) {
			if (ptr->ident != FUNCTION) {
				lval->symbol = ptr;
				lval->indirect = 0;
				lval->tagsym = 0;
				if (ptr->type == CSTRUCT)
					lval->tagsym = &tag_table[ptr->tagidx];
				if (ptr->ident != ARRAY &&
				    (ptr->ident != VARIABLE || ptr->type != CSTRUCT)) {
					if (ptr->ident == POINTER) {
						lval->ptr_type = ptr->type;
						lval->ptr_order = ptr->ptr_order;
					}
					return (1);
				}
				if (!ptr->far)
					immed(T_SYMBOL, (intptr_t)ptr);
				else {
					/* special variables */
					blanks();
					if ((ch() != '[') && (ch() != '(')) {
						/* vram */
						if (strcmp(ptr->name, "vram") == 0) {
							if (indflg)
								return (1);
							else
								error("can't access vram this way");
						}
						/* others */
						immed(T_SYMBOL, (intptr_t)ptr);
//						error ("can't access far array");
					}
				}
				lval->indirect = lval->ptr_type = ptr->type;
				lval->ptr_order = ptr->ptr_order;
//				lval->ptr_type = 0;
				if (ptr->ident == VARIABLE && ptr->type == CSTRUCT)
					return (1);
				else
					return (0);
			}
		}
		blanks();
		if (ch() != '(') {
			if (ptr && (ptr->ident == FUNCTION)) {
				lval->symbol = ptr;
				lval->indirect = 0;
				immed(T_SYMBOL, (intptr_t)ptr->name);
				return (0);
			}
			error("undeclared variable");
		}
		ptr = addglb(sname, FUNCTION, CINT, 0, PUBLIC, 0);
		indflg = 0;
		lval->symbol = ptr;
		lval->indirect = 0;
		return (0);
	}
	if ((k = constant(num))) {
		indflg = 0;
		lval->value = num[0];
		lval->symbol = 0;
		lval->indirect = 0;
		if (k == 2) {
			lval->ptr_type = CCHAR;
			lval->ptr_order = 1;
		}
		return (0);
	}
	else {
		indflg = 0;
		error("invalid expression");
		immed(T_VALUE, 0);
		junk();
		return (0);
	}
}

/*
 *	true if val1 -> int pointer or int array and val2 not pointer or array
 */
intptr_t dbltest (LVALUE val1[], LVALUE val2[])
{
	if (val1 == NULL || !val1->ptr_type)
		return (FALSE);

	if (val1->ptr_type == CCHAR || val1->ptr_type == CUCHAR)
		return (FALSE);

	if (val2->ptr_type)
		return (FALSE);

	return (TRUE);
}

/*
 *	determine type of binary operation
 */
void result (LVALUE lval[], LVALUE lval2[])
{
	if (lval->ptr_type && lval2->ptr_type) {
		lval->ptr_type = 0;
		lval->ptr_order = 0;
	}
	else if (lval2->ptr_type) {
		lval->symbol = lval2->symbol;
		lval->indirect = lval2->indirect;
		lval->ptr_type = lval2->ptr_type;
		lval->ptr_order = lval2->ptr_order;
	}
}

intptr_t constant (intptr_t val[])
{
	if (number(val))
		immed(T_VALUE, val[0]);
	else if (pstr(val))
		immed(T_VALUE, val[0]);
	else if (qstr(val)) {
		immed(T_STRING, val[0]);
		return (2);
	}
	else
		return (0);

	return (1);
}

intptr_t number (intptr_t val[])
{
	intptr_t k, minus, base;
	char c;

	k = minus = 1;
	while (k) {
		k = 0;
		if (match("+"))
			k = 1;
		if (match("-")) {
			minus = (-minus);
			k = 1;
		}
	}
	if (!numeric(c = ch()))
		return (0);

	if (match("0x") || match("0X"))
		while (numeric(c = ch()) ||
		       (c >= 'a' && c <= 'f') ||
		       (c >= 'A' && c <= 'F')) {
			inbyte();
			k = k * 16 +
			    (numeric(c) ? (c - '0') : ((c & 07) + 9));
		}
	else {
		base = (c == '0') ? 8 : 10;
		while (numeric(ch())) {
			c = inbyte();
			k = k * base + (c - '0');
		}
	}
	if (minus < 0)
		k = (-k);
	val[0] = k;
	return (1);
}

static int parse0 (intptr_t *num)
{
	if (!const_expr(num, ")", NULL))
		return (0);

	if (!match(")"))
		error("internal error");
	return (1);
}

static int parse3 (intptr_t *num)
{
	intptr_t num2;
	struct type t;
	char op;
	char n[NAMESIZE];
	int have_paren = 0;

	if (match("-"))
		op = '-';
	else if (match("+"))
		op = '+';
	else if (match("~"))
		op = '~';
	else if (match("!"))
		op = '!';
	else if (match("(")) {
		if (match_type(&t, YES, NO)) {
			if (!match(")")) {
				error("invalid type cast");
				return (0);
			}
			op = 'c';
		}
		else {
			have_paren = 1;
			op = 0;
		}
	}
	else
		op = 0;

	if (!(have_paren && parse0(&num2)) &&
	    !number(&num2) &&
	    !pstr(&num2) &&
	    !(symname(n) && find_enum(n, &num2)))
		return (0);

	if (op == '-')
		*num = -num2;
	else if (op == '~')
		*num = ~num2;
	else if (op == '!')
		*num = !num2;
	else if (op == 'c') {
		if (t.ident != POINTER) {
			assert(sizeof(short) == 2);
			switch (t.type) {
			case CCHAR:
				*num = (char)num2;
				break;
			case CUCHAR:
				*num = (unsigned char)num2;
				break;
			case CINT:
				*num = (short)num2;
				break;
			case CUINT:
				*num = (unsigned short)num2;
				break;
			default:
				error("unable to handle cast in constant expression");
				return (0);
			}
		}
		else
			*num = num2;
	}
	else
		*num = num2;
	return (1);
}

static int parse5 (intptr_t *num)
{
	intptr_t num1, num2;

	if (!parse3(&num1))
		return (0);

	for (;;) {
		char op;
		if (match("*"))
			op = '*';
		else if (match("/"))
			op = '/';
		else {
			*num = num1;
			return (1);
		}

		if (!parse3(&num2))
			return (0);

		if (op == '*')
			num1 *= num2;
		else
			num1 /= num2;
	}
}

static int parse6 (intptr_t *num)
{
	intptr_t num1, num2;

	if (!parse5(&num1))
		return (0);

	for (;;) {
		char op;
		if (match("+"))
			op = '+';
		else if (match("-"))
			op = '-';
		else {
			*num = num1;
			return (1);
		}

		if (!parse5(&num2))
			return (0);

		if (op == '-')
			num1 -= num2;
		else
			num1 += num2;
	}
}

static int parse7 (intptr_t *num)
{
	intptr_t num1, num2;

	if (!parse6(&num1))
		return (0);

	for (;;) {
		char op;
		if (match("<<"))
			op = 'l';
		else if (match(">>"))
			op = 'r';
		else {
			*num = num1;
			return (1);
		}

		if (!parse6(&num2))
			return (0);

		if (op == 'l')
			num1 <<= num2;
		else
			num1 >>= num2;
	}
}

static int parse9 (intptr_t *num)
{
	intptr_t num1, num2;

	if (!parse7(&num1))
		return (0);

	for (;;) {
		char op;
		if (match("=="))
			op = '=';
		else if (match("!="))
			op = '!';
		else {
			*num = num1;
			return (1);
		}

		if (!parse7(&num2))
			return (0);

		if (op == '=')
			num1 = num1 == num2;
		else
			num1 = num1 != num2;
	}
}

int const_expr (intptr_t *num, char *end1, char *end2)
{
	if (!parse9(num)) {
		error("failed to evaluate constant expression");
		return (0);
	}
	blanks();
	if (end1 && !sstreq(end1) && !(end2 && sstreq(end2))) {
		error("unexpected character after constant expression");
		return (0);
	}
	return (1);
}

/*
 *         pstr
 * pstr parses a character than can eventually be 'double' i.e. like 'a9'
 * returns 0 in case of failure else 1
 */
intptr_t pstr (intptr_t val[])
{
	intptr_t k;
	char c;

	k = 0;
	if (!match("'"))
		return (0);

	while ((c = gch()) != '\'') {
		c = (c == '\\') ? spechar() : c;
		k = (k & 255) * 256 + (c & 255);
	}
	val[0] = k;
	return (1);
}

/*
 *         qstr
 * qstr parses a double quoted string into litq
 * return 0 in case of failure and 1 else
 */
intptr_t qstr (intptr_t val[])
{
	char c;

	if (!match(quote))
		return (0);

	val[0] = litptr;
	while (ch() != '"') {
		if (ch() == 0)
			break;
		if (litptr >= LITMAX) {
			error("string space exhausted");
			while (!match(quote))
				if (gch() == 0)
					break;
			return (1);
		}
		c = gch();
		litq[litptr++] = (c == '\\') ? spechar() : c;
	}
	gch();
	litq[litptr++] = 0;
	return (1);
}

intptr_t const_str (intptr_t *val, const char *str)
{
	if (litptr + strlen(str) + 1 >= LITMAX) {
		error("string space exhausted");
		return (1);
	}
	strcpy(&litq[litptr], str);
	*val = litptr;
	litptr += strlen(str) + 1;
	return (1);
}

/*
 *         readqstr
 * readqstr parses a double quoted string into litq2
 * return 0 in case of failure and 1 else
 * Zeograd: this function don't dump the result of the reading in the literal
 * pool, it is rather intended for use in pseudo code
 */
intptr_t readqstr (void)
{
	char c;
	intptr_t posptr = 0;

	if (!match(quote))
		return (0);

	while (ch() != '"') {
		if (ch() == 0)
			break;
		if (posptr >= LITMAX2) {
			error("string space exhausted");
			while (!match(quote))
				if (gch() == 0)
					break;
			return (1);
		}
		c = gch();
		litq2[posptr++] = (c == '\\') ? spechar() : c;
	}
	gch();
	litq2[posptr] = 0;
	return (1);
}

/*
 *         readstr
 * reaqstr parses a string into litq2
 * it only read alpha numeric characters
 * return 0 in case of failure and 1 else
 * Zeograd: this function don't dump the result of the reading in the literal
 * pool, it is rather intended for use in pseudo code
 */
intptr_t readstr (void)
{
	char c;
	intptr_t posptr = 0;

	while (an(ch()) || (ch() == '_')) {
		if (ch() == 0)
			break;
		if (posptr >= LITMAX2) {
			error("string space exhausted");
			return (1);
		}
		c = gch();
		litq2[posptr++] = (c == '\\') ? spechar() : c;
	}
	litq2[posptr] = 0;
	return (1);
}


/*
 *	decode special characters (preceeded by back slashes)
 */
intptr_t spechar (void)
{
	char c;

	c = ch();

	if (c == 'n') c = EOL;
	else if (c == 't') c = TAB;
	else if (c == 'r') c = CR;
	else if (c == 'f') c = FFEED;
	else if (c == 'b') c = BKSP;
	else if (c == '0') c = EOS;
	else if (numeric(c) && c < '8') {
		/* octal character specification */
		int n = 0;
		while (numeric(c) && c < '8') {
			n = (n << 3) | (c - '0');
			gch();
			c = ch();
		}
		return (n);
	}
	else if (c == 'x') {
		int n = 0, i;
		gch();
		for (i = 0; i < 2; i++) {
			c = gch();
			if (c >= 'a' && c <= 'f')
				c = c - 'a' + 10;
			else if (c >= 'A' && c <= 'F')
				c = c - 'A' + 10;
			else if (c >= '0' && c <= '9')
				c = c - '0';
			else {
				error("invalid hex character");
				return (0);
			}
			n = (n << 4) | c;
		}
		return (n);
	}
	else if (c == EOS) return (c);

	gch();
	return (c);
}
