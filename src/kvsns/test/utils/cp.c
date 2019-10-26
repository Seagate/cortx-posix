#include <cp.h>
#include <eosfs.h>
#include <error.h>

int cp_from_kvsns(void *fs_ctx, kvsns_cred_t *user_cred,
		kvsns_file_open_t *kfd, off_t offset, size_t size, int fd)
{
	int rc = STATUS_OK;
	ssize_t rsize, wsize;
	size_t len;
	char read_buff[BUFFSIZE];

	while (size) {
		len = (size > IOLEN) ? IOLEN : size;

		rsize = kvsns2_read(fs_ctx, user_cred, kfd, read_buff, len, offset);
		if (rsize < 0) {
			fprintf(stderr, "kvsns2_read returned %ld", rsize);
			rc = STATUS_FAIL;
			goto error;
		}

		if (rsize == 0) {
			//reached EOF
			break;
		}

		wsize = write(fd, read_buff, rsize);
		if (wsize < 0) {
			rc = STATUS_FAIL;
			goto error;
		}

		if (wsize != rsize) {
			fprintf(stderr, "cp_to_kvsns wsize [%ld] != rsize [%ld].", wsize, rsize);
			rc = STATUS_FAIL;
			goto error;
		}

		size -= rsize;
		offset += rsize;
	}
error:
	return rc;
}

int cp_to_kvsns(void *fs_ctx, kvsns_cred_t *user_cred,
		kvsns_file_open_t *kfd, off_t offset, size_t size, int fd)
{
	int rc = STATUS_OK;
	ssize_t rsize, wsize;
	size_t len;
	char write_buff[BUFFSIZE];

	while (size) {
		len = (size > IOLEN) ? IOLEN : size;

		rsize = read(fd, write_buff, len);
		if (rsize < 0) {
			rc = STATUS_FAIL;
			goto error;
		}

		if (rsize == 0) {
			//reached EOF
			break;
		}

		wsize = kvsns2_write(fs_ctx, user_cred,
				kfd, write_buff, rsize, offset);
		if (wsize < 0) {
			fprintf(stderr, "kvsns2_write returned %ld", rsize);
			rc = STATUS_FAIL;
			goto error;
		}

		if (wsize != rsize) {
			fprintf(stderr, "cp_to_kvsns wsize [%ld] != rsize [%ld].", wsize, rsize);
			rc = STATUS_FAIL;
			goto error;
		}

		offset += rsize;
		size -= rsize;
	}
error:
	return rc;
}

int cp_to_and_from_kvsns(void *fs_ctx, kvsns_cred_t *user_cred,
		kvsns_file_open_t *src_kfd, kvsns_file_open_t *dest_kfd)
{
	int rc = STATUS_OK;
	ssize_t rsize, wsize;
	struct stat st;
	char buff[BUFFSIZE];

	rc = kvsns2_getattr(fs_ctx, user_cred, &src_kfd->ino, &st);
	if (rc < 0) {
		rc = STATUS_FAIL;
		goto error;
	}

	size_t size = st.st_size;
	off_t offset = 0;
	size_t len;

	while (size) {
		len = (size > IOLEN) ? IOLEN : size;

		rsize = kvsns2_read(fs_ctx, user_cred, src_kfd, buff, len, offset);
		if (rsize < 0) {
			fprintf(stderr, "kvsns2_read returned %ld", rsize);
			rc = STATUS_FAIL;
			goto error;
		}

		if (rsize == 0) {
			//reached EOF
			break;
		}

		wsize = kvsns2_write(fs_ctx, user_cred,
				dest_kfd, buff, rsize, offset);
		if (wsize < 0) {
			fprintf(stderr, "kvsns2_write returned %ld", rsize);
			rc = STATUS_FAIL;
			goto error;
		}

		if (wsize != rsize) {
			fprintf(stderr, "cp_to_and_from_kvsns wsize [%ld] != rsize [%ld].", wsize, rsize);
			rc = STATUS_FAIL;
			goto error;
		}

		offset += rsize;
		size -= rsize;
	}
error:
	return rc;
}
