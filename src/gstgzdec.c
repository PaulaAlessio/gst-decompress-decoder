/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2022 Paula Perez <paulaperezrubio@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * SECTION:element-gzdec
 *
 * Plugin decoder for GStreamer. It receives a
 * stream compressed with gzip and emits an
 * uncompressed stream. It should only have one sink
 * pad and one source pad.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! gzdec ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include "zlib.h"

#include "gstgzdec.h"

// zlib chunks
#define CHUNK 16384

GST_DEBUG_CATEGORY_STATIC (gst_gzdec_debug);
#define GST_CAT_DEFAULT gst_gzdec_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

GST_BOILERPLATE (GstGzdec, gst_gzdec, GstElement,
    GST_TYPE_ELEMENT);

static void gst_gzdec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_gzdec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_gzdec_set_caps (GstPad * pad, GstCaps * caps);
static GstFlowReturn gst_gzdec_chain (GstPad * pad, GstBuffer * buf);

/* GObject vmethod implementations */

static void
gst_gzdec_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "Gzdec",
    "gzip decoder",
    "Receive a gzipped stream and  ",
    "Paula Perez <paulaperezrubio@gmail.com>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));
}

/* initialize the gzdec's class */
static void
gst_gzdec_class_init (GstGzdecClass * klass)
{
  GObjectClass *gobject_class;
//  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
//  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_gzdec_set_property;
  gobject_class->get_property = gst_gzdec_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_gzdec_init (GstGzdec * filter,
    GstGzdecClass * gclass)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_setcaps_function (filter->sinkpad,
                                GST_DEBUG_FUNCPTR(gst_gzdec_set_caps));
  gst_pad_set_getcaps_function (filter->sinkpad,
                                GST_DEBUG_FUNCPTR(gst_pad_proxy_getcaps));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_gzdec_chain));

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  gst_pad_set_getcaps_function (filter->srcpad,
                                GST_DEBUG_FUNCPTR(gst_pad_proxy_getcaps));

  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);
  filter->silent = FALSE;
}

static void
gst_gzdec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstGzdec *filter = GST_GZDEC (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_gzdec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstGzdec *filter = GST_GZDEC (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles the link with other elements */
static gboolean
gst_gzdec_set_caps (GstPad * pad, GstCaps * caps)
{
  GstGzdec *filter;
  GstPad *otherpad;

  filter = GST_GZDEC (gst_pad_get_parent (pad));
  otherpad = (pad == filter->srcpad) ? filter->sinkpad : filter->srcpad;
  gst_object_unref (filter);

  return gst_pad_set_caps (otherpad, caps);
}

int inflate_stream(unsigned char *source, unsigned char **dest, uInt lenIn, uInt *lenOut)
{
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char *in = source;
  unsigned char *out;

  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  // This function is needed to be called instead of inflateInit
  // in order for it to work for G-streams
  ret = inflateInit2(&strm, 16 + MAX_WBITS);
  if (ret != Z_OK)   return ret;

  uInt dest_size = lenIn;
  uInt realloc_size = dest_size > CHUNK ? dest_size : CHUNK;
  uInt dest_free_size = dest_size;
  *dest = malloc(dest_size);
  if(*dest == NULL) return -1;
  out = *dest;
  *lenOut = 0;

  uInt remainingIn = lenIn;
  /* decompress until deflate stream ends or end of file */
  do
  {
    int next_chunk_length = remainingIn >= CHUNK ? CHUNK : remainingIn;
    strm.avail_in = next_chunk_length;

    if (strm.avail_in == 0)
        break;
    strm.next_in = in;

    /* run inflate() on input until output buffer not full */
    do {
      if(dest_free_size < CHUNK)
      {
        unsigned char * new_dest;
        new_dest = realloc(*dest, dest_size + realloc_size);
        if(!new_dest)
        {
          free(*dest);
          return Z_DATA_ERROR;
        }
        *dest = new_dest;
        dest_size += realloc_size;
        dest_free_size += realloc_size;
        out = *dest + *lenOut;
      }
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = inflate(&strm, Z_NO_FLUSH);
      if(ret == Z_STREAM_ERROR)  /* state not clobbered */
      {
        (void)inflateEnd(&strm);
        free (*dest);
        return ret;
      }
      switch (ret)
      {
        case Z_NEED_DICT:
          ret = Z_DATA_ERROR;     /* and fall through */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
          (void)inflateEnd(&strm);
          free (*dest);
          return ret;
      }
      have = CHUNK - strm.avail_out;
      *lenOut += have;
      out += have;
      dest_free_size -= have;

    } while (strm.avail_out == 0);

    in += next_chunk_length;
    remainingIn -= next_chunk_length;
    /* done when inflate() says it's done */
  } while (ret != Z_STREAM_END);

  if(*lenOut)
  {
    unsigned char * new_dest;
    new_dest = realloc(*dest, *lenOut);
    if(!new_dest)
    {
      free(*dest);
      return Z_DATA_ERROR;
    }
    *dest = new_dest;
  }
  else
  {
    free(*dest);
    return Z_DATA_ERROR;
  }

  /* clean up and return */
  (void)inflateEnd(&strm);
  return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_gzdec_chain (GstPad * pad, GstBuffer * buf)
{
  GstGzdec *filter;

  filter = GST_GZDEC (GST_OBJECT_PARENT (pad));

  if (filter->silent == FALSE)
    g_print ("I'm plugged, decompressing data\n");

  guint8 *zipped_msg, *msg;
  zipped_msg = GST_BUFFER_DATA(buf);
  uInt lenIn = (uInt)GST_BUFFER_SIZE(buf);
  uInt lenOut;
  if( inflate_stream(zipped_msg, &msg, lenIn, &lenOut) != Z_OK)
  {
     /* just push out the incoming buffer without touching it  and return error*/
     if (filter->silent == FALSE)
       g_print("Stream could not be inflated correctly, returning error (-5)\n");
     gst_pad_push (filter->srcpad, buf);
     return -5; // GstFlowReturn ERROR
  }
  GstBuffer *out_buf = NULL;
  out_buf = gst_buffer_new ();
  GST_BUFFER_SIZE(out_buf) = lenOut;
  gst_buffer_set_data(out_buf, msg, lenOut);

  /* push out the decompressed buffer */
  GstFlowReturn ret = gst_pad_push (filter->srcpad, out_buf);

  return ret;
}



/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
gzdec_init (GstPlugin * gzdec)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template gzdec' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_gzdec_debug, "gzdec",
      0, "Template gzdec");

  return gst_element_register (gzdec, "gzdec", GST_RANK_NONE,
      GST_TYPE_GZDEC);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstgzdec"
#endif

/* gstreamer looks for this structure to register gzdecs
 *
 * exchange the string 'Template gzdec' with your gzdec description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "gzdec",
    "gzip decoder",
    gzdec_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
