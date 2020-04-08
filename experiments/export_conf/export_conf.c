#include <stdio.h>
#include <string.h>

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

int efs_ganesha_conf(FILE * file, struct export_option *options)
{

	fprintf(file, "\nexport {\n");
	fprintf(file, "\tExport_id = %d;\n", options->export_id );
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

int main()
{
	int rc = 0;
	struct export_option options;
	char *file_path = "./test.conf";

	FILE *file = fopen(file_path, "a+");
	if (file == NULL) {
		fprintf(stderr, "Can't create file for writing\n");
		return -1;
	}

	options.export_id = 11;
	memcpy(options.path, "test", sizeof("test"));
	memcpy(options.pseudo, "/test", sizeof("/test"));
	memcpy(options.sectype, "sys", sizeof("sys"));
	memcpy(options.squash, "no_root_squash", sizeof("no_root_squash"));
	memcpy(options.access_type, "RW", sizeof("RW"));
	options.protocols = 4;
	options.fs_id.major = 192;
	options.fs_id.minor = 168;

	rc = efs_ganesha_conf(file, &options);
	return 0;
}
