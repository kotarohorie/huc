/*	File c_data_bank.c: 2.1 (00/07/17,16:02:19) */
/*% cc -O -c %
 *
 */

/* XXX: This passes the initializer more or less verbatim to the assembler,
   which unsurprisingly does not care much for C semantics, breaking stuff
   like pointer arithmetic, and probably many non-trivial expressions.
   Needs a rewrite. */

#include <stdint.h>
#include <stdio.h>
#include "defs.h"
#include "data.h"
#include "code.h"
#include "const.h"
#include "c_data_bank.h"
#include "error.h"
#include "io.h"
#include "lex.h"
#include "primary.h"
#include "sym.h"

/*
 *	setup a new c_data_bank array
 *
 */
void new_c_data_bank (void)
{
	c_data_bank_ptr = &c_data_bank_var[c_data_bank_nb];
	c_data_bank_val_idx = c_data_bank_val_start;
	c_data_bank_data_idx = c_data_bank_data_start;
}


/*
 *	add a c_data_bank array
 *
 */
void add_c_data_bank (intptr_t typ)
{
	if ((c_data_bank_data_idx >= MAX_C_DATA_BANK_DATA) || (c_data_bank_val_idx >= MAX_C_DATA_BANK_VALUE))
		error("too much clang DATA_BANK data (> 16KB)");
	if (c_data_bank_nb >= MAX_C_DATA_BANK)
		error("too much clang DATA_BANK arrays");
	else {
		c_data_bank_ptr->sym = cptr;
		c_data_bank_ptr->typ = typ;
		c_data_bank_ptr->size = c_data_bank_val_idx - c_data_bank_val_start;
		c_data_bank_ptr->data = c_data_bank_val_start;
		c_data_bank_val_start = c_data_bank_val_idx;
		c_data_bank_data_start = c_data_bank_data_idx;
		c_data_bank_nb++;
	}
}


/*
 *	array initializer
 *
 */
intptr_t array_initializer_cdb (intptr_t typ, intptr_t id, intptr_t stor)
{
	intptr_t nb;
	intptr_t k;
	intptr_t i;

	nb = 0;
	k = needsub();

	if (stor == CDB)
		new_c_data_bank();
	if (match("=")) {
		if (stor != CDB)
			error("can't initialize non-const arrays");
		if (!match("{")) {
			#if defined(DBPRN)
			error("syntax error #1");
			#else
			error("syntax error");
			#endif
			return (-1);
		}
		if (!match("}")) {
			for (;;) {
				if (match("}")) {
					error("value missing");
					break;
				}
				if (match(",")) {
					error("value missing");
					continue;
				}
				if ((ch() == '\"') && (id == POINTER))
					i = get_string_ptr_cdb(typ);
				else
					i = get_raw_value_cdb(',');
				nb++;
				blanks();
				if (c_data_bank_val_idx < MAX_C_DATA_BANK_VALUE)
					c_data_bank_val[c_data_bank_val_idx++] = i;
				if ((ch() != ',') && (ch() != '}')) {
					#if defined(DBPRN)
					error("syntax error #2");
					#else
					error("syntax error");
					#endif
					return (-1);
				}
				if (match("}"))
					break;
				gch();
			}
		}
		if (k == 0)
			k = nb;
		if (nb > k) {
			nb = k;
			error("excess elements in array initializer");
		}
	}
	if (stor == CDB) {
		while (nb < k) {
			nb++;
			if (c_data_bank_val_idx < MAX_C_DATA_BANK_VALUE)
				c_data_bank_val[c_data_bank_val_idx++] = -1;
		}
	}
	return (k);
}

/*
 *	scalar initializer
 *
 */
intptr_t scalar_initializer_cdb (intptr_t typ, intptr_t id, intptr_t stor)
{
	intptr_t i;

	if (stor == CDB)
		new_c_data_bank();
	if (match("=")) {
		if (stor != CDB)
			error("can't initialize non-const scalars");
		blanks();
		if (ch() == ';') {
			error("value missing");
			return (-1);
		}
		if (ch() == '\"' && id == POINTER)
			i = get_string_ptr_cdb(typ);
		else
			i = get_raw_value_cdb(';');
		if (c_data_bank_val_idx < MAX_C_DATA_BANK_VALUE)
			c_data_bank_val[c_data_bank_val_idx++] = i;
		blanks();
		if (ch() != ';') {
			#if defined(DBPRN)
			error("syntax error #3");
			#else
			error("syntax error");
			#endif
			return (-1);
		}
	}
	return (1);
}


/*
 *  add a string to the literal pool and return a pointer (index) to it
 *
 */
intptr_t get_string_ptr_cdb (intptr_t typ)
{
	intptr_t num[1];

	if (typ == CINT || typ == CUINT)
		error("incompatible pointer type");
	if (qstr(num))
		return (-(num[0] + 1024));
	else
		return (-1);
}


/*
 *  get value raw text
 *
 */
