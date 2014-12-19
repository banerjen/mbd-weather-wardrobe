
######################################################################

Read April Tags using Apriltag-C standalone library

######################################################################

Change Log:

11-6-2014:
Apriltag-C library was downloaded from the following link:
http://april.eecs.umich.edu/wiki/index.php/AprilTags
Version: 2014-10-20

Make file was altered to change the compiler to the cross compiler.



Instructions:
1) Download, extract and compile using make.
2) SCP the executable apriltag_demo to the galileo
3) SSH into the galileo and run ./apriltag_demo <test image file>
   The test image needs to be in the format: .PBM, .PGM, or .PPM

