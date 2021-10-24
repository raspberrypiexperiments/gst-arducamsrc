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

#ifndef __GST_ARDUCAMSRC_H__
#define __GST_ARDUCAMSRC_H__

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include "arducam_mipicamera.h"

G_BEGIN_DECLS

#define GST_TYPE_ARDUCAMSRC \
  (gst_ardu_cam_src_get_type())
#define GST_ARDUCAMSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ARDUCAMSRC,GstArduCamSrc))
#define GST_ARDUCAMSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ARDUCAMSRC,GstArduCamSrcClass))
#define GST_IS_ARDUCAMSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ARDUCAMSRC))
#define GST_IS_ARDUCAMSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ARDUCAMSRC))

typedef struct _GstArduCamSrc      GstArduCamSrc;
typedef struct _GstArduCamSrcClass GstArduCamSrcClass;

typedef enum
{
  PROP_CHANGE_VFLIP            = (1 << 0),
  PROP_CHANGE_HFLIP            = (1 << 1),
  PROP_CHANGE_EXPOSURE         = (1 << 2),
  PROP_CHANGE_GAIN             = (1 << 3),
  PROP_CHANGE_EXTERNAL_TRIGGER = (1 << 4),
  PROP_CHANGE_EXPOSURE_MODE    = (1 << 5),
} ArduCamPropChangeFlags;

typedef struct
{
  GMutex lock;
  ArduCamPropChangeFlags change_flags;
  gboolean hflip;
  gboolean vflip;
  gint exposure;
  gint gain;
  gboolean external_trigger;
  gboolean exposure_mode;
  gint timeout;
}
ArduCamConfig;

struct _GstArduCamSrc
{
  GstPushSrc parent;

  gchar name[16];
  gint width;
  gint height;
  gint sensor_mode;
  ArduCamConfig config;
};

struct _GstArduCamSrcClass 
{
  GstPushSrcClass parent_class;
};

GType gst_ardu_cam_src_get_type (void);

typedef enum {
  GST_ARDU_CAM_SRC_SENSOR_MODE_AUTOMATIC = -1,
  GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_GREY_60FPS_1LANE = 0,
  GST_ARDU_CAM_SRC_SENSOR_MODE_1280x720_GREY_60FPS_1LANE = 1,
  GST_ARDU_CAM_SRC_SENSOR_MODE_640x400_GREY_210FPS_1LANE = 2,
  GST_ARDU_CAM_SRC_SENSOR_MODE_320x200_GREY_420FPS_1LANE = 3,
  GST_ARDU_CAM_SRC_SENSOR_MODE_160x100_GREY_480FPS_1LANE = 4,
  GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_GREY_480FPS_2LANES = 5,
  GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_Y10P_480FPS_2LANES = 6,
  GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_GREY_60FPS_1LANE_ETM = 7,
  GST_ARDU_CAM_SRC_SENSOR_MODE_1280x720_GREY_60FPS_1LANE_ETM = 8,
  GST_ARDU_CAM_SRC_SENSOR_MODE_640x400_GREY_60FPS_1LANE_ETM = 9,
  GST_ARDU_CAM_SRC_SENSOR_MODE_320x200_GREY_60FPS_1LANE_ETM = 10,
  GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_GREY_60FPS_2LANES_ETM = 11,
  GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_Y10P_60FPS_2LANES_ETM = 12,
  GST_ARDU_CAM_SRC_SENSOR_MODE_1280x720_GREY_60FPS_2LANES_ETM = 13,
  GST_ARDU_CAM_SRC_SENSOR_MODE_640x400_GREY_60FPS_2LANES_ETM = 14,
  GST_ARDU_CAM_SRC_SENSOR_MODE_320x200_GREY_60FPS_2LANES_ETM = 15,
  GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_BA81_60FPS_1LANE = 16,
  GST_ARDU_CAM_SRC_SENSOR_MODE_1280x720_BA81_60FPS_1LANE = 17,
  GST_ARDU_CAM_SRC_SENSOR_MODE_640x400_BA81_210FPS_1LANE = 18,
  GST_ARDU_CAM_SRC_SENSOR_MODE_320x200_BA81_420FPS_1LANE = 19,
  GST_ARDU_CAM_SRC_SENSOR_MODE_160x100_BA81_480FPS_1LANE = 20,
  GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_BA81_480FPS_2LANES = 21,
  GST_ARDU_CAM_SRC_SENSOR_MODE_1280x800_pBAA_480FPS_1LANE = 22,
} GstRpiCamSrcSensorMode;

G_END_DECLS

#endif /* __GST_ARDUCAMSRC_H__ */
