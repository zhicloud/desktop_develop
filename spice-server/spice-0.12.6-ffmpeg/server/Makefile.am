NULL =
SUBDIRS = . tests

AM_CPPFLAGS =					\
	-DSPICE_SERVER_INTERNAL			\
	-DRED_STATISTICS			\
	$(COMMON_CFLAGS)			\
	$(GLIB2_CFLAGS)				\
	$(LZ4_CFLAGS)				\
	$(PIXMAN_CFLAGS)			\
	$(SASL_CFLAGS)				\
	$(SLIRP_CFLAGS)				\
	$(SMARTCARD_CFLAGS)			\
	$(GSTREAMER_0_10_CFLAGS)		\
	$(GSTREAMER_1_0_CFLAGS)			\
	$(SPICE_PROTOCOL_CFLAGS)		\
	$(SSL_CFLAGS)				\
	$(VISIBILITY_HIDDEN_CFLAGS)		\
	$(WARN_CFLAGS)				\
	$(NULL)

lib_LTLIBRARIES = libspice-server.la

libspice_server_la_LDFLAGS =			\
	-version-info $(SPICE_LT_VERSION)	\
	-no-undefined				\
	$(RELRO_LDFLAGS)			\
	$(NO_INDIRECT_LDFLAGS)			\
	$(NULL)

if HAVE_LD_VERSION_SCRIPT
libspice_server_la_LDFLAGS += \
	-Wl,--version-script=$(top_srcdir)/server/spice-server.syms
endif

libspice_server_la_LIBADD =						\
	$(top_builddir)/spice-common/common/libspice-common.la		\
	$(top_builddir)/spice-common/common/libspice-common-server.la	\
	$(GL_LIBS)							\
	$(GLIB2_LIBS)							\
	$(JPEG_LIBS)							\
	$(LZ4_LIBS)							\
	$(LIBRT)							\
	$(PIXMAN_LIBS)							\
	$(SASL_LIBS)							\
	$(SLIRP_LIBS)							\
	$(GSTREAMER_0_10_LIBS)						\
	$(GSTREAMER_1_0_LIBS)						\
	$(SSL_LIBS)							\
	$(Z_LIBS)							\
	$(SPICE_NONPKGCONFIG_LIBS)					\
	$(NULL)

libspice_serverincludedir = $(includedir)/spice-server
libspice_serverinclude_HEADERS =		\
	spice-audio.h				\
	spice-char.h				\
	spice-core.h				\
	spice-experimental.h			\
	spice-input.h				\
	spice-migration.h			\
	spice-qxl.h				\
	spice-server.h				\
	spice-version.h				\
	spice.h					\
	$(NULL)

libspice_server_la_SOURCES =			\
	$(libspice_serverinclude_HEADERS)	\
	agent-msg-filter.c			\
	agent-msg-filter.h			\
	cache-item.h				\
	char_device.c				\
	char_device.h				\
	demarshallers.h				\
	glz_encoder.c				\
	glz_encoder.h				\
	glz_encoder_config.h			\
	glz_encoder_dictionary.c		\
	glz_encoder_dictionary.h		\
	glz_encoder_dictionary_protected.h	\
	inputs_channel.c			\
	inputs_channel.h			\
	jpeg_encoder.c				\
	jpeg_encoder.h				\
	lz4_encoder.c				\
	lz4_encoder.h				\
	main_channel.c				\
	main_channel.h				\
	mjpeg_encoder.c				\
	red_bitmap_utils.h			\
	red_channel.c				\
	red_channel.h				\
	red_common.h				\
	dispatcher.c				\
	dispatcher.h				\
	red_dispatcher.c			\
	red_dispatcher.h			\
	main_dispatcher.c			\
	main_dispatcher.h			\
	migration_protocol.h		\
	red_memslots.c				\
	red_memslots.h				\
	red_parse_qxl.c				\
	red_record_qxl.c			\
	red_record_qxl.h			\
	red_replay_qxl.c			\
	red_replay_qxl.h			\
	red_parse_qxl.h				\
	red_worker.c				\
	red_worker.h				\
	display-channel.c			\
	display-channel.h			\
	cursor-channel.c			\
	cursor-channel.h			\
	reds.c					\
	reds.h					\
	reds-private.h				\
	reds_stream.c				\
	reds_stream.h				\
	reds_sw_canvas.c			\
	reds_sw_canvas.h			\
	snd_worker.c				\
	snd_worker.h				\
	stat.h					\
	spicevmc.c				\
	spice_timer_queue.c			\
	spice_timer_queue.h			\
	video_encoder.h				\
	zlib_encoder.c				\
	zlib_encoder.h				\
	spice_bitmap_utils.h		\
	spice_bitmap_utils.c		\
	spice_server_utils.h		\
	spice_image_cache.h			\
	spice_image_cache.c			\
	pixmap-cache.h				\
	pixmap-cache.c				\
	tree.h				\
	tree.c				\
	spice-bitmap-utils.h			\
	spice-bitmap-utils.c			\
	utils.h					\
	stream.c					\
	stream.h					\
	ffmpeg_encoder.c
	$(NULL)

if HAVE_GL
libspice_server_la_SOURCES +=	\
	reds_gl_canvas.c	\
	reds_gl_canvas.h	\
	$(NULL)
endif

if HAVE_SMARTCARD
libspice_server_la_SOURCES +=	\
	smartcard.c		\
	smartcard.h		\
	$(NULL)
endif

if SUPPORT_GSTREAMER_0_10
libspice_server_la_SOURCES +=	\
	gstreamer_encoder.c			\
	$(NULL)
endif

if SUPPORT_GSTREAMER_1_0
libspice_server_la_SOURCES +=			\
	gstreamer_encoder.c			\
	$(NULL)
endif

EXTRA_DIST =					\
	red_bitmap_utils_tmpl.c			\
	cache_item.tmpl.c			\
	glz_encode_match_tmpl.c			\
	glz_encode_tmpl.c			\
	spice-server.syms			\
	$(NULL)

BUILT_SOURCES = $(spice_built_sources)
