CC	= g++
RM	= rm -fr

SRC	= device.cc drawingareavisualdisk.cc filesystem.cc gparted_core.cc hboxoperations.cc \
	  main.cc operation.cc partition.cc proc_partitions_info.cc treeview_detail.cc \
	  utils.cc win_gparted.cc

HDR	= config.h device.h drawingareavisualdisk.h filesystem.h gparted_core.h hboxoperations.h \
	  i18n.h operation.h partition.h proc_partitions_info.h treeview_detail.h utils.h win_gparted.h

OBJ	= $(SRC:%.cc=%.o)

OUT	= gparted

CFLAGS	= -g -Wall

CFLAGS	+= -I.
CFLAGS	+= -pthread

CFLAGS	+= -I/usr/include/atk-1.0
CFLAGS	+= -I/usr/include/atkmm-1.6
CFLAGS	+= -I/usr/include/cairo
CFLAGS	+= -I/usr/include/cairomm-1.0
CFLAGS	+= -I/usr/include/freetype2
CFLAGS	+= -I/usr/include/gdk-pixbuf-2.0
CFLAGS	+= -I/usr/include/gdkmm-2.4
CFLAGS	+= -I/usr/include/giomm-2.4
CFLAGS	+= -I/usr/include/glib-2.0
CFLAGS	+= -I/usr/include/glibmm-2.4
CFLAGS	+= -I/usr/include/gtk-2.0
CFLAGS	+= -I/usr/include/gtk-unix-print-2.0
CFLAGS	+= -I/usr/include/gtkmm-2.4
CFLAGS	+= -I/usr/include/libpng12
CFLAGS	+= -I/usr/include/pango-1.0
CFLAGS	+= -I/usr/include/pangomm-1.4
CFLAGS	+= -I/usr/include/pixman-1
CFLAGS	+= -I/usr/include/sigc++-2.0

CFLAGS	+= -I/usr/lib64/cairomm-1.0/include
CFLAGS	+= -I/usr/lib64/gdkmm-2.4/include
CFLAGS	+= -I/usr/lib64/giomm-2.4/include
CFLAGS	+= -I/usr/lib64/glib-2.0/include
CFLAGS	+= -I/usr/lib64/glibmm-2.4/include
CFLAGS	+= -I/usr/lib64/gtk-2.0/include
CFLAGS	+= -I/usr/lib64/gtkmm-2.4/include
CFLAGS	+= -I/usr/lib64/pangomm-1.4/include
CFLAGS	+= -I/usr/lib64/sigc++-2.0/include

LDFLAGS	+= -latk-1.0
LDFLAGS	+= -latkmm-1.6
LDFLAGS	+= -lcairo
LDFLAGS	+= -lcairomm-1.0
LDFLAGS	+= -ldl
LDFLAGS	+= -lfontconfig
LDFLAGS	+= -lfreetype
LDFLAGS	+= -lgdkmm-2.4
LDFLAGS	+= -lgdk_pixbuf-2.0
LDFLAGS	+= -lgdk-x11-2.0
LDFLAGS	+= -lgio-2.0
LDFLAGS	+= -lgiomm-2.4
LDFLAGS	+= -lglib-2.0
LDFLAGS	+= -lglibmm-2.4
LDFLAGS	+= -lgmodule-2.0
LDFLAGS	+= -lgobject-2.0
LDFLAGS	+= -lgthread-2.0
LDFLAGS	+= -lgtkmm-2.4
LDFLAGS	+= -lgtk-x11-2.0
LDFLAGS	+= -lpango-1.0
LDFLAGS	+= -lpangocairo-1.0
LDFLAGS	+= -lpangoft2-1.0
LDFLAGS	+= -lpangomm-1.4
LDFLAGS	+= -lparted
LDFLAGS	+= -lrt
LDFLAGS	+= -lsigc-2.0
LDFLAGS	+= -luuid
LDFLAGS	+= -pthread

.cc.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT):	$(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

all:	$(OBJ) $(OUT)

clean:
	$(RM) $(OBJ) $(OUT)

distclean: clean

