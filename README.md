visp_auto_tracker
=================

visp_auto_tracker wraps model-based trackers provided by ViSP visual 
servoing library into a ROS package. The tracked object should have a 
QRcode of Flash code pattern. Based on the pattern, the object is 
automaticaly detected. The detection allows then to initialise the 
model-based trackers. When lost of tracking achieves a new detection 
is performed that will be used to re-initialize the tracker.

This computer vision algorithm computes the pose (i.e. position and
orientation) of an object in an image. It is fast enough to allow
object online tracking using a camera.

This package is composed of one node called 'visp_auto_tracker'. The 
node tries first to detect the QRcode or the Flash code associated to 
the object. Once the detection is performed, the node tracks the object. 
When a lost of tracking occurs the node tries to detect once again the 
object and then restart a tracking.

The viewer comming with visp_tracker package can be used to monitor the 
tracking result.

* [Project webpage on ros.org: tutorial and API reference] [ros-homepage]
* [Project webpage: source code download, bug report] [github-homepage]


Setup
-----

This package can be compiled like any other ROS package using `rosdep`
and `rosmake`.

For more information, refer to the [ROS tutorial]
[ros-tutorial-building-pkg].


Documentation
-------------

The documentation is available on the project [ROS homepage]
[ros-homepage].


[github-homepage]: https://github.com/lagadic/visp_auto_tracker
[ros-homepage]: http://www.ros.org/wiki/visp_auto_tracker
[ros-tutorial-building-pkg]: http://www.ros.org/wiki/ROS/Tutorials/BuildingPackages "Building a ROS Package"
