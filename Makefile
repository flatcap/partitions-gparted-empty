CC	= g++
RM	= rm -fr

SRC	= device.cc drawingareavisualdisk.cc filesystem.cc gparted_core.cc \
	  hboxoperations.cc main.cc operation.cc partition.cc \
	  proc_partitions_info.cc treeview_detail.cc utils.cc win_gparted.cc

HDR	= config.h device.h drawingareavisualdisk.h filesystem.h \
	  gparted_core.h hboxoperations.h i18n.h operation.h partition.h \
	  proc_partitions_info.h treeview_detail.h utils.h win_gparted.h

PACKAGES = atk atkmm-1.6 cairo cairomm-1.0 freetype2 gdkmm-2.4 gdk-pixbuf-2.0 \
	   giomm-2.4 glib-2.0 glibmm-2.4 gtkmm-2.4 gtk+-unix-print-2.0 \
	   libparted libpng pangomm-1.4 pango pixman-1 sigc++-2.0 uuid

OBJ	= $(SRC:%.cc=%.o)

OUT	= gparted

CFLAGS	= -g -Wall

CFLAGS	+= $(shell pkg-config --cflags $(PACKAGES))
LDFLAGS += $(shell pkg-config --libs   $(PACKAGES))

.cc.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT):	$(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

all:	$(OBJ) $(OUT)

clean:
	$(RM) $(OBJ) $(OUT)

distclean: clean

