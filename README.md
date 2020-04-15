# buildref

Copyright 1996 Paul Sander, all rights reserved.

<a rel="license" href="http://creativecommons.org/licenses/by/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by/4.0/88x31.png" /></a><br /><span xmlns:dct="http://purl.org/dc/terms/" property="dct:title">buildref</span> by <a xmlns:cc="http://creativecommons.org/ns#" href="https://github.com/paulmsander/software-chipset" property="cc:attributionName" rel="cc:attributionURL">Paul M. Sander</a> is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by/4.0/">Creative Commons Attribution 4.0 International License</a>.
Permission is granted to distribute, remix, tweak, and build upon this work, provided credit is given to the original author.

The **buildref** project provides source code for build reference tools with navigation assistance.  It provides two tools:  The **bldref** tool creates and edits references to build sandboxes in the filesystem, and the **paths** tool locates named resources within the referenced builds and produces lists of paths to those resources.  This puts a layer of abstraction over builds such that projects that consume their artifacts need not know the internal structure of the referenced builds.  This allows the owner of the referenced builds to reorganize or rename artifacts within the referenced builds in such a way that the consumers of those artifacts need not change their build procedures.

To create a build, the user invokes the **bldref** program, providing fullpaths to the sandboxes of each referenced builds.  Alternatively, these references could be to unpacked archives (e.g. archives that might have been downloaded from the Internet).

Referenced builds contain files that map named resources to artifacts (files or directories) with the build area.  To locate a resource, the user runs the **paths** program specifying the list of build references, and the name of the resource.  The output of the program is one or more fullpaths consisting of the build in which the resource was found, appended with the location of the named resource within that build.

This software requires [Software ChipSet](https://github.com/paulmsander/software-chipset) to be preinstalled in such a way that it can be found by the compiler's and linker's default settings, usually in the /usr/local tree.  To build this software, download it to an empty directory and run "make".  To install the software in the /usr/local tree, run "make install" (possibly with super-user permissions).

To use this software, read the man page for the **paths** program and create a **.bldstate/search** file in a directory to be referenced.  Then read the man page for the **bldref** program and create a **.bldref** file containing a reference to the above directory.  Finally, running the **paths** program to locate the resources named in the referenced directory.

Normally, the **.bldstate/search** file is checked in to version control and distributed with the results of a build.  If the build is distributed in some kind of archive, it is included in the archive.

The **.bldref** file can be written by hand, but more often it is computed (possibly by implementing a referencing algorithm using some kind of promotion system for builds).  If the build system involves downloading archives from the Internet, the references would be fullpaths to the directories where the archives were unpacked.

