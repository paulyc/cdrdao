bin_PROGRAMS = toc2cue toc2mp3
toc2cue_SOURCES = toc2cue.cc
toc2mp3_SOURCES = toc2mp3.cc

INCLUDES = -I$(top_builddir)/trackdb
LDADD = -L$(top_builddir)/trackdb -ltrackdb @LAME_LIBS@
AM_CXXFLAGS = @LAME_CFLAGS@
