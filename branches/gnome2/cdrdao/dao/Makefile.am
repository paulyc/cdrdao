noinst_LIBRARIES = libdao.a

bin_PROGRAMS = cdrdao

cdrdao_SOURCES = \
	main.cc

libdao_a_SOURCES = \
	main.cc			\
	dao.cc			\
	port.cc			\
	data.cc			\
	CdrDriver.cc		\
	CDD2600Base.cc		\
	CDD2600.cc		\
	PlextorReader.cc	\
	GenericMMC.cc		\
	SubChannel.cc		\
	PQSubChannel16.cc	\
	PWSubChannel96.cc	\
	PQChannelEncoder.cc	\
	GenericMMCraw.cc	\
	PlextorReaderScan.cc	\
	RicohMP6200.cc		\
	ScsiIf-lib.cc		\
	TaiyoYuden.cc		\
	YamahaCDR10x.cc		\
	TeacCdr55.cc		\
	SonyCDU920.cc		\
	SonyCDU948.cc		\
	ToshibaReader.cc	\
	CdTextEncoder.cc	\
	Settings.cc

EXTRA_libdao_a_SOURCES = 	\
	dao-win32.cc		\
	decodeSense.cc		\
	sg_err.cc

cdrdao_LDADD =						\
	-L. -ldao					\
	@thread_libs@					\
	-L$(top_builddir)/paranoia -lcdda_paranoia	\
	-L$(top_builddir)/trackdb -ltrackdb		\
	@scsilib_libs@

cdrdao_DEPENDENCIES = ../paranoia/libcdda_paranoia.a libdao.a

CDRDAO_DATA_DIR = $(datadir)/cdrdao
DRIVER_TABLE_FILE = $(CDRDAO_DATA_DIR)/drivers

INCLUDES =						\
	-I$(srcdir)/../trackdb				\
	-I$(srcdir)/../paranoia				\
	@scsilib_incl@

AM_CXXFLAGS = -DDRIVER_TABLE_FILE=\"$(DRIVER_TABLE_FILE)\"

man1_MANS = cdrdao.man
EXTRA_DIST = $(man1_MANS)