intptr_t get_raw_value_cdb (char sep)
{
	char c;
	char tmp[LINESIZE + 1];
	char *ptr;
	intptr_t level;
	intptr_t flag;
	intptr_t start;
	int is_address = 0;
	int had_address = 0;

	flag = 0;
	level = 0;
	start = c_data_bank_data_idx;
	ptr = tmp;

	for (;;) {
		c = ch();

		/* discard blanks */
		if ((c == ' ') || (c == '\t')) {
			gch();
			continue;
		}
		/* string */
		if (c == '\"') {
			for (;;) {
				gch();

				/* add char */
				if (c_data_bank_data_idx < MAX_C_DATA_BANK_DATA)
					c_data_bank_data[c_data_bank_data_idx++] = c;

				/* next */
				c = ch();

				if ((c == 0) || (c == '\"'))
					break;
			}
		}
		/* parenthesis */
		if (c == '(')
			level++;
		else if (c == ')')
			level--;
		/* comma separator */
		else if (c == sep) {
			if (level == 0)
				break;
		}
		/* end */
		else if ((c == 0) || (c == '}')) {
			if (level)
			{
				#if defined(DBPRN)
				error("syntax error #4");
				#else
				error("syntax error");
				#endif
			}
			break;
		}

		/* parse */
		if (an(c)) {
			flag = 1;
			*ptr++ = c;
		}
		else {
			/* add buffer */
			if (flag) {
				flag = 0;
				*ptr = '\0';
				ptr = tmp;
				had_address += add_buffer_cdb(tmp, c, is_address);
				is_address = 0;
				if ((c == '+' || c == '-') && had_address) {
					/* Initializers are passed almost
					   verbatim to the assembler, which
					   doesn't know anything about types
					   and thus doesn't know how to do
					   pointer arithmetic correctly, so
					   we don't allow it. */
					error("pointer arithmetic in initializers not supported");
					return (0);
				}
			}

			/* add char */
			if (c == '&') {
				/* we want the succeeding identifier's address */
				is_address = 1;
				/* we need to remember that we had an address
				   somewhere so we can barf if the identifier
				   contains arithmetic */
				had_address = 1;
			}
			else if (c_data_bank_data_idx < MAX_C_DATA_BANK_DATA)
				c_data_bank_data[c_data_bank_data_idx++] = c;
		}
		gch();
	}
	/* add buffer */
	if (flag) {
		*ptr = '\0';
		add_buffer_cdb(tmp, c, is_address);
	}
	/* close string */
	if (c_data_bank_data_idx < MAX_C_DATA_BANK_DATA)
		c_data_bank_data[c_data_bank_data_idx++] = '\0';

	return (start);
}


/*
 *	add a string to the c_data_bank pool
 *  handle underscore
 *
 */
int add_buffer_cdb (char *p, char c, int is_address)
{
	SYMBOL *s = 0;

	/* underscore */
	if (alpha(*p)) {
		if (!is_address) {
			s = findglb(p);
			if (!s) {
				error("undefined global");
				return (0);
			}
			/* Unless preceded by an address operator, we
			   need to get the value, and it better be
			   constant... */
			p = get_c_data_bank(s);
			if (!p) {
				error("non-constant initializer");
				return (0);
			}
		}
		else if (c != '(') {
			/* If we want the address, we need an underscore
			   prefix. */
			if (c_data_bank_data_idx < MAX_C_DATA_BANK_DATA)
				c_data_bank_data[c_data_bank_data_idx++] = '_';
		}
	}

	/* string */
	while (*p) {
		if (c_data_bank_data_idx < MAX_C_DATA_BANK_DATA)
			c_data_bank_data[c_data_bank_data_idx++] = *p;
		p++;
	}

	/* tell the caller if there were any addresses involved */
	return ((s && s->ident == POINTER) || is_address);
}

char *get_c_data_bank (SYMBOL *s)
{
	int i;
	struct const_array *c_data_bank_ptr;

	if (c_data_bank_nb) {
		c_data_bank_ptr = c_data_bank_var;

		for (i = 0; i < c_data_bank_nb; i++) {
			if (c_data_bank_ptr->sym == s) {
				int j = c_data_bank_val[c_data_bank_ptr->data];
				if (j >= 0)
					return (&c_data_bank_data[j]);
				else
					return (0);
			}
			c_data_bank_ptr++;
		}
	}
	return (0);
}

/*
 *	dump the c_data_bank pool
 *
 */
void dump_c_data_bank (void)
{
	intptr_t i, j, k;
	intptr_t size;

/*	intptr_t c; */

	if (c_data_bank_nb) {
		outstr("        .data\n");
		c_data_bank_ptr = c_data_bank_var;

		for (i = 0; i < c_data_bank_nb; i++) {
			outstr("        .page   3\n");
			size = c_data_bank_ptr->size;
			cptr = c_data_bank_ptr->sym;
			cptr->storage = EXTERN;
			prefix();
			outstr(cptr->name);
			outstr(":");
			nl();
			j = c_data_bank_ptr->data;

			while (size) {
				k = c_data_bank_val[j++];

				if ((cptr->type == CCHAR || cptr->type == CUCHAR) &&
				    cptr->ident != POINTER &&
				    !(cptr->ident == ARRAY && cptr->ptr_order > 0)) {
					defbyte();
					c_data_bank_size += 1;
				}
				else {
					defword();
					c_data_bank_size += 2;
				}
				if ((k == -1) || (k >= MAX_C_DATA_BANK_DATA))
					outstr("0");
				else if (k <= -1024) {
					k = (-k) - 1024;
					outlabel(litlab);
					outbyte('+');
					outdec(k);
				}
				else
					outstr(&c_data_bank_data[k]);
				nl();
				size--;
			}
			c_data_bank_ptr++;
		}
	}
}
