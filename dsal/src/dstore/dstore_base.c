/*
 * Filename:         dstore_base.c
 * Description:      Contains implementation of basic dstore
 * 		     framework APIs.

 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.

 Contains implementation of basic dstore framework APIs.
*/

#include "dstore.h"
#include <assert.h> /* TODO: to be replaced with dassert() */
#include <errno.h> /* ret codes such as EINVAL */
#include <string.h> /* strncmp, strlen */
#include <ini_config.h> /* collection_item and related functions */
#include "common/helpers.h" /* RC_WRAP* */
#include "common/log.h" /* log_* */
#include "debug.h" /* dassert */
#include "dstore_internal.h" /* import internal API definitions */

static struct dstore g_dstore;

struct dstore *dstore_get(void)
{
	return &g_dstore;
}

struct dstore_module {
	char *type;
	const struct dstore_ops *ops;
};

static struct dstore_module dstore_modules[] = {
	{ "eos", &eos_dstore_ops },
	{ NULL, NULL },
};

int dstore_init(struct collection_item *cfg, int flags)
{
	int rc;
	struct dstore *dstore = dstore_get();
	struct collection_item *item = NULL;
	const struct dstore_ops *dstore_ops = NULL;
	char *dstore_type = NULL;
	int i;

	assert(dstore && cfg);

	RC_WRAP(get_config_item, "dstore", "type", cfg, &item);
	if (item == NULL) {
		fprintf(stderr, "dstore type not specified\n");
		return -EINVAL;
	}

	dstore_type = get_string_config_value(item, NULL);

	assert(dstore_type != NULL);

	for (i = 0; dstore_modules[i].type != NULL; ++i) {
		if (strncmp(dstore_type, dstore_modules[i].type,
		    strlen(dstore_type)) == 0) {
			dstore_ops = dstore_modules[i].ops;
			break;
		}
	}

	dstore->type = dstore_type;
	dstore->cfg = cfg;
	dstore->flags = flags;
	dstore->dstore_ops = dstore_ops;
	assert(dstore->dstore_ops != NULL);

	assert(dstore->dstore_ops->init != NULL);
	rc = dstore->dstore_ops->init(cfg);
	if (rc) {
		return rc;
	}

	return 0;
}

int dstore_fini(struct dstore *dstore)
{
	assert(dstore && dstore->dstore_ops && dstore->dstore_ops->fini);

	return dstore->dstore_ops->fini();
}

int dstore_obj_create(struct dstore *dstore, void *ctx,
		      dstore_oid_t *oid)
{
	assert(dstore && dstore->dstore_ops && oid &&
	       dstore->dstore_ops->obj_create);

	return dstore->dstore_ops->obj_create(dstore, ctx, oid);
}

int dstore_obj_delete(struct dstore *dstore, void *ctx,
		      dstore_oid_t *oid)
{
	assert(dstore && dstore->dstore_ops && oid &&
	       dstore->dstore_ops->obj_delete);

	return dstore->dstore_ops->obj_delete(dstore, ctx, oid);
}

int dstore_obj_read(struct dstore *dstore, void *ctx,
		    dstore_oid_t *oid, off_t offset,
		    size_t buffer_size, void *buffer, bool *end_of_file,
		    struct stat *stat)
{
	assert(dstore && oid && buffer && buffer_size && stat &&
		dstore->dstore_ops->obj_read);

	return dstore->dstore_ops->obj_read(dstore, ctx, oid, offset,
					    buffer_size, buffer, end_of_file,
					    stat);
}

int dstore_obj_write(struct dstore *dstore, void *ctx,
		     dstore_oid_t *oid, off_t offset,
		     size_t buffer_size, void *buffer, bool *fsal_stable,
		     struct stat *stat)
{
	assert(dstore && oid && buffer && buffer_size && stat &&
		dstore->dstore_ops->obj_write);

	return dstore->dstore_ops->obj_write(dstore, ctx, oid, offset,
					     buffer_size, buffer, fsal_stable,
					     stat);
}

int dstore_obj_resize(struct dstore *dstore, void *ctx,
		      dstore_oid_t *oid,
		      size_t old_size, size_t new_size)
{
	assert(dstore && oid &&
	       dstore->dstore_ops && dstore->dstore_ops->obj_resize);

	return dstore->dstore_ops->obj_resize(dstore, ctx, oid, old_size,
					      new_size);
}

int dstore_get_new_objid(struct dstore *dstore, dstore_oid_t *oid)
{
	assert(dstore && oid && dstore->dstore_ops &&
	       dstore->dstore_ops->obj_get_id);

	return dstore->dstore_ops->obj_get_id(dstore, oid);
}

int dstore_obj_open(struct dstore *dstore,
		    const dstore_oid_t *oid,
		    struct dstore_obj **out)
{
	int rc;
	struct dstore_obj *result = NULL;

	dassert(dstore);
	dassert(oid);
	dassert(out);

	RC_WRAP_LABEL(rc, out, dstore->dstore_ops->obj_open, dstore, oid,
		      &result);

	result->ds = dstore;
	result->oid = *oid;

	/* Transfer the ownership of the created object to the caller. */
	*out = result;
	result = NULL;

out:
	if (result) {
		dstore_obj_close(result);
	}

	log_debug("open " OBJ_ID_F ", %p, rc=%d", OBJ_ID_P(oid),
		  rc == 0 ? *out : NULL, rc);
	return rc;
}

int dstore_obj_close(struct dstore_obj *obj)
{
	int rc;
	struct dstore *dstore;

	dassert(obj);
	dstore = obj->ds;
	dassert(dstore);

	RC_WRAP_LABEL(rc, out, dstore->dstore_ops->obj_close, obj);

out:

	log_debug("close " OBJ_ID_F ", %p",
		  OBJ_ID_P(dstore_obj_id(obj)), obj);

	return rc;
}
