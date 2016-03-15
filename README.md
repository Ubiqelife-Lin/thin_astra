#thin_astra

ROS Node for the ORBBEC Astra RGBD camera, following the thin philosophy

Dependencies:
- ORBBEC custom OPENNI2 (https://www.dropbox.com/sh/0vf3dz3whmeuvb9/AAArKrzGTBxfvZW2jfMQJRsia?dl=0)

##Preliminaries
- Download the ORBBEC openni2 library from the link
- Test if the device is working running the example OpenNI-Linux-x64|32-2.2/Samples/Bin/SimpleViewer
- If it's working, go to next section. 
- If it's not working, try running by root. If it runs as root, you have to add an udev rule (i provided an example in this repository)
- If it's not working even as root the situation is a bit more complicated, spend a couple of mins/hours/days looking for a solution around, there are multiple causes out there that could make your life miserable (usb/kernel/bad luck)

##Preliminaries, the wrath of sync
- By deafault the device (fw 1.05) does not provide hardware sync.
- But hardware sync is good. We really want it.
- Guys at ORBBEC provided an updated firmware here (https://www.dropbox.com/sh/8av03t5o5co3np7/AACkfn4JYIIh6UsONpfkwFiba?dl=0), please read all the discussion here (https://3dclub.orbbec3d.com/t/rgb-d-sync-project/265)
- HARDWARE SYNC DEPENDS ON THIS!!!!

##How to compile

- uncompress in ~/src (if you want to put them somewhere else change lines 19-21 in CMakeLists.txt)
- compile using catkin_make (or catkin build)

##How to use

###CHECKING THE AVAILABLE PARAMETERS
`rosrun thin_astra thin_astra_node`

### GIMME THE DEPTH
`rosrun thin_astra thin_astra_node _device _device_num:=0 _depth_mode:=0` //if you have just one device and you want depth in mm

### GIMME THE COLORS
`rosrun thin_astra thin_astra_node _device _device_num:=0  _rgb_mode:=0 `//if you have just one device and you really want just the colors

### GIMME BOTH!
`rosrun thin_astra thin_astra_node _device _device_num:=0  _rgb_mode:=0 _depth_mode:=0`

### I WANT COLORS, DEPTH, SYNCHRONIZATION AND REGISTRATION 
`rosrun thin_astra thin_astra_node _device _device_num:=0  _rgb_mode:=0 _depth_mode:=0 _sync:=1 _registration:=1`





