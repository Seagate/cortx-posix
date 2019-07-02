/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

#ifndef _COMMON_H
#define _COMMON_H

#define RC_WRAP(__function, ...) ({\
	int __rc = __function(__VA_ARGS__);\
	if (__rc < 0)        \
		return __rc; })

#define RC_WRAP_LABEL(__rc, __label, __function, ...) ({\
	__rc = __function(__VA_ARGS__);\
	if (__rc < 0)        \
		goto __label; })

/* Uncomment this define if you want to get detailed trace
 * of RC_WRAP_LABEL calls
 */
// #define ENABLE_RC_WRAP_LABEL_TRACE
#if defined(ENABLE_RC_WRAP_LABEL_TRACE)

#undef RC_WRAP_LABEL

#define RC_WRAP_LABEL(__rc, __label, __function, ...) ({\
	__rc = __function(__VA_ARGS__);\
	fprintf(stderr, "%s:%d, %s(%s) = %d\n", __FILE__, __LINE__,\
		# __function, #__VA_ARGS__, __rc); \
	if (__rc < 0)        \
		goto __label; })

#endif


#ifndef kvsns_likely
#define kvsns_likely(__cond)   __builtin_expect(!!(__cond), 1)
#endif

#ifndef kvsns_unlikely
#define kvsns_unlikely(__cond) __builtin_expect(!!(__cond), 0)
#endif

#endif // _COMMON_H
