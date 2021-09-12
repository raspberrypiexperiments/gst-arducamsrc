#ifndef __GST_ARDUCAMSRC_H__
#define __GST_ARDUCAMSRC_H__

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>
#include <gst/pbutils/pbutils.h>        /* only used for GST_PLUGINS_BASE_VERSION_* */

G_BEGIN_DECLS

#define GST_TYPE_ARDUCAMSRC (gst_ardu_cam_src_get_type())
#define GST_ARDUCAMSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ARDUCAMSRC,GstArduCamSrc))
#define GST_ARDUCAMSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ARDUCAMSRC,GstArduCamSrcClass))
#define GST_IS_ARDUCAMSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ARDUCAMSRC))
#define GST_IS_ARDUCAMSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ARDUCAMSRC))



G_END_DECLS

#endif /* __GST_ARDUCAMSRC_H__ */