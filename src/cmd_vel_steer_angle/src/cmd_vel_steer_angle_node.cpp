#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/Int16.h"
#include "std_msgs/Float32.h"
#include "geometry_msgs/Twist.h"

#define RAD2DEG(x) ((x)*180.0/M_PI)
#define RPM2RPS(x) ((x)/60) 
#define RPS2RPM(x) ((x)*60) 

float robot_wheel_base = 0.73;  // 단위 m

float steering_cmd_angular;
float steering_cmd_linear;
float robot_steering_angle = 0.0;

void cmd_velCallback(const geometry_msgs::Twist& msg)
{
  float limited_angular_z = std::max(-6.5, std::min(6.16, msg.angular.z));
  
  steering_cmd_angular = limited_angular_z;
  steering_cmd_linear  = msg.linear.x;
  
  float radius = std::numeric_limits<float>::infinity();
  if (fabs(limited_angular_z) > 0.001) { // 각속도가 0이 아닌 경우에만 계산
    radius = msg.linear.x / fabs(limited_angular_z);
  }
  
  float calculated_angle = RAD2DEG(atan2(robot_wheel_base, radius));
  if (limited_angular_z < 0) {
    calculated_angle = -calculated_angle; // 각속도의 부호에 따라 조향각도 반전
  }
  robot_steering_angle = std::max(-24.0f, std::min(23.9f, calculated_angle));
}

int main(int argc, char **argv)
{
 
  ros::init(argc, argv, "cmd_vel_steer_angle");

  ros::NodeHandle n;
  /* robot param set */
  ros::param::get("~wheel_base", robot_wheel_base);
  
  /* Subscriber define */
  ros::Subscriber cmd_vel_sub = n.subscribe("/cmd_vel",100,&cmd_velCallback);
  
  /* New Publisher for cmd_vel_steer */
  ros::Publisher cmd_vel_steer_pub = n.advertise<geometry_msgs::Twist>("/cmd_vel_steer", 1);
  
  ros::Rate loop_rate(10);
  
  geometry_msgs::Twist cmd_vel_steer_msg; 
  
  while(ros::ok())
  {
    cmd_vel_steer_msg.linear.x = steering_cmd_linear; // 원래 cmd_vel로부터 받은 선속도
    cmd_vel_steer_msg.angular.z = robot_steering_angle; // 계산된 조향각도

    cmd_vel_steer_pub.publish(cmd_vel_steer_msg); // 새로운 Twist 메시지 발행
    
    ros::spinOnce();
    loop_rate.sleep(); 
  }
  
  return 0;
}  
