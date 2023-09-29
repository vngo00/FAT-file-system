# File System



The team have been designing components of a file system.  We have defined the goals and designed the directory entry structure, the volume structure and the free space.  Now it is time to implement our file system.

The project is in three phases.  The first phase is the "formatting" the volume.  This is further described in the steps for phase one and the phase one assignment.

The second phase is the implementation of the directory based functions.  See Phase two assignment.

The final phase is the implementation of the file operations.



A shell program designed to demonstrate the file system called fsshell.c is proviced.  It has a number of built in functions that will work if the implement the above interfaces, these are:
```
ls - Lists the file in a directory
cp - Copies a file - source [dest]
mv - Moves a file - source dest
md - Make a new directory
rm - Removes a file or directory
touch - creates a file
cat - (limited functionality) displays the contents of a file
cp2l - Copies a file from the test file system to the linux file system
cp2fs - Copies a file from the Linux file system to the test file system
cd - Changes directory
pwd - Prints the working directory
history - Prints out the history
help - Prints out help
```




