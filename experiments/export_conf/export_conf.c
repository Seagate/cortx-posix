#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <json.h>

#define PULGINS_DIR				"/usr/lib64/ganesha/"
#define FSAL_NAME				"KVSFS"
#define FSAL_SHARED_LIBRARY			"/usr/lib64/ganesha/libfsalkvsfs.so.4.2.0"
#define EFS_CONFIG				"efs_config"

#define SERIALIZE_BUFFER_DEFAULT_SIZE 		2048 //size opt
struct str256 {
	uint8_t s_len;
	char s_str[256];
};

typedef struct str256 str256_t;

#define str256_from_cstr(dst, src, len) 				\
	do {                                                        	\
		memcpy(dst.s_str, src, len);                           	\
		dst.s_str[len] = '\0';                                 	\
		dst.s_len = len;                                       	\
	} while (0);

struct serialized_buffer{
	void *data;
	size_t size;
	int next;
};

static void init_serialized_buffer(struct serialized_buffer **buff)
{
	(*buff) = (struct serialized_buffer *)malloc( sizeof(struct serialized_buffer));
	(*buff)->data = malloc(SERIALIZE_BUFFER_DEFAULT_SIZE);
	(*buff)->size = SERIALIZE_BUFFER_DEFAULT_SIZE;
	(*buff)->next = 0;
	return;
}

static void free_serialize_buffer(struct serialized_buffer *buff) 
{
	free(buff->data);
	free(buff);
}

void serialize_data(struct serialized_buffer *buff, char *data, int nbytes){

	//if (buff == NULL) assert(0);

	//struct serialized_buffer *buffer = (struct serialized_buffer *)(b);
	int available_size = buff->size - buff->next;
	bool is_resize = 0;

	while(available_size < nbytes){
		buff->size = buff->size * 2;
		available_size = buff->size - buff->next;
		is_resize = 1;
	}

	if(is_resize == 0){
		memcpy((char *)buff->data + buff->next, data, nbytes);
		buff->next += nbytes;
		return;
	}

	// resize of the buffer
	buff->data = realloc(buff->data, buff->size); 
	memcpy((char *)buff->data + buff->next, data, nbytes);
	buff->next += nbytes;
	return;
}

struct export_fsal_block {
	str256_t name;
	str256_t efs_config;
};

struct client_block {
	str256_t clients;
	str256_t squash;
	str256_t access_type;
	str256_t protocols;
};

struct export_block {
	uint16_t export_id;
	str256_t path;
	str256_t pseudo;
	struct export_fsal_block fsal_block;
	str256_t sectype;
	str256_t filesystem_id;
	struct client_block client_block;
};

struct kvsfs_block {
	str256_t fsal_shared_library;
};

struct fsal_block {
	struct kvsfs_block kvsfs_block;

};
struct nfs_core_param_block {
	uint32_t nb_worker;
	uint32_t manage_gids_expiration;
	str256_t Plugins_Dir;
};

struct nfsv4_block {
	str256_t domainname;
	bool graceless;
};

static int json_to_export_fsal_block(struct json_object *obj,
				     struct export_fsal_block *block)
{
	int rc = 0;

	str256_from_cstr(block->name, FSAL_NAME, strlen(FSAL_NAME));
	str256_from_cstr(block->efs_config, EFS_CONFIG, strlen(EFS_CONFIG));
	return rc;
}

static int json_to_client_block(struct json_object *obj,
				struct client_block *block)
{
	int rc = 0;
	struct json_object *json_obj = NULL;
	const char *str = NULL;

	json_object_object_get_ex(obj, "clients", &json_obj);
	str = json_object_get_string(json_obj);
	str256_from_cstr(block->clients, str, strlen(str));
	str = NULL;

	json_object_object_get_ex(obj, "Squash", &json_obj);
	str = json_object_get_string(json_obj);
	str256_from_cstr(block->squash, str, strlen(str));
	str = NULL;

	json_object_object_get_ex(obj, "access_type", &json_obj);
	str = json_object_get_string(json_obj);
	str256_from_cstr(block->access_type, str, strlen(str));

	json_object_object_get_ex(obj, "protocols", &json_obj);
	str = json_object_get_string(json_obj);
	str256_from_cstr(block->protocols, str, strlen(str));
	return rc;
}

static int json_to_export_block(struct json_object *obj,
			  struct export_block *block)
{
	int rc = 0;
	struct json_object *json_obj = NULL;
	const char *str = NULL;
	char *path = "test";

	block->export_id = 9999; /* todo */
	str256_from_cstr(block->path, path, strlen(path));
	str256_from_cstr(block->pseudo, path, strlen(path));

	rc = json_to_export_fsal_block(obj, &block->fsal_block);

	json_object_object_get_ex(obj, "secType", &json_obj);
	str = json_object_get_string(json_obj);
	str256_from_cstr(block->sectype, str, strlen(str));
	str = NULL;

	json_object_object_get_ex(obj, "Filesystem_id", &json_obj);
	str = json_object_get_string(json_obj);
	str256_from_cstr(block->filesystem_id, str, strlen(str));
	str = NULL;

	rc = json_to_client_block(obj, &block->client_block);
	return rc;
}

int main()
{
	struct json_object *obj;
	const char *str  = "{ \"proto\": \"nfs\", \"mode\": \"rw\", \"secType\": \"sys\", \"Filesystem_id\": \"192.168\", \"client\": \"1\", \"clients\": \"*\", \"Squash\": \"no_root_squash\", \"access_type\": \"RW\", \"protocols\": \"4\" }";

	obj = json_tokener_parse(str);
	struct export_block *export_block = NULL;
	struct export_fsal_block *export_fsal_block = NULL;
	struct client_block *client_block = NULL;

	export_block = malloc(sizeof(struct export_block));
	//
	export_fsal_block = malloc(sizeof(struct export_fsal_block));
	//
	client_block = malloc(sizeof(struct client_block));
	//
	json_to_export_block(obj, export_block);
	json_to_export_fsal_block(obj, export_fsal_block);
	json_to_client_block(obj, client_block);

	free(export_block);
	free(export_fsal_block);
	free(client_block);
	json_object_put(obj);
	return 0;
}

/*
 *  * A simple example of json string parsing with json-c.
 *   *
 *    * gcc -Wall -g -I/usr/include/json-c/ -o conf export_conf.c -ljson-c
 *     */
