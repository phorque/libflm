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

#ifndef _FLM_CORE_PUBLIC_TCP_CLIENT_H_
# define _FLM_CORE_PUBLIC_TCP_CLIENT_H_

#include <stdint.h>

#include "flm/core/public/buffer.h"

typedef struct flm_TCPClient flm_TCPClient;

#include "flm/core/public/monitor.h"
#include "flm/core/public/obj.h"

#define FLM_TCP_CLIENT(_obj) FLM_CAST(_obj, flm_TCPClient)

typedef void (*flm_TCPClientConnectHandler)				\
(flm_TCPClient * tcp_client, flm_Monitor * monitor, void * data, int fd);

typedef void (*flm_TCPClientReadHandler)				\
(flm_TCPClient * tcp_client, flm_Monitor * monitor, void * data, flm_Buffer * buffer);

typedef void (*flm_TCPClientWriteHandler)				\
(flm_TCPClient * tcp_client, flm_Monitor * monitor, void * data, flm_Buffer * buffer);

typedef void (*flm_TCPClientCloseHandler)				\
(flm_TCPClient * tcp_client, flm_Monitor * monitor, void * data);

typedef void (*flm_TCPClientErrorHandler)				\
(flm_TCPClient * tcp_client, flm_Monitor * monitor, void * data);

flm_TCPClient *
flm_TCPClientNew (flm_Monitor *				monitor,
		  flm_TCPClientConnectHandler		cn_handler,
		  flm_TCPClientReadHandler		rd_handler,
		  flm_TCPClientWriteHandler		wr_handler,
		  flm_TCPClientCloseHandler		cl_handler,
		  flm_TCPClientErrorHandler		er_handler,
		  void *				data,
		  const char *				host,
		  uint16_t				port);

#endif /* !_FLM_CORE_PUBLIC_TCP_CLIENT_H_ */