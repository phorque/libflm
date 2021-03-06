/*
 * Copyright (c) 2010-2011, Victor Goya <phorque@libflm.me>
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

#if defined(linux)
#define _GNU_SOURCE
#include <sys/socket.h>
#endif

#include <sys/uio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/queue.h>
#include <sys/sendfile.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <openssl/ssl.h>

#include "flm/core/private/alloc.h"
#include "flm/core/private/buffer.h"
#include "flm/core/private/error.h"
#include "flm/core/private/file.h"
#include "flm/core/private/io.h"
#include "flm/core/private/monitor.h"
#include "flm/core/private/obj.h"
#include "flm/core/private/stream.h"

flm_Stream *
flm_StreamNew (flm_Monitor *		monitor,
	       int			fd,
	       void *                   state)
{
    flm_Stream * stream;

    if ((stream = flm__Alloc (sizeof (flm_Stream))) == NULL) {
        return (NULL);
    }
    if (flm__StreamInit (stream, monitor, fd, state) == -1) {
        flm__Free (stream);
        return (NULL);
    }
    return (stream);
}

int
flm_StreamPrintf (flm_Stream *	stream,
		  const char *	format,
		  ...)
{
    flm_Buffer * buffer;

    char * content;
    int len;
    size_t alloc;

    va_list ap;

    va_start (ap, format);
    len = vsnprintf (NULL, 0, format, ap);
    va_end (ap);

    if (len < 0) {
        goto error;
    }

    alloc = len + 1;

    content = flm__Alloc (alloc * sizeof (char));
    if (content == NULL) {
        goto error;
    }

    va_start (ap, format);
    len = vsnprintf (content, alloc, format, ap);
    va_end (ap);

    if (len < 0) {
        goto free_content;
    }

    if ((buffer = flm_BufferNew (content, len, flm__Free)) == NULL) {
        goto free_content;
    }

    if (flm_StreamPushBuffer (stream, buffer, 0, 0) == -1) {
        goto release_buffer;
    }
    flm_BufferRelease (buffer);

    return (0);

  release_buffer:
    flm_BufferRelease (buffer);
    return (-1);
  free_content:
    flm__Free (content);
  error:
    return (-1);
}

int
flm_StreamPushBuffer (flm_Stream *	stream,
		      flm_Buffer *	buffer,
		      off_t		off,
		      size_t		count)
{
    struct flm__StreamInput * input;

    if (off < 0) {
        off = 0;
    }
    if (off > (off_t) flm_BufferLength (buffer)) {
        off = flm_BufferLength (buffer);
    }
    if (count == 0 || count > flm_BufferLength (buffer)) {
        count = flm_BufferLength (buffer) - off;
    }

    if ((input = flm__Alloc (sizeof (struct flm__StreamInput))) == NULL) {
        goto error;
    }

    stream->io.wr.want = true;
    if (stream->io.monitor &&                                   \
        flm__MonitorIOReset (stream->io.monitor, &stream->io) == -1) {
        goto free_input;
    }

    input->class.buffer = flm_BufferRetain (buffer);
    input->type = FLM__STREAM_TYPE_BUFFER;
    input->off = off;
    input->count = count;
    TAILQ_INSERT_TAIL (&(stream->inputs), input, entries);

    return (0);

  free_input:
    flm__Free (input);
  error:
    return (-1);
}

int
flm_StreamPushFile (flm_Stream *	stream,
		    flm_File *		file,
		    off_t		off,
		    size_t		count)
{
    struct flm__StreamInput * input;
    struct stat stat;

    if (count == 0) {
        if (fstat (file->io.sys.fd, &stat) == -1) {
            return (-1);
        }
        if (off > stat.st_size) {
            off = stat.st_size;
        }
        count = stat.st_size - off;
    }

    if ((input = flm__Alloc (sizeof (struct flm__StreamInput))) == NULL) {
        goto error;
    }
    if (stream->io.monitor &&                           \
        flm__MonitorIOReset (stream->io.monitor, &stream->io) == -1) {
        goto free_input;
    }

    input->class.file = flm_FileRetain (file);
    input->type = FLM__STREAM_TYPE_FILE;
    input->off = off;
    input->count = count;
    TAILQ_INSERT_TAIL (&(stream->inputs), input, entries);

    stream->io.wr.want = true;
    return (0);

  free_input:
    flm__Free (input);
  error:
    return (-1);
}

void
flm_StreamShutdown (flm_Stream *        stream)
{
    flm_IOShutdown (&stream->io);
}

void
flm_StreamClose (flm_Stream *           stream)
{
    flm_IOClose (&stream->io);
}

void
flm_StreamOnRead (flm_Stream *		stream,
		  flm_StreamReadHandler	handler)
{
    stream->rd.handler = handler;
    return ;
}

void
flm_StreamOnWrite (flm_Stream *			stream,
		   flm_StreamWriteHandler	handler)
{
    stream->wr.handler = handler;
    return ;
}

void
flm_StreamOnClose (flm_Stream *                 stream,
                   flm_StreamCloseHandler       handler)
{
    flm_IOOnClose (&stream->io, (flm_IOCloseHandler) handler);
}

void
flm_StreamOnError (flm_Stream *                 stream,
                   flm_StreamErrorHandler       handler)
{
    flm_IOOnError (&stream->io, (flm_IOErrorHandler) handler);
}

int
flm_StreamStartTLSServer (flm_Stream *               stream,
                          SSL_CTX *                  context)
{
    /**
     * TODO: don't forget to flush the existing buffers, since an
     * attacker could easily inject some clear-text data right after the
     * TLS negociation.
     */
    if (flm__StreamInitTLS (stream, context) == -1) {
        goto error;
    }

    if (SSL_accept (stream->tls.obj) < 0) {
        goto shutdown_tls;
    }

    return (0);

  shutdown_tls:
    flm__StreamShutdownTLS (stream);
  error:
    return (-1);
}

