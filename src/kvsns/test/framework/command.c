/**
 * @file command.c
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief  command line argument parser utility to.
 */

#include <stdio.h>
#include <string.h>
#include <command.h>

kvsns_command_t command_table[] = {

#define XX(uop, lop, desc) \
	{#lop,	KVSNS_ ## uop,	op_ ## lop,	desc},
	KVSNS_OP_MAP(XX)
#undef XX

#define XX(uop, lop, desc) \
	{#lop,	KVSAL_ ## uop,	op_ ## lop,	desc},
	KVSAL_OP_MAP(XX)
#undef XX
	/* Coomand table terminator entry */
	{NULL,	0,		NULL,		NULL}
};

int parse_command(char *str_command, kvsns_command_t **command)
{
	int rc = 0;
	*command = NULL;
	int i;

	for (i = 0; command_table[i].name != NULL; i++) {
		if (!strcmp(str_command, command_table[i].name)){
			*command = &command_table[i];
			break;
		}
	}

	if (command_table[i].name == NULL) {
		fprintf(stderr, "No such command: %s\n", str_command);
		rc = -1;
		goto error;
	}

error:
	return rc;
}

void display_commands(void)
{
	int i;

	for (i = 0; command_table[i].name != NULL; i++) {
		printf("\t%-15s\t%s\n",
			command_table[i].name, command_table[i].desc);
	}
}
