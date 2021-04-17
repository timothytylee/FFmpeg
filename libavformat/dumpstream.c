/*
 * Input stream dump protocol.
 * Copyright (c) 2021 Timothy Lee
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "avformat.h"
#include "avio.h"
#include "url.h"

typedef struct Context {
    AVClass *class;
    int64_t count;
    AVIOContext *dump;
    AVIOContext *io;
    const char *dump_file;
} Context;

static int dumpstream_open(URLContext *h, const char *arg, int flags, AVDictionary **options)
{
    Context *c= h->priv_data;
    int ret = avio_open2(&c->dump, c->dump_file, AVIO_FLAG_READ_WRITE, NULL, NULL);
    if (ret < 0) {
        av_log(h, AV_LOG_ERROR, "Failed to create dump file\n");
        return ret;
    }
    av_strstart(arg, "dumpstream:", &arg);
    return avio_open2(&c->io, arg, flags, &h->interrupt_callback, options);
}

static int dumpstream_read(URLContext *h, unsigned char *buf, int size)
{
    Context *c = h->priv_data;
    int r = avio_read(c->io, buf, size);
    if ((r > 0) && c->dump) {
        avio_write(c->dump, buf, r);
        c->count += r;
    }
    return r;
}

static int dumpstream_close(URLContext *h)
{
    Context *c = h->priv_data;
    av_log(h, AV_LOG_INFO, "Dumped %"PRId64" bytes\n", c->count);
    if (c->dump)  avio_closep(&c->dump);
    return avio_closep(&c->io);
}

#define OFFSET(x) offsetof(Context, x)
#define D AV_OPT_FLAG_DECODING_PARAM

static const AVOption options[] = {
    { "dump_file", "Name of dump file", OFFSET(dump_file), AV_OPT_TYPE_STRING, { .str = "dump.dat" },  CHAR_MIN, CHAR_MAX, D },
    {NULL},
};

static const AVClass dumpstream_context_class = {
    .class_name = "dumpstream",
    .item_name  = av_default_item_name,
    .option     = options,
    .version    = LIBAVUTIL_VERSION_INT,
};

const URLProtocol ff_dumpstream_protocol = {
    .name                = "dumpstream",
    .url_open2           = dumpstream_open,
    .url_read            = dumpstream_read,
    .url_close           = dumpstream_close,
    .priv_data_size      = sizeof(Context),
    .priv_data_class     = &dumpstream_context_class,
};
