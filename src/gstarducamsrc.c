/*
* MIT License
*
* Copyright (c) 2021 Marcin Sielski <marcin.sielski@gmail.com>
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define _GNU_SOURCE
#include <gst/video/video.h>
#include <stdio.h>
#include <linux/v4l2-controls.h> 
#include <unistd.h>
#include "gstarducamsrc.h"

GST_DEBUG_CATEGORY_STATIC (gst_ardu_cam_src_debug);
#define GST_CAT_DEFAULT gst_ardu_cam_src_debug

enum
{
  PROP_0,
  PROP_SENSOR_NAME,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_SENSOR_MODE,
  PROP_HFLIP,
  PROP_VFLIP,
  PROP_EXPOSURE,
  PROP_GAIN,
  PROP_EXTERNAL_TRIGGER,
  PROP_AUTO_EXPOSURE,
  PROP_ROTATION,
  PROP_VIDEO_DIRECTION
};

#define WIDTH_DEFAULT 160
#define HEIGHT_DEFAULT 100
#define HFLIP_DEFAULT FALSE
#define VFLIP_DEFAULT FALSE
#define EXPOSURE_DEFAULT 681
#define GAIN_DEFAULT 1
#define EXTERNAL_TRIGGER_DEFAULT FALSE
#define AUTO_EXPOSURE_DEFAULT FALSE
#define ROTATION_DEFAULT 0

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */

#define RAW_CAPS \
  "video/x-raw, " \
  "width = (int) { 160, 320, 640, 1280 }," \
  "height = (int) { 100, 200, 400, 720, 800 }," \
  "format = (string) GRAY8," \
  "framerate = (fraction) [ 0, 480 ], " \
  "sensor-mode = (int) [ -1, 22 ] "

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ( RAW_CAPS )
    );


static void gst_ardu_cam_src_finalize (GObject *object);
static void gst_ardu_cam_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_ardu_cam_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static GstFlowReturn gst_ardu_cam_src_create (GstPushSrc * parent, 
    GstBuffer ** buf);
static GstCaps *gst_ardu_cam_src_get_caps (GstBaseSrc * src, GstCaps * filter);
static gboolean gst_ardu_cam_src_set_caps (GstBaseSrc * src, GstCaps * caps);
static gboolean gst_ardu_cam_src_start (GstBaseSrc * parent);
static gboolean gst_ardu_cam_src_stop (GstBaseSrc * parent);
static gboolean gst_ardu_cam_src_decide_allocation (GstBaseSrc * src,
    GstQuery * query);
static int raw_callback (BUFFER *buffer);
static void gst_ardu_cam_src_orientation_init (
    GstVideoOrientationInterface * iface);
static void gst_ardu_cam_src_direction_init (
  GstVideoDirectionInterface * iface);

#define gst_ardu_cam_src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE (GstArduCamSrc, gst_ardu_cam_src, 
    GST_TYPE_PUSH_SRC,
    G_IMPLEMENT_INTERFACE (GST_TYPE_VIDEO_DIRECTION, 
        gst_ardu_cam_src_direction_init);
    G_IMPLEMENT_INTERFACE (GST_TYPE_VIDEO_ORIENTATION, 
        gst_ardu_cam_src_orientation_init)
    );

#define C_ENUM(v) ((gint) v)

