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

#ifndef _FLM_CORE_PUBLIC_IO_H_
# define _FLM_CORE_PUBLIC_IO_H_

#include <stdint.h>

typedef struct flm_IO flm_IO;

#include "flm/core/public/container.h"
#include "flm/core/public/monitor.h"
#include "flm/core/public/obj.h"

#define FLM_IO(_obj) FLM_CAST(_obj, flm_IO)

typedef void (*flm_IOReadHandler) (void * state);

typedef void (*flm_IOWriteHandler) (void * state);

typedef void (*flm_IOCloseHandler) (void * state);

typedef void (*flm_IOErrorHandler) (void * state, int error);

flm_IO *
flm_IONew (flm_Monitor *	monitor,
	   int			fd,
	   void *	state);

/**
 * \brief Close the \c flm_io object as soon as possible.
 *
 * All the data to write will be written before closing the underlying
 * file descriptor. You will never receive read notifications anymore.
 *
 * \param io A pointer to a \c flm_io object.
 * \return Nothing, this function cannot fail.
 */
void
flm_IOShutdown (flm_IO *		io);

/**
 * \brief Close the \c flm_io object immediatly.
 *
 * The underlying file descriptor will be closed even if some data are not
 * processed or written yet.
 *
 * \param io A pointer to a \c flm_IO object.
 * \return Nothing, this function cannot fail.
 */
void
flm_IOClose (flm_IO *			io);

int
flm_IODescriptor (flm_IO *		io);

void
flm_IOOnRead (flm_IO *          	io,
              flm_IOReadHandler         handler);

void
flm_IOOnWrite (flm_IO *			io,
               flm_IOWriteHandler	handler);

void
flm_IOOnClose (flm_IO *			io,
	       flm_IOCloseHandler	handler);

void
flm_IOOnError (flm_IO *			io,
	       flm_IOErrorHandler	handler);

#endif /* _FLM_CORE_PUBLIC_IO_H_ */
