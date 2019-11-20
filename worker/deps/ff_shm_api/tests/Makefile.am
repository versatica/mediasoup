bin_PROGRAMS = test_reader

test_reader_SOURCES = ffngxshm_test_reader.c
test_reader_CFLAGS = \
	-I../include \
	-I$(BASE_LIB_DIR)/include \
	-I$(BASE_LIB_DIR)/include/nginx/core \
	-I$(BASE_LIB_DIR)/include/nginx/nginx-shm-module \
	-I$(BASE_LIB_DIR)/include/nginx/objs \
	-I$(BASE_LIB_DIR)/include/nginx/os/unix

## test_reader_LDFLAGS = -drm -x11 -pthread
## -lfribidi \ -lvdpau \

test_reader_LDFLAGS = -static

test_reader_LDADD = \
	-L../src \
	-L$(BASE_LIB_DIR)/lib \
	-lffngxshm \
	-lngxshm \
	-lm -lz -ldl -lpcre -lcrypto -lrt \
	-L$(BASE_LIB_DIR)/lib -lavformat -lm -lbz2 -lz -lavcodec -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -lm -lz -lfdk-aac -lm -lmp3lame -lm -lopus -lm -lspeex -lm -ltheoraenc -ltheoradec -logg -lvorbis -lm -logg -lvorbisenc -lvorbis -lm -logg -lx264 -lpthread -lm -lswresample -lm -lavutil -lm \
	-L$(BASE_LIB_DIR)/lib -lavcodec -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -lm -lz -lfdk-aac -lm -lmp3lame -lm -lopus -lm -lspeex -lm -ltheoraenc -ltheoradec -logg -lvorbis -lm -logg -lvorbisenc -lvorbis -lm -logg -lx264 -lpthread -lm -lswresample -lm -lavutil -lm \
	-L$(BASE_LIB_DIR)/lib -lswresample -lm -lavutil -lm \
	-L$(BASE_LIB_DIR)/lib -lavutil -lm \
	-lX11 \
	-lavfilter -lm -lfreetype -lbz2 -lpng16 -lm -lz -lm -lz -lswscale -lm -lpostproc -lm -lavformat -lm -lbz2 -lz -lavcodec -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -lm -lz -lfdk-aac -lm -lmp3lame -lm -lopus -lm -lspeex -lm -ltheoraenc -ltheoradec -logg -lvorbis -lm -logg -lvorbisenc -lvorbis -lm -logg -lx264 -lpthread -lm -lswresample -lm -lavutil -lm \
	-lass \
	-lharfbuzz \
	-lfontconfig \
	-lexpat \
	-lfreetype \
	-lpng \
	-lswscale -lm -lavutil -lm \
	-lpostproc