GType
gst_ardu_cam_src_sensor_mode_get_type (void)
{
  static const GEnumValue values[] = {
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_AUTOMATIC), "AUTOMATIC", "automatic"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_GREY_60FPS_1LANE), 
        "GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_GREY_60FPS_1LANE",
        "1280x800 GREY 60fps 1lane"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_1280x720_GREY_60FPS_1LANE),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_1280x720_GREY_60FPS_1LANE",
       "1280x720 GREY 60fps 1lane"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_640x400_GREY_210FPS_1LANE),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_640x400_GREY_210FPS_1LANE", 
        "640x400 GREY 210fps 1lane"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_320x200_GREY_420FPS_1LANE),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_320x200_GREY_420FPS_1LANE", 
        "320x200 GREY 420fps 1lane"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_160x100_GREY_480FPS_1LANE),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_160x100_GREY_480FPS_1LANE", 
        "160x100 GREY 480fps 1lane"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_GREY_480FPS_2LANES),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_GREY_480FPS_2LANES", 
        "1280x800 GREY 480fps 2lanes"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_Y10P_480FPS_2LANES),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_Y10P_480FPS_2LANES", 
        "1280x800 Y10P 480fps 2lanes"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_GREY_60FPS_1LANE_ETM),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_GREY_60FPS_1LANE_ETM", 
        "1280x800 GREY 60fps 1lanes ETM"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_1280x720_GREY_60FPS_1LANE_ETM),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_1280x720_GREY_60FPS_1LANE_ETM", 
        "1280x720 GREY 60fps 1lanes ETM"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_640x400_GREY_60FPS_1LANE_ETM),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_640x400_GREY_60FPS_1LANE_ETM", 
        "640x400 GREY 60fps 1lanes ETM"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_320x200_GREY_60FPS_1LANE_ETM),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_320x200_GREY_60FPS_1LANE_ETM", 
        "320x200 GREY 60fps 1lanes ETM"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_GREY_60FPS_2LANES_ETM),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_GREY_60FPS_2LANES_ETM", 
        "1280x800 GREY 60fps 2lanes ETM"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_Y10P_60FPS_2LANES_ETM),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_Y10P_60FPS_2LANES_ETM", 
        "1280x800 Y10P 60fps 2lanes ETM"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_1280x720_GREY_60FPS_2LANES_ETM),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_1280x720_GREY_60FPS_2LANES_ETM", 
        "1280x720 GREY 60fps 2lanes ETM"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_640x400_GREY_60FPS_2LANES_ETM),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_640x400_GREY_60FPS_2LANES_ETM", 
        "640x400 GREY 60fps 2lanes ETM"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_320x200_GREY_60FPS_2LANES_ETM),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_320x200_GREY_60FPS_2LANES_ETM", 
        "320x200 GREY 60fps 2lanes ETM"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_BA81_60FPS_1LANE),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_BA81_60FPS_1LANE", 
        "1280x800 BA81 60fps 1lane"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_1280x720_BA81_60FPS_1LANE),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_1280x720_BA81_60FPS_1LANE", 
        "1280x720 BA81 60fps 1lane"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_640x400_BA81_210FPS_1LANE),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_640x400_BA81_210FPS_1LANE", 
        "640x400 BA81 210fps 1lane"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_320x200_BA81_420FPS_1LANE),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_320x200_BA81_420FPS_1LANE", 
        "320x200 BA81 420fps 1lane"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_160x100_BA81_480FPS_1LANE),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_160x100_BA81_480FPS_1LANE", 
        "160x100 BA81 480fps 1lane"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_BA81_480FPS_2LANES),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_BA81_480FPS_2LANES", 
        "1280x800 BA81 480fps 2lanes"},
    {C_ENUM (GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_pBAA_480FPS_1LANE),
        "GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_pBAA_480FPS_1LANE", 
        "1280x800 pBAA 480fps 1lane"},
    {0, NULL, NULL}
  };

  static volatile GType id = 0;
  if (g_once_init_enter ((gsize *) & id)) {
    GType _id;
    _id = g_enum_register_static ("GstArduCamSrcSensorMode", values);
    g_once_init_leave ((gsize *) & id, _id);
  }

  return id;
}


/* GObject vmethod implementations */