flm_Stream *
flm_StreamRetain (flm_Stream * stream)
{
    return (flm__Retain (&stream->io.obj));
}

void
flm_StreamRelease (flm_Stream * stream)
{
    flm__Release (&stream->io.obj);
    return ;
}

int
flm__StreamInit (flm_Stream *	stream,
		 flm_Monitor *	monitor,
		 int		fd,
		 void *		state)
{
    if (flm__IOInit (&stream->io, monitor, fd, state) == -1) {
        return (-1);
    }
    stream->io.obj.type = FLM__TYPE_STREAM;

    stream->io.obj.perf.destruct =                              \
        (flm__ObjPerfDestruct_f) flm__StreamPerfDestruct;

    stream->io.perf.read	=                       \
        (flm__IOSysRead_f) flm__StreamPerfRead;

    stream->io.perf.write	=                       \
        (flm__IOSysWrite_f) flm__StreamPerfWrite;

    TAILQ_INIT (&stream->inputs);

    flm_StreamOnRead (stream, NULL);
    flm_StreamOnWrite (stream, NULL);

    stream->perf.alloc = flm__StreamPerfAlloc;

    stream->tls.obj = NULL;

    return (0);
}

int
flm__StreamInitTLS (flm_Stream *                stream,
                    SSL_CTX *                   context)
{
    stream->tls.obj = SSL_new (context);
    if (stream->tls.obj == NULL) {
        goto error;
    }
    if (!SSL_set_fd (stream->tls.obj, stream->io.sys.fd)) {
        goto free_obj;
    }
    return (0);

  free_obj:
    SSL_free (stream->tls.obj);
    stream->tls.obj = NULL;
  error:
    return (-1);
}

void
flm__StreamShutdownTLS (flm_Stream *    stream)
{
    if (stream->tls.obj) {
        SSL_free (stream->tls.obj);
        stream->tls.obj = NULL;
    }
    return ;
}

void
flm__StreamPerfDestruct (flm_Stream * stream)
{
    struct flm__StreamInput * input;
    struct flm__StreamInput temp;

    /**
     * Remove remaining stuff to write
     */
    TAILQ_FOREACH (input, &stream->inputs, entries) {
        temp.entries = input->entries;
        flm__Release (input->class.obj);
        TAILQ_REMOVE (&stream->inputs, input, entries);
        flm__Free (input);
        input = &temp;
    }
    flm__StreamShutdownTLS (stream);
    flm__IOPerfDestruct (&stream->io);
    return ;
}

