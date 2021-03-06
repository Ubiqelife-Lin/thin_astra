#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/CameraInfo.h>
#include <sensor_msgs/distortion_models.h>
#include <stdio.h>
#include <iostream>
#include <OpenNI.h>
#include <stdlib.h> 
#include <string>

using namespace openni;
using namespace std;

//IMAGE
ros::Publisher pub_depth;// = n.advertise<sensor_msgs::Image>("/"+topic+"/depth/image_raw", 1);
ros::Publisher pub_rgb;// = n.advertise<sensor_msgs::Image>("/"+topic+"/rgb/image_raw", 1);
//CAMERA INFO	
ros::Publisher pub_camera_info_depth;// = n.advertise<sensor_msgs::CameraInfo>("/"+topic+"/depth/camera_info", 1);
ros::Publisher pub_camera_info_rgb;// = n.advertise<sensor_msgs::CameraInfo>("/"+topic+"/rgb/camera_info", 1);

volatile bool run;

VideoStream depth;
VideoStream rgb;
openni::VideoStream** streams;


string topic;
string frame_id;
int _depth_mode;
int _device_num;
string _device_uri;
int _rgb_mode;
int _registration;
int _sync;
int _frame_skip;
int _exposure;
int _gain;

int gain_status=0;

void* camera_thread(void*){
  VideoFrameRef frame;
  VideoFrameRef rgbframe;
  int changed_stream;
  VideoStream* pStream;
  VideoStream* pRgbStream;
  sensor_msgs::CameraInfo rgb_info;
  sensor_msgs::CameraInfo depth_info;
  sensor_msgs::Image image;
  image.header.frame_id=frame_id;
  image.is_bigendian=0;

  int count = 0;
  int num_frames_stat=64;
  struct timeval previous_time;
  gettimeofday(&previous_time, 0);
  uint64_t last_stamp = 0;
  while(run){
    bool new_frame = false;
    uint64_t current_stamp = 0;
    openni::OpenNI::waitForAnyStream(streams, 2, &changed_stream);
    switch (changed_stream)
      {
	//DEPTH
      case 0:
	depth.readFrame(&frame);
	depth_info.header.stamp=ros::Time::now();
	image.header.stamp=ros::Time::now();
	if(!frame.isValid()) break;
	current_stamp=frame.getTimestamp();

	if (current_stamp-last_stamp>5000){
	  count++;
	  new_frame = true;
	}
	last_stamp = current_stamp;

	if (! (count % (_frame_skip)) ){
	  image.header.seq = count;
	  if (! _registration){
	    image.header.frame_id=frame_id+"_depth";
	  } else 
	    image.header.frame_id=frame_id+"_rgb";

	  depth_info.width=frame.getWidth();
	  depth_info.height=frame.getHeight();

	  depth_info.header.seq = count;

    depth_info.distortion_model = sensor_msgs::distortion_models::PLUMB_BOB;
    depth_info.D.resize(5, 0.0);

	  depth_info.K[0]=depth_info.width/(2*tan(depth.getHorizontalFieldOfView()/2)); //fx
	  depth_info.K[4]=depth_info.height/(2*tan(depth.getVerticalFieldOfView()/2));; //fy
	  depth_info.K[2]=depth_info.width/2; //cx
	  depth_info.K[5]=depth_info.height/2; //cy
	  depth_info.K[8]=1;

	  depth_info.header.frame_id=image.header.frame_id;
	  image.height=frame.getHeight();
	  image.width=frame.getWidth();
	  image.encoding="mono16";
	  image.step=frame.getWidth()*2;
	  image.data.resize(image.step*image.height);
	  memcpy((char*)(&image.data[0]),frame.getData(),image.height*image.width*2);
	  pub_depth.publish(image);
	  pub_camera_info_depth.publish(depth_info);
	}
	break;
	//RGB
      case 1:
	rgb.readFrame(&rgbframe);
	image.header.stamp=ros::Time::now();
	rgb_info.header.stamp=ros::Time::now();
	if(!rgbframe.isValid()) break;
	current_stamp=rgbframe.getTimestamp();

	if (current_stamp-last_stamp>5000){
	  count++;
	  new_frame = true;
	}
	last_stamp = current_stamp;

	if (_gain>=0) {
	  if (count > 64 && gain_status==0){
	    rgb.getCameraSettings()->setAutoExposureEnabled(false);
	    rgb.getCameraSettings()->setAutoWhiteBalanceEnabled(false);
	    rgb.getCameraSettings()->setExposure(1);
	    rgb.getCameraSettings()->setGain(100);
	    gain_status=1;
	  } else if (count > 128 && gain_status==1){
	    rgb.getCameraSettings()->setExposure(_exposure);
	    rgb.getCameraSettings()->setGain(_gain);
	    gain_status=2;
	  }
	}

	if (! (count%(_frame_skip)) ){
	  rgb_info.header.frame_id=frame_id+"_rgb";
	  rgb_info.width=rgbframe.getWidth();
	  rgb_info.height=rgbframe.getHeight();
	  rgb_info.header.seq = count;
	  rgb_info.K[0]=rgb_info.width/(2*tan(rgb.getHorizontalFieldOfView()/2)); //fx
	  rgb_info.K[4]=rgb_info.height/(2*tan(rgb.getVerticalFieldOfView()/2));; //fy
	  rgb_info.K[2]=rgb_info.width/2; //cx
	  rgb_info.K[5]=rgb_info.height/2; //cy
	  rgb_info.K[8]=1;
	  
	  image.header.seq = count;
	  image.header.frame_id=frame_id+"_rgb";
	  image.height=rgbframe.getHeight();
	  image.width=rgbframe.getWidth();
	  image.encoding="rgb8";
	  image.step=rgbframe.getWidth()*3;
	  image.data.resize(image.step*image.height);
	  memcpy((char*)(&image.data[0]),rgbframe.getData(),image.height*image.width*3);
	  pub_rgb.publish(image);
	  pub_camera_info_rgb.publish(rgb_info);
	}
	break;
      default:
	printf("Error in wait\n");
      }


    if (!(count%num_frames_stat) && new_frame){
      struct timeval current_time, interval;
      gettimeofday(&current_time, 0);
      timersub(&current_time, &previous_time, &interval);
      previous_time = current_time;
      double fps = num_frames_stat/(interval.tv_sec +1e-6*interval.tv_usec);
      printf("running at %lf fps\n", fps);
      fflush(stdout);
    }
  }
}

