#include <ros/ros.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/Pose2D.h>
#include <sensor_msgs/PointCloud.h>
#include <yeti_snowplow/obstacles.h>
#include <yeti_snowplow/obstacle.h>
#include <yeti_snowplow/location_point.h>
#include <yeti_snowplow/lidar_point.h>
#include <yeti_snowplow/turn.h>
#include <yeti_snowplow/target.h>
#include <sensor_msgs/LaserScan.h>
#include <sensor_msgs/PointCloud.h>
#include <laser_geometry/laser_geometry.h>
#include <yeti_snowplow/waypoint.h>
#include "buffer.h"
#include <math.h>
#include <vector>
#include <algorithm>

class ObstacleReaction
{
public:
	// void obstaclePositionsCallback(const yeti_snowplow::obstacles::ConstPtr& groupOfObstacles)
	// {
	// 	obstacles = groupOfObstacles->obstacles;
	// }
	ObstacleReaction()
	{
		wayPointID = 0;

		//turnPub = n.advertise<yeti_snowplow::turn>("obstacle_reaction_turn", 1000);
		//obstaclePositionsSub = n.subscribe("obstacles", 1000, obstaclePositionsCallback);
		robotPositionSub = n.subscribe("robot_position", 1000, &ObstacleReaction::robotPositionCallback, this);
		waypointClient = n.serviceClient<yeti_snowplow::waypoint>("waypoint");
		scanSub = n.subscribe("obstacles", 1000, &ObstacleReaction::scanCallback,this);
		velocityPub  = n.advertise<geometry_msgs::Twist>("obstacle_reactance/velocity", 1000);
		
		n.param("maximum_navigation_speed", maxSpeed, 0.7);
		n.param("obstacle_turn_boost", turnBoost, -1.2);
		n.param("obstacle_reaction_reverse_speed", reverseSpeed, -0.5);

		
	}
	void robotPositionCallback(const geometry_msgs::Pose2D::ConstPtr& robotPosition)
	{
		robotLocation.x = robotPosition->x;
		robotLocation.y = robotPosition->y;
		robotLocation.theta = robotPosition->theta;
	}

	void getNextWaypoint()
	{
		yeti_snowplow::waypoint wayPointInfo;
		wayPointInfo.request.ID = wayPointID; 
		ROS_INFO("Received request: %i", wayPointInfo.request.ID);

		if(waypointClient.call(wayPointInfo))
		{
			ROS_INFO("Calling waypoint service");
			dir = wayPointInfo.response.waypoint.dir;
			navSpeed = wayPointInfo.response.waypoint.speed;
			nextWayPoint = wayPointInfo.response.waypoint.location;
		}
		else{
			ROS_INFO("Failed to call service");
		}

		wayPointID++;
	}

	void scanCallback(const sensor_msgs::LaserScan::ConstPtr& scannedData)
	{
		/* This fires every time a new obstacle scan is published */
		//location contains an array of points, which contains an x and a y relative to the robot
	
		//Projecting LaserScanData to PointCloudData
		laser_geometry::LaserProjection projector_;
		sensor_msgs::PointCloud cloudData;
		projector_.projectLaser(*scannedData, cloudData);
		lidarData = cloudData.points;
	}

	void obstacleReactance()
	{
		Buffer buffer;
		geometry_msgs::Twist msg;
		yeti_snowplow::turn turnMsg;
		getNextWaypoint();
		buffer.combinedUpdatePoints(lidarData);
		//Creates List of LiDAR points which have a positive Y value, and are within the Buffer distance threshold
		
		//look at angle needed to go to target waypoint, if there is an obstacle in the way, then find what turn angle is needed to avoid it to the right. 
		double rightAngle = buffer.combinedRightWheelScan(nextWayPoint);
		//look at angle needed to go to target waypoint, if there is an obstacle in the way, then find what turn angle is needed to avoid it to the left. 
		double leftAngle = buffer.combinedLeftWheelScan(nextWayPoint);

		
		for(yeti_snowplow::obstacle object : obstacles)
		{
			if(object.isAMovingObstacle)
			{
				movingObstacleDetected = true;
				break;
			}
			else
			movingObstacleDetected = false;
		}

		if(movingObstacleDetected)
		{
			speed = 0;
			turn = 0;
			leftSpeed = 0;
			rightSpeed = 0;
		}
		else if(rightAngle == 0 && leftAngle == 0)
		{
			//turn unchanged
			speed = 1 / (1 + 1 * abs(turn)) * (double)dir;
            speed = (double)turn * min(abs(speed), 1.0);
            leftSpeed = (float)((speed + turnBoost * turn) * maxSpeed * navSpeed);//controlvarspeed is read in from text file, and limits speed by a percentage
            rightSpeed = (float)((speed - turnBoost * turn) * maxSpeed * navSpeed);
		}
		else if (rightAngle == buffer.DOOM && leftAngle == buffer.DOOM )//There is no way to avoid anything to the left or the right, so back up.
        {
            leftSpeed = reverseSpeed * (float)maxSpeed * (float) .25;
            rightSpeed = reverseSpeed * (float)maxSpeed * (float) .25;
            ROS_INFO("I reached DOOM!");
        }
		else
		{
			if(abs(rightAngle - turn) <= abs(leftAngle - turn))
            {
                //move right of obstacle
                turn = rightAngle;
            }
            else if (abs(rightAngle - turn) > abs(leftAngle - turn))
            {
                //move Left of obstacle
                turn = leftAngle;
            }
            //speed slower as turn steeper 
            speed = 1 / (1 + 1 * abs(turn)) * (double)dir;
            speed = (double)dir * min(abs(speed), 1.0);
            leftSpeed = (float)((speed + turnBoost * .5 *  turn) * maxSpeed  * navSpeed);
            rightSpeed = (float)((speed - turnBoost * .5* turn) * maxSpeed *   navSpeed);
		}
		//Needs review
		msg.linear.x = speed;
		msg.angular.z = turn;
		velocityPub.publish(msg);
		// turnMsg.angle = turn;
		// turnMsg.stop = movingObstacleDetected;
		// turnPub.publish(turnMsg);
	}
private:
//ROS Variables
	ros::NodeHandle n;
	ros::Publisher turnPub;
	//ros::Subscriber // ;
	ros::Subscriber robotPositionSub;
	ros::ServiceClient waypointClient;
	ros::Subscriber scanSub;
	ros::Publisher velocityPub;
	vector<yeti_snowplow::obstacle> obstacles;
	vector<yeti_snowplow::obstacle> obstaclesInFront;
	vector<geometry_msgs::Point32> lidarData;
	geometry_msgs::Pose2D robotLocation;
	double rightAngle; 
	double leftAngle;
	double leftSpeed;
	double rightSpeed;
	yeti_snowplow::location_point nextWayPoint;
	double navSpeed;
	double speed;
	double turn;
	double turnBoost;
	double maxSpeed;
	double reverseSpeed;
	bool movingObstacleDetected;
	int dir;
	int wayPointID;
};

int main(int argc, char **argv){
	ros::init(argc, argv, "obstacle_reaction");

	ObstacleReaction obstacleReaction;
	
	while(ros::ok())
	{
		ros::spin();
		obstacleReaction.obstacleReactance();
	}
	
	
	return 0;
}
