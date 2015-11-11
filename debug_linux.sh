echo Debug Texture Packer.
PREPRO=DEBUG
SOURCE="source/main.c "
SOURCE=$SOURCE"source/texturepacker.c "
SOURCE=$SOURCE"source/strtools.c "
SOURCE=$SOURCE"source/filetools.c "
SOURCE=$SOURCE"source/mantxt.c "
SOURCE=$SOURCE"source/manc.c "
SOURCE=$SOURCE"source/manjava.c "
SOURCE=$SOURCE"source/utils.c "
SOURCE=$SOURCE"source/squarefit.c "
SOURCE=$SOURCE"source/font.c "
FREETYPE=`freetype-config --cflags --libs`
GLIB=`pkg-config --libs --cflags glib-2.0`
gcc $SOURCE -g -o tpak $FREETYPE $GLIB -lpng -Wall -O0 -D$PREPRO
ctags ./source/*
mkdir output
