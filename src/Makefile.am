plugin_LTLIBRARIES = libgstarducamsrc.la

libgstarducamsrc_la_SOURCES = \
   gstarducamsrc.c gstarducamsrc.h

# Need -DGST_USE_UNSTABLE_API for GstBaseCameraSrc
libgstarducamsrc_la_CFLAGS = $(GST_CFLAGS) $(RPI_INCLUDEPATH) -I$(top_srcdir)
libgstarducamsrc_la_LIBADD = $(GST_LIBS) $(RPI_LIBFLAGS) -larducam_mipicamera -lbcm_host
libgstarducamsrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstarducamsrc_la_LIBTOOLFLAGS = --tag=disable-static

noinst_HEADERS = gstarducamsrc.h