flm_Buffer *
flm__StreamPerfAlloc (flm_Stream *      stream)
{
    char *              content;
    flm_Buffer *        buffer;

    (void) stream;

    content = flm__Alloc (FLM_STREAM__RBUFFER_SIZE);
    if (content == NULL) {
        goto error;
    }
    buffer = flm_BufferNew (content, FLM_STREAM__RBUFFER_SIZE, flm__Free);
    if (buffer == NULL) {
        goto free_content;
    }
    return (buffer);

  free_content:
    flm__Free (content);
  error:
    return (NULL);
}

void
flm__StreamPerfRead (flm_Stream *	stream,
		     flm_Monitor *	monitor,
		     uint8_t		count)
{
    struct iovec        iovec[count];
    int                 iov_count;
    int                 drain_count;

    flm_Buffer **       inputs;

    ssize_t             nb_read;
    size_t              drain;

    flm_Buffer *        buffer;
    size_t              iov_read;

    (void) monitor;

    inputs = flm__Alloc (count * sizeof (flm_Buffer *));
    if (inputs == NULL) {
        return ;
    }
    for (iov_count = 0; iov_count < count; iov_count++) {
        inputs[iov_count] = stream->perf.alloc (stream);
        if (inputs[iov_count] == NULL) {
            /* no more memory */
            count = iov_count;
            break ;

        }
        iovec[iov_count].iov_base = flm_BufferContent (inputs[iov_count]);
        iovec[iov_count].iov_len = flm_BufferLength (inputs[iov_count]);
    }
    if (iov_count == 0) {
        goto free_inputs;
    }

    nb_read = readv (((flm_IO *)(stream))->sys.fd, iovec, iov_count);

    drain_count = 0;
    if (nb_read == 0) {
        /* close */
        stream->io.rd.can = false;
        flm_IOShutdown (&stream->io);
        goto out;
    }
    else if (nb_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /**
             * the kernel buffer was empty
             */
            stream->io.rd.can = false;
        }
        else if (errno == EINTR) {
            /**
             * interrupted by a signal, just retry
             */
            stream->io.rd.can = true;
        }
        else {
            /* fatal error */
            flm__Error = FLM_ERR_ERRNO;
            flm_IOClose (&stream->io);
            /**
             * Call the error handler
             */
            if (stream->io.er.handler) {
                stream->io.er.handler (&stream->io, stream->io.state, flm_Error());
            }
        }
        goto out;
    }
    /* read event */
    ((flm_IO *)(stream))->rd.can = true;
    for (drain = nb_read; drain; drain_count++) {

        iov_read = drain < iovec[drain_count].iov_len ?		   \
            drain : iovec[drain_count].iov_len;

        if ((buffer = flm_BufferView (inputs[drain_count],         \
                                      0,                           \
                                      iov_read)) == NULL) {
            /**
             * Call the error handler
             */
            if (stream->io.er.handler) {
                stream->io.er.handler (&stream->io, stream->io.state, flm_Error());
            }
            goto out;
        }

        /**
         * Call the read handler with the new buffer
         */
        if (stream->rd.handler) {
            stream->rd.handler (stream, &stream->io.state, buffer);
        }

        if (drain < iovec[drain_count].iov_len) {
            stream->io.rd.can = false;
            drain = 0;
        }
        else {
            drain -= iovec[drain_count].iov_len;
        }
    }

  out:
    /* free remaining iov_bases */
    for (iov_count = drain_count; iov_count < count; iov_count++) {
        flm__Free (iovec[iov_count].iov_base);
    }

  free_inputs:
    for (iov_count--; iov_count >= 0; iov_count--) {
        flm_BufferRelease (inputs[iov_count]);
    }
    flm__Free (inputs);
    return ;
}

