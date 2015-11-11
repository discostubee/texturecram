echo Compiling: Texture Packer -debug-
PREPRO="-DDEBUG -DXTRA_LOGGING"
SOURCE="source/main.c "
SOURCE=$SOURCE"source/texturepacker.c "
SOURCE=$SOURCE"source/strtools.c "
SOURCE=$SOURCE"source/filetools.c "
SOURCE=$SOURCE"source/mantxt.c "
SOURCE=$SOURCE"source/manjava.c "
SOURCE=$SOURCE"source/manc.c "
SOURCE=$SOURCE"source/utils.c "
SOURCE=$SOURCE"source/squarefit.c "
SOURCE=$SOURCE"source/font.c "
echo Source: $SOURCE
FREETYPE=`freetype-config --cflags --libs`
X11="-I/usr/X11/include -L/usr/X11/lib"
gcc $SOURCE -g -o tpak $FREETYPE $X11 -lpng -framework CoreFoundation -framework CoreServices -Wall -O0 $PREPRO
ctags ./source/*
mkdir output
#echo run -d bin -o output/test -f java -p | gdb tpak
echo Finished.
