/**
 * @file command.h
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief  Headers for op/command handler.
 */

#ifndef _TEST_KVSNS_COMMAND_H
#define _TEST_KVSNS_COMMAND_H

#include <string.h>

/**
 * List of supported Command's
 */

#define KVSNS_OP_MAP(XX)							\
	XX(CREATE,	create,		"Create the FILE")			\
	XX(MKDIR,	mkdir,		"Create the DIRECTORY")			\
	XX(RMDIR,	rmdir,		"Remove the DIRECTORY")			\
	XX(READDIR,	readdir,	"List the DIRECTORY CONTENT")		\
	XX(LOOKUP,	lookup,		"Find the FILE/DIRECTORY")		\
	XX(READ,	read,		"Read the FILE")			\
	XX(WRITE,	write,		"Write the FILE")			\
	XX(LINK,	link,		"Create the symbolic LINK")		\
	XX(UNLINK,	unlink,		"Unlink the FILE")			\
	XX(READLINK,	readlink,	"Read the symbolic LINK CONTENT")	\
	XX(SET,		set,		"set key value pair")			\
	XX(GET,		get,		"get value for given key")			\
	XX(DEL,		del,		"del key value pair")

#define KVSAL_OP_MAP(XX)							\
	XX(SET,		set,		"set key value pair")			\
	XX(GET,		get,		"get value for given key")		\
	XX(DEL,		del,		"del key value pair")			\
	XX(INC_COUNT,	inc_count,	"inc count key value")

typedef enum kvsns_command_id_t {
	KVSNS_INIT	= 0,
#define XX(uop, lop, _) KVSNS_ ## uop,
	KVSNS_OP_MAP(XX)
#undef XX
#define XX(uop, lop, _) KVSAL_ ## uop,
	KVSAL_OP_MAP(XX)
#undef XX

} kvsns_cmd_id_t;

typedef struct kvsns_command {
	char 		*name;	/*< command name */
	kvsns_cmd_id_t	id;	/*< command id */
	int (*handle)(void *fs_ctx, void *user_cred,
			int argc, char *argv[]); /*< command handler */
	const char 	*desc;	/*< command description */
} kvsns_command_t;

/**
 * @brief kvsns command parser
 *
 * @param[in] str_command	kvsns command.
 * @param[in] fs_ctx		Filesystem context.
 * @param[in] user_cred		User credentials.
 * @param[in] argv		Command line argumenets.
 * @param[in] argc		Number of command line argumenets.
 *
 * @return 			returns success = 0, failure = -1.
 */
int parse_command(char *str_command, kvsns_command_t **command);

/**
 * Display command list.
 */
void display_commands(void);

/**
 * Command handler prototype declarations falls here
 */
#define XX(uop, lop, _) \
	int op_ ## lop (void *fs_ctx, void *user_cred, int argc, char *argv[]);
	KVSNS_OP_MAP(XX)
#undef XX

#define XX(uop, lop, _) \
	int op_ ## lop (void *fs_ctx, void *user_cred, int argc, char *argv[]);
	KVSAL_OP_MAP(XX)
#undef XX

#endif
