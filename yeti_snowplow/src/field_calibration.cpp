/* Yeti Field Calibration
    Stores seen landmarks into configuration file
    Progress: Incomplete
*/
#include <ros/ros.h>
#include <vector>
#include <math.h>
#include <sensor_msgs/LaserScan.h>
#include <sensor_msgs/PointCloud.h>
#include <laser_geometry/laser_geometry.h>
#include <yeti_snowplow/robot_position.h>
#include <yeti_snowplow/obstacle.h>
#include <yeti_snowplow/obstacles.h>

using namespace std;

//Global Variables












int main(int argc, char **argv)
{
    ros::init(argc, argv, "field_calibration");
    ros::NodeHandle nh;
    ros::Subscriber scanSub = n.subscribe("scan", 1, scanCallback);
    ros::Publisher calibratePub;//Publishes status of calibration
    //calibratePub = nh.advertise...
    while(ros::ok())
    {
        //Publish status
    }
    ros::spin();
    return 0;
}