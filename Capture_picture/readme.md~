C-code for capturing image from webcam attached to Galileo
Source: https://gist.github.com/bellbind/6813905

Changelog:
11-7-2014:
change the following code: camera_t* camera = camera_open("/dev/video0", 352, 288);
Replacement code: camera_t* camera = camera_open("/dev/video0", 640, 480);


Instructions:
Note: printenv | grep CC to see current environment
Compiling for Linux use
1) Use: gcc -std=c99 capture.c -ljpeg -o capture
2) To run: ./capture
3) An image named result.jpg is created 
Compiling for Galileo use
1) Set up compiler to use: CC=i586-poky-linux-gcc  -m32 -march=i586 --sysroot=/opt/clanton-full/1.4.2/sysroots/i586-poky-linux
1*) It will probably give an error saying Fatal Python error about No module named 'encoding' Just ignore.
2) To Compile: $CC -std=c99 capture.c -ljpeg -o cap.o
3) SCP cap.o to galileo
4) To Run: ./cap.o
5) An image named result.jpg is created
	
