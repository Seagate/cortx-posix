/**
 * @file error.h
 * @author Yogesh Lahane <yogesh.lahane@seagate.com>
 */
#ifndef _TEST_KVSNS_ERROR_H
#define _TEST_KVSNS_ERROR_H

#include <errno.h>

#if 1
#define DEBUG ON
#endif

#define KVSNS_ERRNO_MAP(XX)							\
	XX(EPERM,		"Operation not permitted")			\
	XX(ENOENT,		"No such file or directory")			\
	XX(ESRCH,		"No such process")				\
	XX(EINTR,		"Interrupted system call")			\
	XX(EIO,			"Input/output error")				\
	XX(ENXIO,		"No such device or address")			\
	XX(E2BIG,		"Argument list too long")			\
	XX(ENOEXEC,		"Exec format error")				\
	XX(EBADF,		"Bad file descriptor")				\
	XX(ECHILD,		"No child processes")				\
	XX(EAGAIN,		"Resource temporarily unavailable")		\
	XX(ENOMEM,		"Cannot allocate memory")			\
	XX(EACCES,		"Permission denied")				\
	XX(EFAULT,		"Bad address")					\
	XX(ENOTBLK,		"Block device required")			\
	XX(EBUSY,		"Device or resource busy")			\
	XX(EEXIST,		"File exists")					\
	XX(EXDEV,		"Invalid cross-device link")			\
	XX(ENODEV,		"No such device")				\
	XX(ENOTDIR,		"Not a directory")				\
	XX(EISDIR,		"Is a directory")				\
	XX(EINVAL,		"Invalid argument")				\
	XX(ENFILE,		"Too many open files in system")		\
	XX(EMFILE,		"Too many open files")				\
	XX(ENOTTY,		"Inappropriate ioctl for device")		\
	XX(ETXTBSY,		"Text file busy")				\
	XX(EFBIG,		"File too large")				\
	XX(ENOSPC,		"No space left on device")			\
	XX(ESPIPE,		"Illegal seek")					\
	XX(EROFS,		"Read-only file system")			\
	XX(EMLINK,		"Too many links")				\
	XX(EPIPE,		"Broken pipe")					\
	XX(EDOM,		"Numerical argument out of domain")		\
	XX(ERANGE,		"Numerical result out of range")		\
	XX(EDEADLK,		"Resource deadlock avoided")			\
	XX(ENAMETOOLONG,	"File name too long")				\
	XX(ENOLCK,		"No locks available")				\
	XX(ENOSYS,		"Function not implemented")			\
	XX(ENOTEMPTY,		"Directory not empty")				\
	XX(ELOOP,		"Too many levels of symbolic links")		\
	XX(ENOMSG,		"No message of desired type")			\
	XX(EIDRM,		"Identifier removed")				\
	XX(ECHRNG,		"Channel number out of range")			\
	XX(EL2NSYNC,		"Level 2 not synchronized")			\
	XX(EL3HLT,		"Level 3 halted")				\
	XX(EL3RST,		"Level 3 reset")				\
	XX(ELNRNG,		"Link number out of range")			\
	XX(EUNATCH,		"Protocol driver not attached")			\
	XX(ENOCSI,		"No CSI structure available")			\
	XX(EL2HLT,		"Level 2 halted")				\
	XX(EBADE,		"Invalid exchange")				\
	XX(EBADR,		"Invalid request descriptor")			\
	XX(EXFULL,		"Exchange full")				\
	XX(ENOANO,		"No anode")					\
	XX(EBADRQC,		"Invalid request code")				\
	XX(EBADSLT,		"Invalid slot")					\
	XX(EBFONT,		"Bad font file format")				\
	XX(ENOSTR,		"Device not a stream")				\
	XX(ENODATA,		"No data available")				\
	XX(ETIME,		"Timer expired")				\
	XX(ENOSR,		"Out of streams resources")			\
	XX(ENONET,		"Machine is not on the network")		\
	XX(ENOPKG,		"Package not installed")			\
	XX(EREMOTE,		"Object is remote")				\
	XX(ENOLINK,		"Link has been severed")			\
	XX(EADV,		"Advertise error")				\
	XX(ESRMNT,		"Srmount error")				\
	XX(ECOMM,		"Communication error on send")			\
	XX(EPROTO,		"Protocol error")				\
	XX(EMULTIHOP,		"Multihop attempted")				\
	XX(EDOTDOT,		"RFS specific error")				\
	XX(EBADMSG,		"Bad message")					\
	XX(EOVERFLOW,		"Value too large for defined data type")	\
	XX(ENOTUNIQ,		"Name not unique on network")			\
	XX(EBADFD,		"File descriptor in bad state")			\
	XX(EREMCHG,		"Remote address changed")			\
	XX(ELIBACC,		"Can not access a needed shared library")	\
	XX(ELIBBAD,		"Accessing a corrupted shared library")		\
	XX(ELIBSCN,		".lib section in a.out corrupted")		\
	XX(ELIBMAX,		"Attempting to link in too many shared libraries")\
	XX(ELIBEXEC,		"Cannot exec a shared library directly")	\
	XX(EILSEQ,		"Invalid or incomplete multibyte or wide character")\
	XX(ERESTART,		"Interrupted system call should be restarted")	\
	XX(ESTRPIPE,		"Streams pipe error")				\
	XX(EUSERS,		"Too many users")				\
	XX(ENOTSOCK,		"Socket operation on non-socket")		\
	XX(EDESTADDRREQ,	"Destination address required")			\
	XX(EMSGSIZE,		"Message too long")				\
	XX(EPROTOTYPE,		"Protocol wrong type for socket")		\
	XX(ENOPROTOOPT,		"Protocol not available")			\
	XX(EPROTONOSUPPORT,	"Protocol not supported")			\
	XX(ESOCKTNOSUPPORT,	"Socket type not supported")			\
	XX(ENOTSUP,		"Operation not supported")			\
	XX(EPFNOSUPPORT,	"Protocol family not supported")		\
	XX(EAFNOSUPPORT,	"Address family not supported by protocol")	\
	XX(EADDRINUSE,		"Address already in use")			\
	XX(EADDRNOTAVAIL,	"Cannot assign requested address")		\
	XX(ENETDOWN,		"Network is down")				\
	XX(ENETUNREACH,		"Network is unreachable")			\
	XX(ENETRESET,		"Network dropped connection on reset")		\
	XX(ECONNABORTED,	"Software caused connection abort")		\
	XX(ECONNRESET,		"Connection reset by peer")			\
	XX(ENOBUFS,		"No buffer space available")			\
	XX(EISCONN,		"Transport endpoint is already connected")	\
	XX(ENOTCONN,		"Transport endpoint is not connected")		\
	XX(ESHUTDOWN,		"Cannot send after transport endpoint shutdown")\
	XX(ETOOMANYREFS,	"Too many references: cannot splice")		\
	XX(ETIMEDOUT,		"Connection timed out")				\
	XX(ECONNREFUSED,	"Connection refused")				\
	XX(EHOSTDOWN,		"Host is down")					\
	XX(EHOSTUNREACH,	"No route to host")			\
	XX(EALREADY,		"Operation already in progress")		\
	XX(EINPROGRESS,		"Operation now in progress")			\
	XX(ESTALE,		"Stale file handle")				\
	XX(EUCLEAN,		"Structure needs cleaning")			\
	XX(ENOTNAM,		"Not a XENIX named type file")			\
	XX(ENAVAIL,		"No XENIX semaphores available")		\
	XX(EISNAM,		"Is a named type file")				\
	XX(EREMOTEIO,		"Remote I/O error")				\
	XX(EDQUOT,		"Disk quota exceeded")				\
	XX(ENOMEDIUM,		"No medium found")				\
	XX(EMEDIUMTYPE,		"Wrong medium type")				\
	XX(ECANCELED,		"Operation canceled")				\
	XX(ENOTRECOVERABLE,	"State not recoverable")			\
	XX(ERFKILL,		"Operation not possible due to RF-kill")	\
	XX(EHWPOISON,		"Memory page has hardware error")		

typedef enum kvsns_error_t {
	//KVSNS_SUCCESS	= 0,
#define XX(err, _) KVSNS_ ## err = err,
	KVSNS_ERRNO_MAP(XX)
#undef XX
	KVSNS_FAIL = -1
	
} kvsns_error_t;

int kvsns_error(int err);
const char* kvsns_strerror(int err);

#define STATUS_OK 0
#define STATUS_FAIL -1

#define RC_JUMP_EQ(__rc, __status, __label) ({ \
		if (__rc == __status) \
		goto __label; \
		})

#define RC_JUMP_NOTEQ(__rc, __status, __label) ({ \
		if (__rc != __status) \
		goto __label; \
		})

#define RC_LOG_NOTEQ(__rc, __status, __label,  __function, ...) ({ \
		__rc = __function(__VA_ARGS__); \
		if (__rc != __status) { \
		fprintf(stderr, "%s:%d, %s(%s) = %d\n", __FILE__, __LINE__, \
# __function, #__VA_ARGS__, __rc); \
		goto __label; \
		}})

#define RC_RETURN_EQ(__rc, __status) ({ \
		if (__rc == __status) \
		return __rc;})

#endif