/* initialize the arducamsrc's class */
static void
gst_ardu_cam_src_class_init (GstArduCamSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *basesrc_class;
  GstPushSrcClass *pushsrc_class;

  gobject_class = G_OBJECT_CLASS(klass);
  gstelement_class = GST_ELEMENT_CLASS(klass);
  basesrc_class = GST_BASE_SRC_CLASS(klass);
  pushsrc_class = GST_PUSH_SRC_CLASS(klass);

  gobject_class->finalize = gst_ardu_cam_src_finalize;
  gobject_class->set_property = gst_ardu_cam_src_set_property;
  gobject_class->get_property = gst_ardu_cam_src_get_property;
  gst_element_class_set_static_metadata (gstelement_class,
    "ArduCamSrc",
    "Source/Video",
    "ArduCam camera module source",
    "Marcin Sielski <marcin.sielski@gmail.com>");
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_template));
  basesrc_class->start = GST_DEBUG_FUNCPTR (gst_ardu_cam_src_start);
  basesrc_class->stop = GST_DEBUG_FUNCPTR (gst_ardu_cam_src_stop);
  basesrc_class->decide_allocation =
      GST_DEBUG_FUNCPTR (gst_ardu_cam_src_decide_allocation);
  basesrc_class->get_caps = GST_DEBUG_FUNCPTR (gst_ardu_cam_src_get_caps);
  basesrc_class->set_caps = GST_DEBUG_FUNCPTR (gst_ardu_cam_src_set_caps);
  pushsrc_class->create = gst_ardu_cam_src_create;  

  g_object_class_install_property (gobject_class, PROP_SENSOR_NAME,
      g_param_spec_string ("sensor-name", "Sensor Name", "Gets sensor name",
          NULL, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_WIDTH,
      g_param_spec_int ("width", "Width", "Get image width",
          160, 1280, WIDTH_DEFAULT, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_HEIGHT,
      g_param_spec_int ("height", "Height", "Get image height",
          100, 800, HEIGHT_DEFAULT, G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_SENSOR_MODE,
      g_param_spec_enum ("sensor-mode", "Sesnor Mode", "Get sensor mode",
          gst_ardu_cam_src_sensor_mode_get_type(), 
          GST_ARDU_CAM_SRC_SENSOR_MODE_AUTOMATIC,
          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_HFLIP,
      g_param_spec_boolean ("hflip", "HFlip", 
          "Enable or disable horizontal image flip", 
          HFLIP_DEFAULT, 
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_VFLIP,
      g_param_spec_boolean ("vflip", "VFlip", 
          "Enable or disable vertical image flip", 
          VFLIP_DEFAULT, 
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_EXPOSURE,
      g_param_spec_int ("exposure", "Exposure", "Set or get imager exposure",
          1, 65535, EXPOSURE_DEFAULT, 
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_GAIN,
      g_param_spec_int ("gain", "Gain", "Set or get imager gain",
          0, 15, GAIN_DEFAULT, 
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_EXTERNAL_TRIGGER,
      g_param_spec_boolean ("external-trigger", "External Trigger Mode", 
          "Enable or disable external trigger mode", 
          EXTERNAL_TRIGGER_DEFAULT, 
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_AUTO_EXPOSURE,
      g_param_spec_boolean ("auto-exposure", "Auto Exposure Mode", 
          "Enable or disable software auto exposure mode", 
          AUTO_EXPOSURE_DEFAULT, 
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_ROTATION,
      g_param_spec_int ("rotation", "Rotation",
          "Rotate captured image (0, 90, 180, 270 degrees)", 0, 270, 0,
          G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
    g_object_class_override_property (gobject_class, PROP_VIDEO_DIRECTION,
      "video-direction");
}

static void
gst_ardu_cam_src_init (GstArduCamSrc * src)
{
  gst_base_src_set_format (GST_BASE_SRC (src), GST_FORMAT_TIME);
  gst_base_src_set_live (GST_BASE_SRC (src), TRUE);
  gst_base_src_set_do_timestamp (GST_BASE_SRC (src), TRUE);

  gint stdout_bk;
  stdout_bk = dup (fileno (stderr));
  gint pipefd[2];
  pipe2 (pipefd, 0);
  dup2 (pipefd[1], fileno (stderr));
  arducam_init_camera (&src->camera_instance);
  fflush (stderr);
  close (pipefd[1]);
  dup2 (stdout_bk, fileno (stderr));
  gchar ch = 0;
  gint i = 0;
  do {
    read (pipefd[0], &ch, 1);
    if (i >= 13) {
      src->name[i-13] = ch;
    }
    i++;
  } while(!(ch == ' ' && i > 13 && i < 30));
  src->name[i-13-1] = 0;
  src->width = WIDTH_DEFAULT;
  src->height = HEIGHT_DEFAULT;
  g_mutex_init (&src->config.lock);
  src->config.change_flags = 0;
  src->config.hflip = HFLIP_DEFAULT;
  src->config.vflip = VFLIP_DEFAULT;
  src->sensor_mode = GST_ARDU_CAM_SRC_SENSOR_MODE_AUTOMATIC;
  src->config.exposure = EXPOSURE_DEFAULT;
  src->config.gain = GAIN_DEFAULT;
  src->config.external_trigger = EXTERNAL_TRIGGER_DEFAULT;
  src->config.auto_exposure = AUTO_EXPOSURE_DEFAULT;
  src->config.rotation = ROTATION_DEFAULT;

  g_mutex_init (&src->buffer.lock.ardu);
  g_mutex_init (&src->buffer.lock.gst);
}

static void
gst_ardu_cam_src_finalize (GObject *object)
{
  GstArduCamSrc *src = GST_ARDUCAMSRC (object);
  if (arducam_close_camera (src->camera_instance)) {
    GST_WARNING_OBJECT (src, "failed to close the camera");
  }
  G_OBJECT_CLASS (gst_ardu_cam_src_parent_class)->finalize (object);
}

static void gst_ardu_cam_src_set_orientation (
  GstArduCamSrc * src, GstVideoOrientationMethod orientation)
{
  switch (orientation) {
    case GST_VIDEO_ORIENTATION_IDENTITY:
      src->config.rotation = 0;
      src->config.hflip = FALSE;
      src->config.vflip = FALSE;
      GST_DEBUG_OBJECT (src, "set orientation identity");
      break;
    case GST_VIDEO_ORIENTATION_90R:
      src->config.rotation = 90;
      src->config.hflip = FALSE;
      src->config.vflip = FALSE;
      GST_DEBUG_OBJECT (src, "set orientation 90R");
      break;
    case GST_VIDEO_ORIENTATION_180:
      src->config.rotation = 180;
      src->config.hflip = FALSE;
      src->config.vflip = FALSE;
      GST_DEBUG_OBJECT (src, "set orientation 180");
      break;
    case GST_VIDEO_ORIENTATION_90L:
      src->config.rotation = 270;
      src->config.hflip = FALSE;
      src->config.vflip = FALSE;
      GST_DEBUG_OBJECT (src, "set orientation 90L");
      break;
    case GST_VIDEO_ORIENTATION_HORIZ:
      src->config.rotation = 0;
      src->config.hflip = TRUE;
      src->config.vflip = FALSE;
      GST_DEBUG_OBJECT (src, "set orientation hflip");
      break;
    case GST_VIDEO_ORIENTATION_VERT:
      src->config.rotation = 0;
      src->config.hflip = FALSE;
      src->config.vflip = TRUE;
      GST_DEBUG_OBJECT (src, "set orientation vflip");
      break;
    case GST_VIDEO_ORIENTATION_UL_LR:
      src->config.rotation = 90;
      src->config.hflip = FALSE;
      src->config.vflip = TRUE;
      GST_DEBUG_OBJECT (src, "set orientation trans");
      break;
    case GST_VIDEO_ORIENTATION_UR_LL:
      src->config.rotation = 270;
      src->config.hflip = FALSE;
      src->config.vflip = TRUE;
      GST_DEBUG_OBJECT (src, "set orientation trans");
      break;
    case GST_VIDEO_ORIENTATION_CUSTOM:
      break;
    default:
      GST_WARNING_OBJECT (src, "unsupported orientation %d", orientation);
      break;
  }
  src->config.orientation =
    orientation >= GST_VIDEO_ORIENTATION_IDENTITY &&
    orientation <= GST_VIDEO_ORIENTATION_CUSTOM ?
    orientation : GST_VIDEO_ORIENTATION_CUSTOM;
  src->config.change_flags |= PROP_CHANGE_ORIENTATION;
}

static void
gst_ardu_cam_src_direction_init (GstVideoDirectionInterface * iface)
{
  /* We implement the video-direction property */
}

static gboolean
gst_ardu_cam_src_orientation_get_hflip (
  GstVideoOrientation * orientation,gboolean * flip)
{
  GstArduCamSrc *src = GST_ARDUCAMSRC (orientation);

  g_return_val_if_fail (src != NULL, FALSE);
  g_return_val_if_fail (GST_IS_ARDUCAMSRC (src), FALSE);

  g_mutex_lock (&src->config.lock);
  *flip = src->config.hflip;
  g_mutex_unlock (&src->config.lock);

  return TRUE;
}

static gboolean
gst_ardu_cam_src_orientation_get_vflip (
  GstVideoOrientation * orientation, gboolean * flip)
{
  GstArduCamSrc *src = GST_ARDUCAMSRC (orientation);

  g_return_val_if_fail (src != NULL, FALSE);
  g_return_val_if_fail (GST_IS_ARDUCAMSRC (src), FALSE);

  g_mutex_lock (&src->config.lock);
  *flip = src->config.vflip;
  g_mutex_unlock (&src->config.lock);

  return TRUE;
}

static gboolean
gst_ardu_cam_src_orientation_set_hflip (GstVideoOrientation * orientation, gboolean flip)
{
  GstArduCamSrc *src = GST_ARDUCAMSRC (orientation);

  g_return_val_if_fail (src != NULL, FALSE);
  g_return_val_if_fail (GST_IS_ARDUCAMSRC (src), FALSE);

  g_mutex_lock (&src->config.lock);
  src->config.hflip = flip;
  src->config.change_flags |= PROP_CHANGE_ORIENTATION;
  g_mutex_unlock (&src->config.lock);
  return TRUE;
}

static gboolean
gst_ardu_cam_src_orientation_set_vflip (GstVideoOrientation * orientation, gboolean flip)
{
  GstArduCamSrc *src = GST_ARDUCAMSRC (orientation);

  g_return_val_if_fail (src != NULL, FALSE);
  g_return_val_if_fail (GST_IS_ARDUCAMSRC (src), FALSE);

  g_mutex_lock (&src->config.lock);
  src->config.vflip = flip;
  src->config.change_flags |= PROP_CHANGE_ORIENTATION;
  g_mutex_unlock (&src->config.lock);
  return TRUE;
}


static void
gst_ardu_cam_src_orientation_init (GstVideoOrientationInterface * iface)
{
  iface->get_hflip = gst_ardu_cam_src_orientation_get_hflip;
  iface->set_hflip = gst_ardu_cam_src_orientation_set_hflip;
  iface->get_vflip = gst_ardu_cam_src_orientation_get_vflip;
  iface->set_vflip = gst_ardu_cam_src_orientation_set_vflip;
}

static void
gst_ardu_cam_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstArduCamSrc *src = GST_ARDUCAMSRC (object);

  g_mutex_lock(&src->config.lock);

  switch (prop_id) 
  {
    case PROP_HFLIP:
      src->config.hflip = g_value_get_boolean (value);
      src->config.change_flags |= PROP_CHANGE_HFLIP;
      break;
    case PROP_VFLIP:
      src->config.vflip = g_value_get_boolean (value);
      src->config.change_flags |= PROP_CHANGE_VFLIP;
      break;
    case PROP_EXPOSURE:
      src->config.exposure = g_value_get_int (value);
      src->config.change_flags |= PROP_CHANGE_EXPOSURE;
      break;
    case PROP_GAIN:
      src->config.gain = g_value_get_int (value);
      src->config.change_flags |= PROP_CHANGE_GAIN;
      break;
    case PROP_EXTERNAL_TRIGGER:
      src->config.external_trigger = g_value_get_boolean (value);
      src->config.change_flags |= PROP_CHANGE_EXTERNAL_TRIGGER;
      break;
    case PROP_AUTO_EXPOSURE:
      src->config.auto_exposure = g_value_get_boolean (value);
      src->config.change_flags |= PROP_CHANGE_AUTO_EXPOSURE;
      break;
    case PROP_VIDEO_DIRECTION:
      gst_ardu_cam_src_set_orientation (src, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  g_mutex_unlock (&src->config.lock);
}

static void
gst_ardu_cam_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstArduCamSrc *src = GST_ARDUCAMSRC (object);
  g_mutex_lock (&src->config.lock);
  switch (prop_id) {
    case PROP_SENSOR_NAME:
      g_value_set_string (value, src->name);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, src->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, src->height);
      break;
    case PROP_SENSOR_MODE:
      g_value_set_enum (value, src->sensor_mode);
      break;
    case PROP_HFLIP:
      g_value_set_boolean (value, src->config.hflip);
      break;
    case PROP_VFLIP:
      g_value_set_boolean (value, src->config.vflip);
      break;
    case PROP_EXPOSURE:
      g_value_set_int (value, src->config.exposure);
      break;
    case PROP_GAIN:
      g_value_set_int (value, src->config.gain);
      break;
    case PROP_EXTERNAL_TRIGGER:
      g_value_set_boolean (value, src->config.external_trigger);
      break;
    case PROP_AUTO_EXPOSURE:
      g_value_set_boolean (value, src->config.auto_exposure);
      break;
    case PROP_ROTATION:
      g_value_set_int (value, src->config.rotation);
      break;
    case PROP_VIDEO_DIRECTION:
      g_value_set_enum (value, src->config.orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  g_mutex_unlock (&src->config.lock);
}

static int 
raw_callback (BUFFER *buffer) 
{
  GstBuffer *buf;
  GstArduCamSrc *src = buffer->userdata;
  
  g_mutex_lock (&src->buffer.lock.gst);
    if(!src->buffer.pointer) 
    {
      g_cond_wait (&src->buffer.lock.gst_cond, &src->buffer.lock.gst);
    }
  g_mutex_unlock (&src->buffer.lock.gst);

  g_mutex_lock (&src->buffer.lock.gst);
    g_mutex_lock (&src->buffer.lock.ardu);
      // NOTE(marcin.sielski): required while stopping
      if (src->started && src->buffer.pointer)
      { 
        buf = gst_buffer_new_allocate (NULL, buffer->length, NULL);
        gst_buffer_fill (buf, 0, buffer->data, buffer->length);
        *(src->buffer.pointer) = buf;
      }
      g_cond_signal (&src->buffer.lock.ardu_cond);
    g_mutex_unlock (&src->buffer.lock.ardu);
    g_cond_wait (&src->buffer.lock.gst_cond, &src->buffer.lock.gst);
  g_mutex_unlock (&src->buffer.lock.gst);
  return 0;
}

static GstFlowReturn
gst_ardu_cam_src_create (GstPushSrc * parent, GstBuffer ** buf)
{
  GstArduCamSrc *src = GST_ARDUCAMSRC (parent);
  if (!src->started) {
    src->started = TRUE;
    if (arducam_set_raw_callback (src->camera_instance, raw_callback, src)) {
      GST_ERROR_OBJECT(src, "Failed to start streaming");
      src->started = FALSE;
      return GST_FLOW_ERROR;
    }
  }
  if (src->started) {
    g_mutex_lock (&src->config.lock);
    if (src->config.change_flags)
    {
      // NOTE(marcin.sielski): Must be called upfront
      if (src->config.change_flags & PROP_CHANGE_EXTERNAL_TRIGGER)
      {
        if (arducam_set_control (src->camera_instance, V4L2_CID_ARDUCAM_EXT_TRI, 
          src->config.external_trigger)) 
        {
          GST_WARNING_OBJECT (src, "Could not set external trigger mode");
        }
      }
      if (src->config.change_flags & PROP_CHANGE_HFLIP)
      {
        if (arducam_set_control (src->camera_instance, V4L2_CID_HFLIP,
          src->config.hflip)) 
        {
          GST_WARNING_OBJECT (src, "Could not set hflip");
        }
      }
      if (src->config.change_flags & PROP_CHANGE_VFLIP)
      {
        if (arducam_set_control (src->camera_instance, V4L2_CID_VFLIP, 
          src->config.vflip)) 
        {
          GST_WARNING_OBJECT (src, "Could not set vflip");
        }
      }
      if (src->config.change_flags & PROP_CHANGE_EXPOSURE)
      {
        if (arducam_set_control (src->camera_instance, V4L2_CID_EXPOSURE, 
          src->config.exposure)) 
        {
          GST_WARNING_OBJECT (src, "Could not set exposure");
        }
      }
      if (src->config.change_flags & PROP_CHANGE_GAIN)
      {
        if (arducam_set_control (src->camera_instance, V4L2_CID_GAIN, 
          src->config.gain)) 
        {
          GST_WARNING_OBJECT (src, "Could not set gain");
        }
      }
      if (src->config.change_flags & PROP_CHANGE_AUTO_EXPOSURE)
      {
        if (arducam_software_auto_exposure (src->camera_instance, 
          src->config.auto_exposure)) 
        {
          GST_WARNING_OBJECT (src, "Could not set auto exposure mode");
        }
      }
      src->config.change_flags = 0;
    }
    g_mutex_unlock(&src->config.lock); 

    g_mutex_lock (&src->buffer.lock.gst);
      src->buffer.pointer = buf;
      g_cond_signal (&src->buffer.lock.gst_cond);
    g_mutex_unlock (&src->buffer.lock.gst);

    g_mutex_lock (&src->buffer.lock.ardu);
      g_cond_wait (&src->buffer.lock.ardu_cond, &src->buffer.lock.ardu);
      g_mutex_lock (&src->buffer.lock.gst);
        src->buffer.pointer = NULL;
      g_mutex_unlock (&src->buffer.lock.gst);
    g_mutex_unlock (&src->buffer.lock.ardu);
  }
  return GST_FLOW_OK;
}

static gboolean
gst_ardu_cam_src_start (GstBaseSrc * parent)
{
  return TRUE;
}

static gboolean
gst_ardu_cam_src_stop (GstBaseSrc * parent)
{
  GstArduCamSrc *src = GST_ARDUCAMSRC (parent);
  src->started = FALSE;
  src->buffer.pointer = NULL;  
  g_cond_signal (&src->buffer.lock.gst_cond);
  g_cond_signal (&src->buffer.lock.ardu_cond);
  if (arducam_set_raw_callback (src->camera_instance, NULL, NULL)) {
    GST_ERROR_OBJECT (src, "Could not stop streaming");
  }
  return TRUE;
}

static gboolean
gst_ardu_cam_src_decide_allocation (GstBaseSrc * bsrc, GstQuery * query)
{
  GST_LOG_OBJECT (bsrc, "In decide_allocation");
  return GST_BASE_SRC_CLASS (parent_class)->decide_allocation (bsrc, query);
}

static GstCaps *
gst_ardu_cam_src_get_caps (GstBaseSrc * bsrc, GstCaps * filter)
{
  GstCaps *caps;
  caps = gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD (bsrc));
  caps = gst_caps_make_writable (caps);
  return caps;
}

static gboolean
gst_ardu_cam_src_set_caps (GstBaseSrc * bsrc, GstCaps * caps)
{
  GstArduCamSrc *src = GST_ARDUCAMSRC (bsrc);
  GstVideoInfo info;
  GstStructure *structure;
  const gchar *format = NULL;

  if (!gst_video_info_from_caps (&info, caps)) return FALSE;
  structure = gst_caps_get_structure (caps, 0);
  if (gst_structure_get_int (structure, "height", &src->height)) 
  {
    switch(src->height) 
    {
      case 100:
        src->width = 160;
        break;
      case 200:
        src->width = 320;
        break;
      case 400:
        src->width = 640;
        break;
      case 720:
      case 800:
        src->width = 1280;
        break;
      default:
        return FALSE;
    }
    gint width;
    if (gst_structure_get_int (structure, "width", &width) && 
      width != src->width) {
      return FALSE;
    }
  }
  gint sensor_mode;
  if (gst_structure_get_int (structure, "sensor-mode", &sensor_mode)) {
    switch(sensor_mode) {
      case -1:
        break;
      case 0:
      case 5:
      case 6:
      case 7:
      case 11:
      case 12:
      case 16:
      case 21:
      case 22:
        if (src->height != 800) return FALSE;
        break;
      case 1:
      case 8:
      case 13:
      case 17:
         if (src->height != 720) return FALSE;
         break;
      case 2:
      case 9:
      case 14:
      case 18:
        if (src->height != 400) return FALSE;
        break;
      case 3:
      case 10:
      case 15:
      case 19:
        if (src->height != 200) return FALSE;
        break;
      case 4:
      case 20:
        if (src->height != 100) return FALSE;
        break;
      default:
        return FALSE;
    }
  }
  format = gst_structure_get_string (structure, "format");
  if (format && !g_str_equal (format, "GRAY8")) return FALSE;
  if (sensor_mode != -1)
  {
    src->sensor_mode = sensor_mode;
    if (arducam_set_mode (src->camera_instance, src->sensor_mode))
    {
      GST_WARNING_OBJECT (src, "Could not set sensor mode");
    }
  }
  if (arducam_set_resolution (src->camera_instance, &src->width, &src->height))
  {
    GST_ERROR_OBJECT (src, "Could not set resolution");
    return FALSE;
  }
  return TRUE;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
arducamsrc_init (GstPlugin * arducamsrc)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template arducamsrc' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_ardu_cam_src_debug, "arducamsrc",
      0, "arducamsrc");

  return gst_element_register (arducamsrc, "arducamsrc", GST_RANK_NONE,
      GST_TYPE_ARDUCAMSRC);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "gstarducamsrc"
#endif

/* gstreamer looks for this structure to register arducamsrcs
 *
 * exchange the string 'Template arducamsrc' with your arducamsrc description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    arducamsrc,
    "ArduCam camera module source",
    arducamsrc_init,
    VERSION,
    "MIT/X11",
    "Marcin Sielski",
    "https://github.com/raspberrypiexperiments/gst-arducamsrc"
)
