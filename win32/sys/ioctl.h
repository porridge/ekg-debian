/*
 * <sys/ioctl.h>
 *
 * zast�pienie ioctl() przez ioctlsocket(). zwyk�y #define
 * to za ma�o ze wzgl�du na niezgodno�� typu ostatniego argumentu obu
 * funkcji.
 *
 * $Id$
 */

#ifndef COMPAT_SYS_IOCTL_H
#define COMPAT_SYS_IOCTL_H

#include <winioctl.h>
#include <stdarg.h>

inline int ioctl(int fd, int request, ...)
{
	va_list ap;
	unsigned long *foo;

	va_start(ap, request);
	foo = va_arg(ap, unsigned long*);
	va_end(ap);
	
	return ioctlsocket(fd, request, foo);

}

#endif /* COMPAT_SYS_IOCTL_H */