int main(int argc, char **argv){


  printf("starting\n");
  fflush(stdout);
  ros::init(argc, argv, "xtion",ros::init_options::AnonymousName);
  ros::NodeHandle n("~");

  //Base topic name
  n.param("topic", topic, string("/camera"));
  //Resolution
  //0 = 160x120
  //1 = 320x240
  n.param("depth_mode", _depth_mode, -1);
  n.param("rgb_mode", _rgb_mode, -1);
  n.param("sync", _sync, 0);
  n.param("registration", _registration,0);
  n.param("frame_id", frame_id, string("camera_frame"));
  n.param("device_num", _device_num, -1);
  n.param("device_uri", _device_uri, string("NA"));
  n.param("frame_skip", _frame_skip, 0);
  n.param("exposure", _exposure, -1);
  n.param("gain", _gain, -1);

  printf("Launched with params:\n");
  printf("_device_num:= %d\n",_device_num);
  printf("_device_uri:= %s\n",_device_uri.c_str());
  printf("_topic:= %s\n",topic.c_str());
  printf("_sync:= %d\n",_sync);
  printf("_registration:= %d\n",_registration);
  printf("_depth_mode:= %d\n",_depth_mode);
  printf("_rgb_mode:= %d\n",_rgb_mode);
  printf("_frame_id:= %s\n",frame_id.c_str());
  printf("_frame_skip:= %d\n",_frame_skip);
  printf("_exposure:= %d\n",_exposure);
  printf("_gain:= %d\n",_gain);
  
  fflush(stdout);


  if (_frame_skip<=0)
    _frame_skip = 1;

	
  //OPENNI2 STUFF
  //===================================================================
  streams = new openni::VideoStream*[2];
  streams[0]=&depth;
  streams[1]=&rgb;

  Status rc = OpenNI::initialize();
  if (rc != STATUS_OK)
    {
      printf("Initialize failed\n%s\n", OpenNI::getExtendedError());
      fflush(stdout);
      return 1;
    }

  // enumerate the devices
  openni::Array<openni::DeviceInfo> device_list;
  openni::OpenNI::enumerateDevices(&device_list);
  
  Device device;
  
  if(_device_uri.compare("NA")){
    
    string dev_uri("NA");
    
    for (int i = 0; i<device_list.getSize(); i++){
      if(!string(device_list[i].getUri()).compare(0, _device_uri.size(), _device_uri )){
	dev_uri = device_list[i].getUri();
	break;
      }
    }
    if(!dev_uri.compare("NA")){
      cerr << "cannot find device with uri starting for: " << _device_uri << endl;
    }
    rc = device.open(dev_uri.c_str());
  }
  else{
    
    if (_device_num < 0){
      cerr << endl << endl << "found " << device_list.getSize() << " devices" << endl;
      for (int i = 0; i<device_list.getSize(); i++)
	cerr << "\t num: " << i << " uri: " << device_list[i].getUri() << endl;
    }

    if (_device_num>=device_list.getSize() || _device_num<0 ) {
      cerr << "device num: " << _device_num << " does not exist, aborting" << endl;
      openni::OpenNI::shutdown();
      return 0;
    }

    rc = device.open(device_list[_device_num].getUri());
  }
  
  if (rc != STATUS_OK){
    printf("Couldn't open device\n%s\n", OpenNI::getExtendedError());
    fflush(stdout);
    return 2;
  }
	

  if(_depth_mode>=0){
    if (device.getSensorInfo(SENSOR_DEPTH) != NULL){
      rc = depth.create(device, SENSOR_DEPTH);
      if (rc != STATUS_OK){
	printf("Couldn't create depth stream\n%s\n", OpenNI::getExtendedError());
	fflush(stdout);
	return 3;
      }
      //DEPTH
      pub_depth = n.advertise<sensor_msgs::Image>("/"+topic+"/depth/image_raw", 1);
      pub_camera_info_depth = n.advertise<sensor_msgs::CameraInfo>("/"+topic+"/depth/camera_info", 1);

    }
  }


  if(_rgb_mode>=0){
    if (device.getSensorInfo(SENSOR_COLOR) != NULL){
      rc = rgb.create(device, SENSOR_COLOR);
      if (rc != STATUS_OK){
	printf("Couldn't create rgb stream\n%s\n", OpenNI::getExtendedError());
	fflush(stdout);
	return 3;
      }
      //RGB
      pub_rgb = n.advertise<sensor_msgs::Image>("/"+topic+"/rgb/image_raw", 1);
      pub_camera_info_rgb = n.advertise<sensor_msgs::CameraInfo>("/"+topic+"/rgb/camera_info", 1);
    }

  }
	

  if(_depth_mode<0 && _rgb_mode<0){
    cout << "Depth modes" << endl;
    const openni::SensorInfo* sinfo = device.getSensorInfo(openni::SENSOR_DEPTH); // select index=4 640x480, 30 fps, 1mm
    const openni::Array< openni::VideoMode>& modesDepth = sinfo->getSupportedVideoModes();

    printf("Enums data:\nPIXEL_FORMAT_DEPTH_1_MM = 100,\nPIXEL_FORMAT_DEPTH_100_UM = 101,\nPIXEL_FORMAT_SHIFT_9_2 = 102,\nPIXEL_FORMAT_SHIFT_9_3 = 103,\nPIXEL_FORMAT_RGB888 = 200,\nPIXEL_FORMAT_YUV422 = 201,\nPIXEL_FORMAT_GRAY8 = 202,\nPIXEL_FORMAT_GRAY16 = 203,\nPIXEL_FORMAT_JPEG = 204,\nPIXEL_FORMAT_YUYV = 205,\n\n");
    cout << "Depth modes" << endl;
    for (int i = 0; i<modesDepth.getSize(); i++) {
      printf("%i: %ix%i, %i fps, %i format\n", i, modesDepth[i].getResolutionX(), modesDepth[i].getResolutionY(),modesDepth[i].getFps(), modesDepth[i].getPixelFormat()); //PIXEL_FORMAT_DEPTH_1_MM = 100, PIXEL_FORMAT_DEPTH_100_UM = 101
    }

    cout << "Rgb modes" << endl;
    const openni::SensorInfo* sinfoRgb = device.getSensorInfo(openni::SENSOR_COLOR); // select index=4 640x480, 30 fps, 1mm
    const openni::Array< openni::VideoMode>& modesRgb = sinfoRgb->getSupportedVideoModes();
    for (int i = 0; i<modesRgb.getSize(); i++) {
      printf("%i: %ix%i, %i fps, %i format\n", i, modesRgb[i].getResolutionX(), modesRgb[i].getResolutionY(),modesRgb[i].getFps(), modesRgb[i].getPixelFormat()); //PIXEL_FORMAT_DEPTH_1_MM = 100, PIXEL_FORMAT_DEPTH_100_UM
    }
    depth.stop();
    depth.destroy();
    rgb.stop();
    rgb.destroy();
    device.close();
    OpenNI::shutdown();	
    exit(1);
  }
	
	
  if(_depth_mode>=0){
    rc = depth.setVideoMode(device.getSensorInfo(SENSOR_DEPTH)->getSupportedVideoModes()[_depth_mode]);
    depth.setMirroringEnabled(false);
    rc = depth.start();
  }
  if(_rgb_mode>=0){
    rc = rgb.setVideoMode(device.getSensorInfo(SENSOR_COLOR)->getSupportedVideoModes()[_rgb_mode]);
    rgb.setMirroringEnabled(false);


    rgb.getCameraSettings()->setAutoExposureEnabled(true);
    rgb.getCameraSettings()->setAutoWhiteBalanceEnabled(true);
	  
    cerr << "Camera settings valid: " << rgb.getCameraSettings()->isValid() << endl;
    rc = rgb.start();
  }
  
  if(_depth_mode>=0 && _rgb_mode>=0 && _sync==1){
    rc =device.setDepthColorSyncEnabled(true);
    if (rc != STATUS_OK) {
      printf("Couldn't enable de  pth and rgb images synchronization\n%s\n",
	     OpenNI::getExtendedError());
      exit(2);
    }
  }


  if(_depth_mode>=0 && _rgb_mode>=0 && _registration==1){
    device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);
  }

  run = true;
  pthread_t runner;
  pthread_create(&runner, 0, camera_thread, 0);
  ros::spin();
  void* result;
  run =false;

  pthread_join(runner, &result);

  depth.stop();
  depth.destroy();
  rgb.stop();
  rgb.destroy();
  device.close();
  OpenNI::shutdown();	
  return 0;

}