void
flm__StreamPerfWrite (flm_Stream *	stream,
		      flm_Monitor *	monitor,
		      uint8_t		count)
{
    struct flm__StreamInput * input;
    struct flm__StreamInput temp;
    ssize_t nb_write;
    ssize_t drain;

    (void) monitor;
    (void) count;

    nb_write = flm__StreamWrite (stream);
    if (nb_write < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* kernel buffer was full */
            stream->io.wr.can = false;
            return ;
        }
        else if (errno == EINTR) {
            /* interrupted by a signal, just retry */
            stream->io.wr.can = true;
            return ;
        }
        else {
            /* fatal error */
            flm__Error = FLM_ERR_ERRNO;
            flm_IOClose (&stream->io);

            /**
             * Call the error handler
             */
            if (stream->io.er.handler) {
                stream->io.er.handler (&stream->io, stream->io.state, flm_Error());
            }
            return ;
        }
    }

    /**
     * Call the write handler with the number of bytes written
     */
    if (stream->wr.handler) {
        stream->wr.handler (stream, stream->io.state, nb_write);
    }

    drain = nb_write;
    stream->io.wr.can = true;
    TAILQ_FOREACH (input, &(stream->inputs), entries) {
        if (drain == 0) {
            break ;
        }
        if (drain < input->tried) {
            input->off += drain;
            input->count -= drain;
            stream->io.wr.can = false;
            break ;
        }

        input->off += input->tried;
        input->count -= input->tried;
        drain -= input->tried;

        if (input->count == 0) {
            temp.entries = input->entries;
            flm__Release (input->class.obj);
            TAILQ_REMOVE (&(stream->inputs), input, entries);
            flm__Free (input);
            input = &temp;
        }
    }

    if (TAILQ_FIRST (&stream->inputs) == NULL) {
        stream->io.wr.want = false;
    }
    return ;
}

ssize_t
flm__StreamWrite (flm_Stream * stream)
{
    struct flm__StreamInput * input;
    ssize_t nb_write;

    if ((input = TAILQ_FIRST (&stream->inputs)) == NULL) {
        return (0);
    }

    nb_write = 0;
    switch (input->type) {
    case FLM__STREAM_TYPE_BUFFER:
        nb_write = flm__StreamSysWritev (stream);
        break ;

    case FLM__STREAM_TYPE_FILE:
#if defined (HAVE_SENDFILE)
        nb_write = flm__StreamSysSendFile (stream);
#else
        nb_write = flm__StreamSysReadWriteTo (stream);
#endif
        break ;
    }
    return (nb_write);
}

ssize_t
flm__StreamSysWritev (flm_Stream * stream)
{
    struct flm__StreamInput * input;

    struct iovec iovec[FLM_STREAM__IOVEC_SIZE];
    size_t iov_count;

    iov_count = 0;
    TAILQ_FOREACH (input, &stream->inputs, entries) {
        if (iov_count == FLM_STREAM__IOVEC_SIZE) {
            break ;
        }
        if (input->type != FLM__STREAM_TYPE_BUFFER) {
            break ;
        }

        iovec[iov_count].iov_base =
            &(flm_BufferContent (input->class.buffer)[input->off]);
        iovec[iov_count].iov_len = input->tried = input->count;
        iov_count++;
    }
    return (writev (stream->io.sys.fd, iovec, iov_count));
}

ssize_t
flm__StreamSysReadWriteTo (flm_Stream * stream)
{
    struct flm__StreamInput *   input;
    char *                      buffer;
    ssize_t                     rcount;
    ssize_t                     wcount;

    if ((input = TAILQ_FIRST (&stream->inputs)) == NULL) {
        return (0);
    }

    if (lseek (input->class.file->io.sys.fd,
               input->off,
               SEEK_SET) == -1) {
        goto error;
    }

    buffer = flm__Alloc (FLM_STREAM__READ_FILE_SIZE * sizeof (char));
    if (buffer == NULL) {
        goto error;
    }

    rcount = read (input->class.file->io.sys.fd,
                   buffer,
                   FLM_STREAM__READ_FILE_SIZE);
    if (rcount == -1) {
        goto free_buffer;
    }

    input->tried = rcount;

    wcount = write (stream->io.sys.fd, buffer, rcount);
    if (wcount == -1) {
        goto free_buffer;
    }

    flm__Free (buffer);

    return (wcount);

  free_buffer:
    flm__Free (buffer);
  error:
    return (-1);
}

ssize_t
flm__StreamSysSendFile (flm_Stream * stream)
{
    struct flm__StreamInput *   input;
    off_t                       off;

    if ((input = TAILQ_FIRST (&stream->inputs)) == NULL) {
        return (0);
    }

    off = input->off;
    input->tried = input->count;
    return (sendfile (stream->io.sys.fd,              \
                      input->class.file->io.sys.fd,   \
                      &off,                           \
                      input->tried));
}
