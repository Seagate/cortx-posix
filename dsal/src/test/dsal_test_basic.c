/*****************************************************************************/
/*
 * Filename:		dsal_test_basic.c
 * Description:		Test group for very basic DSAL tests.
 *
 * Do NOT modify or remove this copyright and confidentiality notice!
 * Copyright (c) 2019, Seagate Technology, LLC.
 * The code contained herein is CONFIDENTIAL to Seagate Technology, LLC.
 * Portions are also trade secret. Any use, duplication, derivation,
 * distribution or disclosure of this code, for any reason, not expressly
 * authorized is prohibited. All other rights are expressly reserved by
 * Seagate Technology, LLC.
 */
/******************************************************************************/
#include <stdio.h> /* *printf */
#include <memory.h> /* mem* functions */
#include <assert.h> /* asserts */
#include <errno.h> /* errno codes */
#include <stdlib.h> /* alloc, free */
#include "dstore.h" /* dstore operations to be tested */
#include "dsal_test_lib.h" /* DSAL-specific helpers for tests */

/*****************************************************************************/
/** Test environment for the test group.
 * The environment is prepared by setup() and cleaned up by teardown()
 * calls.
 */
struct env {
	/* Object ID to be used in the test cases.
	 * Rationale of keeping it global:
	 * 1. Each test case is responsibe for releasing this ID.
	 * 2. If test fails then the process goes down, so that
	 *    a new ID will be generated so that we won't get any collisions.
	 */
	dstore_oid_t oid;
	/* Dstore instance.
	 * Rationale of keeping it global:
	 * It is a singleton and it is being initialized
	 * only once. The initialization is not a part
	 * of any test case in this module.
	 */
	struct dstore *dstore;
};

#define ENV_FROM_STATE(__state) (*((struct env **) __state))

/*****************************************************************************/
/* Description: Test create and delete operation.
 * Strategy:
 *	Create a new file.
 *	Delete the new file.
 * Expected behavior:
 *	No errors from the DSAL calls.
 * Enviroment:
 *	Empty dstore.
 */
static void test_creat_del_basic(void **state)
{
	int rc;
	struct env *env = ENV_FROM_STATE(state);

	rc = dstore_obj_create(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_delete(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);
}

/*****************************************************************************/
/* Description: Test creation of already existing object
 * Strategy:
 *	Create a new file
 *	Again create the same file
 *	In above step DSAL should return -EEXIST
 *	Delete the new file
 * Expected behavior:
 *	When we try to recreate the already existing file DSAL should give
 *	error with  -EEXIST return code
 * Enviroment:
 *	Empty dstore.
 */
static void test_creat_existing(void **state)
{
	int rc;
	struct env *env = ENV_FROM_STATE(state);

	rc = dstore_obj_create(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_create(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, -EEXIST);

	rc = dstore_obj_delete(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);
}

/*****************************************************************************/
/* Description: Test deletion of non existing object
 * Strategy:
 *	Delete the non existing file
 * Expected behavior:
 *	 DSAL layer should return -ENOENT
 * Enviroment:
 *	Empty dstore.
 */
static void test_del_non_existing(void **state)
{
	int rc;
	struct env *env = ENV_FROM_STATE(state);

	rc = dstore_obj_delete(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, -ENOENT);
}

/*****************************************************************************/
/* Description: Test Open and Close operations.
 * Strategy:
 *	Create a new file.
 *	Open the new file.
 *	Close the new file.
 *	Delete the new file.
 * Expected behavior:
 *	No errors from the DSAL calls.
 * Enviroment:
 *	Empty dstore.
 */
static void test_open_close_basic(void **state)
{
	int rc;
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);

	rc = dstore_obj_create(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_open(env->dstore, &env->oid, &obj);
	ut_assert_int_equal(rc, 0);
	ut_assert_not_null(obj);

	rc = dstore_obj_close(obj);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_delete(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);
}

/*****************************************************************************/
/* Description: Open non-existing object.
 * Strategy:
 *	Create a new file.
 *	Open the new file.
 *	Close the new file.
 *	Delete the new file.
 *	Open the deleted file
 * Expected behavior:
 *	No errors from the DSAL calls except ENOENT.
 * Enviroment:
 *	Empty dstore.
 */
static void test_open_non_existing(void **state)
{
	int rc;
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);

	rc = dstore_obj_create(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_open(env->dstore, &env->oid, &obj);
	ut_assert_int_equal(rc, 0);
	ut_assert_not_null(obj);

	rc = dstore_obj_close(obj);
	ut_assert_int_equal(rc, 0);
	obj = NULL;

	rc = dstore_obj_delete(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_open(env->dstore, &env->oid, &obj);
	ut_assert_int_equal(rc, -ENOENT);
	ut_assert_null(obj);
}

/*****************************************************************************/
/* Description: Delete open object.
 * DSAL does not provide any guarantes regarding removal of open objects
 * and should not fail on close() call even if the object was removed.
 * The upper modules should detect and handle this scenario in accordance
 * with their state management policies.
 * Strategy:
 *	Create a new file.
 *	Open the new file.
 *	Close the new file.
 *	Delete the new file.
 *	Open the deleted file
 * Expected behavior:
 *	No errors from the DSAL calls except ENOENT.
 * Enviroment:
 *	Empty dstore.
 */
static void test_delete_open(void **state)
{
	int rc;
	struct dstore_obj *obj = NULL;
	struct env *env = ENV_FROM_STATE(state);

	rc = dstore_obj_create(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_open(env->dstore, &env->oid, &obj);
	ut_assert_int_equal(rc, 0);
	ut_assert_not_null(obj);

	rc = dstore_obj_delete(env->dstore, NULL, &env->oid);
	ut_assert_int_equal(rc, 0);

	rc = dstore_obj_close(obj);
	ut_assert_int_equal(rc, 0);
	obj = NULL;
}

/*****************************************************************************/
static int test_group_setup(void **state)
{
	struct env *env;

	env = calloc(sizeof(struct env), 1);
	ut_assert_not_null(env);

	env->oid = *dtlib_def_obj();
	env->dstore = dtlib_dstore();

	*state = env;

	return 0;
}

static int test_group_teardown(void **state)
{
	struct env *env = ENV_FROM_STATE(state);

	free(env);
	*state = NULL;

	return 0;
}

/*****************************************************************************/
/* Entry point for test group execution. */
int main(int argc, char *argv[])
{
	int rc;

	struct test_case test_group[] = {
		ut_test_case(test_creat_del_basic, NULL, NULL),
		ut_test_case(test_creat_existing, NULL, NULL),
		ut_test_case(test_del_non_existing, NULL, NULL),
		ut_test_case(test_open_close_basic, NULL, NULL),
		ut_test_case(test_open_non_existing, NULL, NULL),
		ut_test_case(test_delete_open, NULL, NULL),
	};

	dtlib_setup(argc, argv);
	rc = DSAL_UT_RUN(test_group, test_group_setup, test_group_teardown);
	dtlib_teardown();

	return rc;
}
