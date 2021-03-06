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

#include "gstgzdec.h"
#include "inflatebuf.h"

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

#ifdef GSTREAMER_010
GST_BOILERPLATE (GstGzdec, gst_gzdec, GstElement,
    GST_TYPE_ELEMENT);
static gboolean gst_gzdec_set_caps (GstPad * pad, GstCaps * caps);
static GstFlowReturn gst_gzdec_chain (GstPad * pad, GstBuffer * buf);
#else
#define gst_gzdec_parent_class parent_class
G_DEFINE_TYPE (GstGzdec, gst_gzdec, GST_TYPE_ELEMENT);
static gboolean gst_gzdec_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static GstFlowReturn gst_gzdec_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buf);
#endif


static void gst_gzdec_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_gzdec_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);


/* GObject vmethod implementations */

#ifdef GSTREAMER_010
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
#endif
/* initialize the gzdec's class */
static void
gst_gzdec_class_init (GstGzdecClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_gzdec_set_property;
  gobject_class->get_property = gst_gzdec_get_property;

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));
#ifndef GSTREAMER_010
  gst_element_class_set_details_simple(gstelement_class,
    "Gzdec",
    "gzip decoder",
    "Receive a gzipped stream and  ",
    "Paula Perez <paulaperezrubio@gmail.com>");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
#endif
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
#ifdef GSTREAMER_010
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
  init_buffer();
}
#else
static void
gst_gzdec_init (GstGzdec * filter)
{
  filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (filter->sinkpad,
      GST_DEBUG_FUNCPTR (gst_gzdec_sink_event));
  gst_pad_set_chain_function (filter->sinkpad,
      GST_DEBUG_FUNCPTR (gst_gzdec_chain));
  GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

  filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (filter->srcpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

  filter->silent = FALSE;
  init_buffer();
}
#endif

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
#ifdef GSTREAMER_010
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
#else
static gboolean
gst_gzdec_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  GstGzdec *filter;
  gboolean ret;

  filter = GST_GZDEC (parent);

  GST_LOG_OBJECT (filter, "Received %s event: %" GST_PTR_FORMAT,
      GST_EVENT_TYPE_NAME (event), event);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);
      /* do something with the caps */

      /* and forward */
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}
#endif
/* chain function
 * this function does the actual processing
 */
#ifdef GSTREAMER_010
static GstFlowReturn
gst_gzdec_chain (GstPad * pad, GstBuffer * buf)
{
  GstGzdec *filter;
  filter = GST_GZDEC (GST_OBJECT_PARENT (pad));
  GstBuffer *obuf = NULL;
  guint8 *zipped_msg, *msg;
  zipped_msg = GST_BUFFER_DATA(buf);
  uint len_in = (uint)GST_BUFFER_SIZE(buf);
  uint len_out;
  int z_ret = inflate_buffer(zipped_msg, &msg, len_in, &len_out);
  if(z_ret != 0)
  {
     if (filter->silent == FALSE)
       g_print("Stream could not be inflated correctly, returning error: (%d) \n", z_ret);
     gst_buffer_unmap (buf, &map);
     gst_buffer_unref (buf);
     return GST_FLOW_ERROR;
  }
  obuf = gst_buffer_new();
  GST_BUFFER_SIZE(obuf) = len_out;
  gst_buffer_set_data(obuf, msg, len_out);

  /* push out the decompressed buffer */
  GstFlowReturn ret = gst_pad_push (filter->srcpad, obuf);
  gst_buffer_unref (buf);
  return ret;
}
#else  
static GstFlowReturn
gst_gzdec_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstGzdec *filter;
  filter = GST_GZDEC (parent);

  GstBuffer *obuf = NULL;
  GstMemory  *mem = NULL;
  GstMapInfo map = GST_MAP_INFO_INIT;
  gst_buffer_map(buf, &map, GST_MAP_READ);
  uint len_out;
  guint8 *msg;
  int z_ret = inflate_buffer(map.data, &msg, map.size, &len_out);
  if(z_ret != 0) 
  {
     if (filter->silent == FALSE)
       g_print("Stream could not be inflated correctly, returning error (%d)\n", z_ret);
     gst_buffer_unmap (buf, &map);
     gst_buffer_unref (buf);
     return GST_FLOW_ERROR;
  }

  obuf = gst_buffer_new();
  mem = gst_memory_new_wrapped (GST_MEMORY_FLAG_READONLY,msg, len_out, 
                                0, len_out, NULL, NULL);
  gst_buffer_append_memory(obuf, mem);

  GstFlowReturn ret = gst_pad_push (filter->srcpad, obuf);
  gst_buffer_unmap (buf, &map);
  gst_buffer_unref (buf);
  return ret;
}
#endif  


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
#ifdef GSTREAMER_010
    "gzdec",
#else
    gzdec,
#endif    
    "gzip decoder",
    gzdec_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
