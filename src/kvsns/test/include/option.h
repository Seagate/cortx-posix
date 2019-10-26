/**
 * @file option.h
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 * @brief  Header file for open.c. create.c etc...
 */

#ifndef _TEST_KVSNS_OPTION_H
#define _TEST_KVSNS_OPTION_H

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h> /* mode_t */
#include <kvsns/kvsal.h>
#include <kvsns/kvsns.h>

typedef struct mode_map {
	const char *name; /*< mode name */
	const mode_t  mode; /*< mode int val*/
} mode_map_t;

/**
 * @brief file mode  parser
 * 
 * @param[in] str_mode		command line file mode.
 * @param[in|out] return_mode	The mode argument specifies the file mode bits be
 * 				applied when a new file is created.
 *
 * @return 			returns success = 0, failure = -1.
 */
int parse_mode(char *str_mode, mode_t *return_mode);

typedef struct flag_map {
	const char *name; /*< flag name */
	const int  flag; /*< flag int val */
} flag_map_t;

/**
 * @brief file flag  parser
 * 
 * @param[in] str_flags		command line file flags.
 * @param[in|out] return_mode	The mode argument specifies the file flags bits be
 * 				applied when a new file is opened/created.
 *
 * @return 			returns success = 0, failure = -1.
 */
int parse_flags(char *str_flags, int *return_flags);

/**
 * @brief user credentail parser
 * 
 * @param[in] str_user_cred	command line user cred.
 * @param[in|Out] cred		uid and gid to user opeing underline file.
 *
 * @return			returns success = 0, failure = -1.
 */
int parse_user_cred(char *str_user_cred, kvsns_cred_t *cred);

/**
 * IO mode's
 */
typedef enum io_mode {

	IO_MODE_FILE = 0, /*< IO mode file */

#define STR_IO_MODE_FILE "FILE"
#define READ_MODE_FILE IO_MODE_FILE
#define WRITE_MODE_FILE IO_MODE_FILE

	IO_MODE_INODE = 1 /*< IO mode inode */
#define STR_IO_MODE_INODE "INODE"
#define READ_MODE_INODE IO_MODE_INODE
#define WRITE_MODE_INODE IO_MODE_INODE

} io_mode_t;

/**
 * @brief IO mode parser
 * 
 * @param[in] str_io_mode	command line IO mode.
 * @param[in|Out] io_mode	io mode.
 *
 * @return			returns success = 0, failure = -1.
 */
int parse_io_mode(char *str_read_mode, int *io_mode);

#define parse_read_mode(str_read_mode, read_mode) \
	 parse_io_mode(str_read_mode, read_mode)

#define parse_write_mode(str_write_mode, write_mode) \
	 parse_read_mode(str_write_mode, write_mode)

#endif
