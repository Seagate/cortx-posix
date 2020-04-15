#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

struct filesystem_id {
	int major;
	int minor;
};

struct export_option {
	int export_id;
	char path[256];
	char pseudo[256];
	char sectype[256];
	//only singal client 
	char squash[256];
	char access_type[10];
	int protocols;
	struct filesystem_id fs_id;
};

int create_block(FILE * file, struct export_option *options)
{
	fprintf(file, "\nexport {\n");
	fprintf(file, "\tExport_Id = %d;\n", options->export_id);
	fprintf(file, "\tPath = %s;\n", options->path);
	fprintf(file, "\tPseudo = /%s;\n", options->path);
	fprintf(file, "\tFSAL {\n");
	fprintf(file, "\t\tNAME = KVSFS;\n");
	fprintf(file, "\t\tefs_config = /etc/efs/efs.conf;\n");
	fprintf(file, "\t}\n");
	fprintf(file, "\tSecType = %s;\n", options->sectype);
	fprintf(file, "\tFilesystem_id = %d.%d;\n", options->fs_id.major, options->fs_id.minor);
	/* depend on number of client */
	fprintf(file, "\tclient {\n");
	fprintf(file, "\t\tclients = *;\n"); //chanage acc to export options
	fprintf(file, "\t\tSquash = no_root_squash;\n");
	fprintf(file, "\t\taccess_type=RW;\n");
	fprintf(file, "\t\tprotocols = %d;\n", options->protocols);
	fprintf(file, "\t}\n");
	fprintf(file, "}\n");
	// confirm test.conf with ganesha helper api config_parse and retunr error code
	return 0;
}

int OSCopyFile(const char* source, const char* destination)
{
	int input, output;
	if ((input = open(source, O_RDONLY)) == -1)
	{
		return -1;
	}
	if ((output = creat(destination, 0660)) == -1)
	{
		close(input);
		return -1;
	}

	off_t bytesCopied = 0;
	struct stat fileinfo = {0};
	fstat(input, &fileinfo);
	int result = sendfile(output, input, &bytesCopied, fileinfo.st_size);

	close(input);
	close(output);
	return result;
}

int dump_file_buff(FILE * file, char **buffer)
{
	int rc = 0;
	char * buf = NULL;
	size_t length;

	fseek (file, 0, SEEK_END);
	length = ftell (file);
	fseek (file, 0, SEEK_SET);

	buf = malloc (length);
	if (buf == NULL) {
		fprintf(stderr, "Cannot allocate memeory for buffer\n");
		rc = -ENOMEM;
		goto out;
	}
	fread (buf, 1, length, file);
	*buffer = buf;
out:
	return rc;

}

int dump_buff_file( char **buffer)
{
	int rc = 0;
	int length = strlen(*buffer);
	printf("length = %d\n",length);

	FILE *tmp_file = fopen("./tmp", "a");
	if (tmp_file ==NULL ) {
		rc = -EINVAL; //retink error no;
		goto out;
	}
	fwrite(*buffer, 1, length, tmp_file);

	//check tmp file passes the ganesha config santiy if yes.

	rc = rename("./tmp", "./tmpganesha1");// should be ganesha.comnf
out:
	return rc;
}

int find_block(char *src, const char *dst, int *block_start, int *block_end)
{
	int rc = 0;
	char *tmp = strstr(src, dst);
	if(tmp == NULL) {
		printf("export block does't found\n");
		rc = -ENOENT;
		goto out;
	}
	int pos = tmp - src;
	*block_start = tmp - 10 - src;
	*block_end = *block_start + 250;
out:
	return rc;
}

int process_block(char *buffer, int start, int end)
{
	memmove(&buffer[start - 1], &buffer[start +end - 1], 250);
}

int delete_block(FILE * file, int export_id)
{
	int rc;
	int block_start, block_end;
	char *buffer;

	rc = dump_file_buff(file, &buffer);
	if (rc !=0) {
		fprintf(stderr, "Cannot dump file into buffer\n");
		goto out;
	}

	rc = find_block(buffer, "Export_Id = 11", &block_start, &block_end);
	if (rc !=0) {
		goto out;
	}

	rc = process_block(buffer, block_start, block_end);

	rc = dump_buff_file(&buffer);
	free(buffer);
out:
	return rc;
}

int main()
{
	int rc = 0;
	struct export_option options;
	char *file_path = "./test.conf";

	options.export_id = 11;
	memcpy(options.path, "test", sizeof("test"));
	memcpy(options.pseudo, "/test", sizeof("/test"));
	memcpy(options.sectype, "sys", sizeof("sys"));
	memcpy(options.squash, "no_root_squash", sizeof("no_root_squash"));
	memcpy(options.access_type, "RW", sizeof("RW"));
	options.protocols = 4;
	options.fs_id.major = 192;
	options.fs_id.minor = 168;

	rc = OSCopyFile(file_path, "./tmpganesha");

	FILE *file = fopen("./tmpganesha", "a+");
	if (file == NULL) {
		fprintf(stderr, "Can't create file for writing\n");
		return -1;
	}
	rc = create_block(file, &options);

	rc = delete_block(file, options.export_id);
	return 0;
}
