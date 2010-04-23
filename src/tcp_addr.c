/*
 * Copyright (c) 2008-2009, Victor Goya <phorque@libflm.me>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "flm/core/private/addr.h"
#include "flm/core/private/alloc.h"
#include "flm/core/private/tcp_addr.h"

flm_TCPAddr *
flm_TCPAddrNew (const char *		hostname,
		uint16_t		port)
{
	flm_TCPAddr * tcp_addr;

	if ((tcp_addr = flm__Alloc (sizeof (flm_TCPAddr))) == NULL) {
		return (NULL);
	}
	if (flm__TCPAddrInit (tcp_addr, hostname, port) == -1) {
		flm__Free (tcp_addr);
		return (NULL);
	}
	return (tcp_addr);
}

int
flm__TCPAddrInit (flm_TCPAddr *		tcp_addr,
		  const char *		hostname,
		  uint16_t		port)
{
	struct addrinfo hints;
	char sport[6];

	memset (&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	snprintf (sport, 6, "%d", port);

	if (flm__AddrInit (FLM_ADDR (tcp_addr), hostname, sport, &hints) == -1) {
		return (-1);
	}
	FLM_OBJ (tcp_addr)->type = FLM__TYPE_TCP_ADDR;

	return (0);
}
