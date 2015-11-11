Texture Packer - Read Me

By Stuart Bridgens, 2013

All the code for this project is licensed under GNU GPL version 3, which has a copy in the root dir for
your reading pleasure.

---------------
Introduction.
---------------
This commandline utility is here to help you pack all your individual png images onto an organized 
collection of sprite sheets. As well as outputting a manifest file that lets you know where your images 
are.

This functionality is aimed at gamedevs who sometimes use lots of individual images for their 2D games and
animations. So, Nifty has a simple naming convention to help it understand the difference between a static
single image and a sequence used to make an animation. You can also place your images in sub directories
to create different packages.

---------------
Installation
---------------
If you downloaded a precompiled binary, There shouldn't be anything else you need to install, just chuck
the binary somewhere.

---------------
Compilation.
---------------

You'll need to download, compile and install the freetype library, which can be found here:
www.freetype.org/download.html
Open up the readmes inside the documents folder, and you should be able to get it working.
Remeber to type this in at the command prompt:
freetype-config --prefix
To check if everything is working, where you'll see the install location if it is.



---------------
Use
---------------
When compiled you invoke the program from the command line. If run without any options it searches the 
directory it's in, and any sub directories, for png files and outputs a manifest called 'output', and a 
number of png sheet images called 'outputN.png' where N is the number for that sheet. Any scanned sub
directories will be treated as different outputs, so they take their manifest and sheet names from the
name of the sub directory. To use options, type it in with a space between each option and value.

-d [value] Specify the directory you want to scan.

-o [value] The name for output and the directory you want to output to. For instance '-o output/test' 
   will scan the 'output' directory (which in this example is a directory in the same one as the packer). 
   It uses 'test' for the name of the manifest file and the output sheets.
   
-p Output sheets are all padded to have sizes that are to a power of 2.

-s Specifies the max size our sheets can be. The default is 1024.

-f [value] Lets you choose an alternative format for the manifest output, you have some set choices for
   what values you can use.
   ---java Outputs a java class file
   ---c++ or cpp This is yet to be developed.
   
-class [value] When using java (and in the future c++), the value becomes the class name.

-jpak [value] When using java, this is the package name.

An example for someone who wants to use a sub directory for input, and what's the output to be called 
'fluffy', would look like this:
$./tpak -d subdir -o fluffy

---------------
Help or Helping
---------------
We'd love to hear from you, so if you have a problem with the software or you want to contribute, come on
over to:
texturepacker.sourceforge.net
And feel free to post in the discussion or add a help ticket.
