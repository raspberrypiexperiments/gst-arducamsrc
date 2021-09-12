plugin_LTLIBRARIES = libgstarducamsrc.la

# ENUM_FILES = gstrpicam_types.h
# BUILT_SOURCES = gstrpicam-enum-types.h gstrpicam-enum-types.c

libgstarducamsrc_la_SOURCES = \
   $(ENUM_FILES) $(BUILT_SOURCES) \
   gstarducamsrc.c gstarducamsrc.h
#    \
#   gstrpicamsrcdeviceprovider.c gstrpicamsrcdeviceprovider.h \
#   RaspiCapture.c RaspiCamControl.c RaspiPreview.c \
#   RaspiCLI.c

# Need -DGST_USE_UNSTABLE_API for GstBaseCameraSrc
libgstarducamsrc_la_CFLAGS = $(GST_CFLAGS) $(RPI_INCLUDEPATH) -I$(top_srcdir)
libgstarducamsrc_la_LIBADD = $(GST_LIBS) $(RPI_LIBFLAGS) -larducam_mipicamera
libgstarducamsrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstarducamsrc_la_LIBTOOLFLAGS = --tag=disable-static

#gstrpicam-enum-types.h: gstrpicam-enums-template.h $(ENUM_FILES) $(GLIB_MKENUMS)
#	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template gstrpicam-enums-template.h --identifier-prefix GstRpiCamSrc --symbol-prefix gst_rpi_cam_src $(ENUM_FILES)) > $@

#gstrpicam-enum-types.c: gstrpicam-enum-types.h gstrpicam-enums-template.c $(ENUM_FILES) $(GLIB_MKENUMS)
#	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) --template gstrpicam-enums-template.c --identifier-prefix GstRpiCamSrc --symbol-prefix gst_rpi_cam_src $(ENUM_FILES)) > $@

noinst_HEADERS = gstarducamsrc.h 
#RaspiCapture.h RaspiCamControl.h \
#    RaspiPreview.h RaspiCLI.h \
#    gstrpicam-enum-types.h $(ENUM_FILES)
CLEANFILES = $(BUILT_SOURCES)
