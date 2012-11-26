#include "node.h"
#include "names.h"

//command line parameters
#include "cmd_line/cmd_line.h"

//detectors
#include "detectors/datamatrix/detector.h"
#include "detectors/qrcode/detector.h"

//tracking
#include "libauto_tracker/tracking.h"
#include "libauto_tracker/threading.h"
#include "libauto_tracker/events.h"

//visp includes
#include <visp/vpDisplayX.h>
#include <visp/vpMbEdgeKltTracker.h>
#include <visp/vpMbKltTracker.h>
#include <visp/vpMbEdgeTracker.h>

#include "conversions/camera.h"
#include "conversions/image.h"
#include "conversions/3dpose.h"

#include "libauto_tracker/tracking.h"

namespace visp_auto_tracker{
	Node::Node() :
			n_("~"),
			queue_size_(1),
			got_image_(false){
		//get the tracker configuration file
		//this file contains all of the tracker's parameters, they are not passed to ros directly.
		n_.param<std::string>("tracker_config_path", tracker_config_path_, "");
		if(tracker_config_path_ == ""){
			ROS_ERROR("cannot find config file!");
			ros::shutdown();
		}else
			ROS_INFO("tracker config path: %s",tracker_config_path_.c_str());
	}

	void Node::waitForImage(){
	    while ( ros::ok ()){
	    	if(got_image_) return;
	        ros::spinOnce();
	      }
	}

	//records last recieved image
	void Node::frameCallback(const sensor_msgs::ImageConstPtr& image, const sensor_msgs::CameraInfoConstPtr& cam_info){
		boost::mutex::scoped_lock(lock_);
		I_ = visp_bridge::toVispImageRGBa(*image); //make sure the image isn't worked on by locking a mutex
		got_image_ = true;
	}

	void Node::spin(){
		//Parse command line arguments from config file (as ros param)
		CmdLine cmd(tracker_config_path_);

		if(cmd.should_exit()) return; //exit if needed

		vpMbTracker* tracker; //mb-tracker will be chosen according to config

		vpCameraParameters cam = cmd.get_cam_calib_params(); //retrieve camera parameters
		if(cmd.get_verbose())
			std::cout << "loaded camera parameters:" << cam << std::endl;


		//create display
		vpDisplayX* d = new vpDisplayX();

		//init detector based on user preference
		detectors::DetectorBase* detector = NULL;
		if (cmd.get_detector_type() == CmdLine::ZBAR)
			detector = new detectors::qrcode::Detector;
		else if(cmd.get_detector_type() == CmdLine::DTMX)
			detector = new detectors::datamatrix::Detector;

		//init tracker based on user preference
		if(cmd.get_tracker_type() == CmdLine::KLT)
			tracker = new vpMbKltTracker();
		else if(cmd.get_tracker_type() == CmdLine::KLT_MBT)
			tracker = new vpMbEdgeKltTracker();
		else if(cmd.get_tracker_type() == CmdLine::MBT)
			tracker = new vpMbEdgeTracker();

		//compile detectors and paramters into the automatic tracker.
		t_ = new tracking::Tracker(cmd,detector,tracker);
		t_->start(); //start the state machine

		//subscribe to ros topics and prepare a publisher that will publish the pose
		message_filters::Subscriber<sensor_msgs::Image> raw_image_subscriber(n_, image_topic, queue_size_);
		message_filters::Subscriber<sensor_msgs::CameraInfo> camera_info_subscriber(n_, camera_info_topic, queue_size_);
		message_filters::TimeSynchronizer<sensor_msgs::Image, sensor_msgs::CameraInfo> image_info_sync(raw_image_subscriber, camera_info_subscriber, queue_size_);
		image_info_sync.registerCallback(boost::bind(&Node::frameCallback,this, _1, _2));
		ros::Publisher object_pose_publisher = n_.advertise<geometry_msgs::PoseStamped>(object_position_topic, queue_size_);

		//wait for an image to be ready
		waitForImage();
		{
			//when an image is ready tell the tracker to start searching for patterns
			boost::mutex::scoped_lock(lock_);
			d->init(I_); //also init display
			t_->process_event(tracking::select_input(I_));
		}

		unsigned int iter=0;
		geometry_msgs::PoseStamped ps;
		ps.header.frame_id = tracker_ref_frame;
		ros::Rate rate(25); //init 25fps publishing frequency
		while(ros::ok()){
			boost::mutex::scoped_lock(lock_);
			//process the new frame with the tracker
			t_->process_event(tracking::input_ready(I_,cam,iter));
			//When the tracker is tracking, it's in the tracking::TrackModel state
			//Access this state and broadcast the pose
			tracking::TrackModel& track_model = t_->get_state<tracking::TrackModel&>();
			ps.header.stamp = ros::Time::now(); //sign the pose with time
			ps.header.seq = iter++; //...and sequence
			ps.pose = visp_bridge::toGeometryMsgsPose(track_model.cMo); //convert
			object_pose_publisher.publish(ps); //publish
			ros::spinOnce();
			rate.sleep();
		}
		t_->process_event(tracking::finished());

	}
}