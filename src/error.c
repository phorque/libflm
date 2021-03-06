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

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "flm/core/public/error.h"
#include "flm/core/private/error.h"
#include "flm/core/private/alloc.h"

pthread_key_t flm__ErrorLocationKey;
pthread_once_t flm__ErrorInitOnce;

static void flm__ErrorInit (void);

int
flm_Error ()
{
    return (flm__Error);
}

void
flm__ErrorInit ()
{
    pthread_key_create (&flm__ErrorLocationKey, NULL);
    return ;
}

int *
flm__ErrorLocation ()
{
    int * error;

    assert (pthread_once (&flm__ErrorInitOnce, flm__ErrorInit) != -1);

    if ((error = pthread_getspecific (flm__ErrorLocationKey)) == NULL) {
        error = malloc (sizeof (int));

        /**
         * If the library cannot even take some memory to store the
         * error value the first time, there is no point to continue.
         */
        assert (error != NULL);

        pthread_setspecific (flm__ErrorLocationKey, error);
    }
    return (error);
}
