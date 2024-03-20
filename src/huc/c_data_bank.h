/*	File c_data_bank.h: 2.1 (00/07/17,16:02:19) */
/*% cc -O -c %
 *
 */

#ifndef _C_DATA_BANK_H
#define _C_DATA_BANK_H

void new_c_data_bank (void);
void add_c_data_bank (intptr_t typ);
intptr_t array_initializer_cdb (intptr_t typ, intptr_t id, intptr_t stor);
intptr_t scalar_initializer_cdb (intptr_t typ, intptr_t id, intptr_t stor);
intptr_t get_string_ptr_cdb (intptr_t typ);
intptr_t get_raw_value_cdb (char sep);
int add_buffer_cdb (char *p, char c, int is_address);
void dump_c_data_bank (void);
char *get_c_data_bank (SYMBOL *);

#endif
