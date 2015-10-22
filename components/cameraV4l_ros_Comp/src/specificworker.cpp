/*
 *    Copyright (C) 2015 by YOUR NAME HERE
 *
 *    This file is part of RoboComp
 *
 *    RoboComp is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    RoboComp is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RoboComp.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "specificworker.h"

/**
* \brief Default constructor
*/
SpecificWorker::SpecificWorker(MapPrx& mprx) : GenericWorker(mprx)
,str(new std_msgs::String)
{
	namedWindow("img",1);
	
	//initializing ros node
	
	test_pub = nh.advertise<std_msgs::String>(nh.resolveName("testpublication"),1);
	sub = nh.subscribe("chatter", 1000, &SpecificWorker::chatterCallback, this);
}

/**
* \brief Default destructor
*/
SpecificWorker::~SpecificWorker()
{
	qDebug() << "CameraV4L exiting...";
}


void SpecificWorker::chatterCallback(const std_msgs::String::ConstPtr& msg)
{
  ROS_INFO("I heard: [%s]", msg->data.c_str());
}

bool SpecificWorker::setParams(RoboCompCommonBehavior::ParameterList params)
{
	RoboCompRGBDBus::CameraParams camParams;
	try
	{
		RoboCompCommonBehavior::Parameter par = params.at("CameraV4L.Device0.Name");
		camParams.name = par.value;
		par = params.at("CameraV4L.Device0.FPS");
		camParams.colorFPS = atoi(par.value.c_str());
		par = params.at("CameraV4L.Device0.Width");
		camParams.colorWidth = atoi(par.value.c_str());
		par = params.at("CameraV4L.Device0.Height");
		camParams.colorHeight = atoi(par.value.c_str());
	}
	catch(std::exception e) 
	{ qFatal("\nAborting. Error reading config params"); }

	std::array<string, 6> list = { "0", "1", "2", "3", "4", "5" };
	qDebug() << __FUNCTION__ << "Opening device:" << camParams.name.c_str();
	if( camParams.name == "default")
		grabber.open(0);
	else if	( find( begin(list), end(list), camParams.name) != end(list)) 
		grabber.open(atoi(camParams.name.c_str()));
	else
		grabber.open(camParams.name);
	
	if(grabber.isOpened() == false)  // check if we succeeded
		qFatal("Aborting. Could not open default camera %s", camParams.name.c_str());
	else
		qDebug() << __FUNCTION__ << "Camera " << QString::fromStdString(camParams.name) << " opened!";
	
	//Setting grabber
	
	camParams.colorFPS = 20;
	grabber.set(CV_CAP_PROP_FPS, camParams.colorFPS);
	camParams.colorFocal = 400;
	grabber.set(CV_CAP_PROP_FRAME_HEIGHT, 480);  //Get from PARAMS
	grabber.set(CV_CAP_PROP_FRAME_WIDTH, 640);

	//One frame to get real sizes
	Mat frame;
	grabber >> frame; 		// get a new frame from camera
	Size s = frame.size();
	double rate = grabber.get(CV_CAP_PROP_FPS);
	qDebug() << __FUNCTION__ << "Current frame size:" << s.width << "x" << s.height << ". RGB 8 bits format at" << rate << "fps";
	camParams.colorWidth = s.width;
	camParams.colorHeight = s.height;
	writeBuffer.resize( camParams.colorWidth * camParams.colorHeight * 3);
	readBuffer.resize( camParams.colorWidth * camParams.colorHeight * 3);
	cameraParamsMap[camParams.name] = camParams;

	timer.start(30);

	return true;
}

void SpecificWorker::compute()
{
	static QTime reloj = QTime::currentTime();
	static int fps=0;
	
	Mat frame, frameRGB;
	grabber.read(frame); 	
	cvtColor( frame, frameRGB, CV_BGR2RGB );
	memcpy( &writeBuffer[0], frameRGB.data, frameRGB.size().area() * 3);
	//qDebug() << "Reading..."; // at" << grabber.get(CV_CAP_PROP_FPS) << "fps";
	//imshow("img", frame);
	QMutexLocker ml(mutex);
	readBuffer.swap( writeBuffer);
	
	if( reloj.elapsed() > 1000)
	{
		qDebug() << "Grabbing at"<< fps << "FPS ";
		fps=0;
		reloj.restart();
	}
	fps++;
	
	str->data = "If you are seeing this, it's mainly coz I'm awesome";
	test_pub.publish(str);
// 	ROS_INFO_STREAM("they see me rollin!");
	ros::spinOnce();
	//if(waitKey(30) >= 0) exit(-1);
	
}

////////////////////////////////////////////////////////////
///// SERVANTS
///////////////////////////////////////////////////////////


CameraParamsMap SpecificWorker::getAllCameraParams()
{
	return cameraParamsMap;
}

void SpecificWorker::getPointClouds(const CameraList &cameras, PointCloudMap &clouds)
{
	//Not implemented
}

void SpecificWorker::getImages(const CameraList &cameras, ImageMap &images)
{
	if( cameras.size() <= MAX_CAMERAS and ( cameraParamsMap.find(cameras.front()) != cameraParamsMap.end()))
	{
		//uint size = cameraParamsMap[cameras.front()].colorHeight * cameraParamsMap[cameras.front()].colorWidth * 3;
		RoboCompRGBDBus::Image img;
		img.camera = cameraParamsMap[cameras.front()];
		QMutexLocker ml(mutex);
		img.colorImage.resize( readBuffer.size());
		img.colorImage.swap( readBuffer );
		images.insert( std::pair<std::string, RoboCompRGBDBus::Image>("default", img));
		std::cout << "camera name " <<cameras.front() << " " << img.colorImage.size() << " " << images.empty() << std::endl;
	}
}

void SpecificWorker::getProtoClouds(const CameraList &cameras, PointCloudMap &protoClouds)
{
		//Not implemented
}

void SpecificWorker::getDecimatedImages(const CameraList &cameras, const int decimation, ImageMap &images)
{
}




