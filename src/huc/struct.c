/*
 * File struct.c: (12/12/12,21:31:33)
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "data.h"
#include "lex.h"
#include "error.h"
#include "main.h"

/**
 * look up a tag in tag table by name
 * @param sname
 * @return index
 */
int find_tag (char *sname)
{
	int index;

	index = 0;
	while (index < tag_table_index) {
		if (astreq(sname, tag_table[index].name, NAMEMAX))
			return (index);

		++index;
	}
	return (-1);
}

/**
 * determine if 'sname' is a member of the struct with tag 'tag'
 * @param tag
 * @param sname
 * @return pointer to member symbol if it is, else 0
 */
SYMBOL *find_member (TAG_SYMBOL *tag, char *sname)
{
	int member_idx;

	member_idx = tag->member_idx;

	while (member_idx < tag->member_idx + tag->number_of_members) {
		if (strcmp(member_table[member_idx].name, sname) == 0)
			return (&member_table[member_idx]);

		++member_idx;
	}
	return (0);
}

/**
 * add new structure member to table
 * @param sname
 * @param identity - variable, array, pointer, function
 * @param typ
 * @param offset
 * @param storage
 * @return
 */
void add_member (char *sname, char identity, char type, int offset, int storage_class, int otag, int ptr_order)
{
	char *buffer_ptr;
	SYMBOL *symbol;

	if (member_table_index >= NUMMEMB) {
		error("symbol table overflow");
		return;
	}
	symbol = &member_table[member_table_index];
	buffer_ptr = symbol->name;
	while (an(*buffer_ptr++ = *sname++)) ;
	symbol->ident = identity;
	symbol->type = type;
	symbol->storage = storage_class;
	symbol->offset = offset;
	symbol->ptr_order = ptr_order;
	if (type == CSTRUCT)
		symbol->tagidx = otag;

	member_table_index++;
}

int define_struct (char *sname, int storage, int is_struct)
{
	TAG_SYMBOL *symbol;
	char *buffer_ptr;
	int tti;

	// tag_table_index++;
	if (tag_table_index >= NUMTAG) {
		//printf("struct table overflow (name=%s,strage=%d,is_struct=%d)", sname, storage, is_struct);
		error("struct table overflow");
		return (0);
	}
	tti = tag_table_index++;
	symbol = &tag_table[tti];
	buffer_ptr = symbol->name;
	while (an(*buffer_ptr++ = *sname++)) ;
	symbol->size = 0;
	symbol->member_idx = member_table_index;

	needbrack("{");
	while (!match("}")) {
		if (!dodcls(storage, &tag_table[tti], is_struct))
			break;
		/* We need to keep number_of_members up-to-date to make sure that
		   find_member() works so we can catch duplicate member names. */
		symbol->number_of_members = member_table_index - symbol->member_idx;
	}
	;
	/* XXX: still necessary? */
	symbol->number_of_members = member_table_index - symbol->member_idx;
	return (tti);
}
