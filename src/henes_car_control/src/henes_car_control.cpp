#define DEBUG 0
#define DEBUG_ROS_INFO 1 

#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/Int16.h"
#include "std_msgs/Int32.h"
#include "std_msgs/Float32.h"
#include "geometry_msgs/Twist.h"
#include "sensor_msgs/Range.h"
#include "sensor_msgs/Imu.h"
#include "tf/transform_broadcaster.h"
#include "nav_msgs/Odometry.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include "geometry_msgs/Vector3.h"
#include <sstream>

// 라디안-각도 변환하는거
#define RAD2DEG(x) ((x)*180./M_PI)
#define DEG2RAD(x) ((x)/180.*M_PI)

// Steering Control
#define Neutral_Angle_Offset  3
#define Right_MAX -40
#define Left_MAX  40

int steering_angle = 0;
float motor_speed = 0;
float motor_speed_cmd = 0;
int Base_Speed = 60;
int steering_angle_old = 0;
float motor_speed_old = 0;
long encoder1data = 0;
long encoder2data = 0;

double roll,pitch,yaw,yaw_old;
double roll_d,pitch_d,yaw_d,yaw_d_old;
double delta_yaw_d, delta_yaw;
int  imu_read_flag_start = 0;
bool imu_init_angle_flag = 0;
double init_yaw_angle = 0.0;
double init_yaw_angle_d = 0.0;
double imu_offset_angle = 0;  // degree

// 엔코더 센서 데이터 구조체 형성
struct BaseSensorData
{
    int encoder = 0;   
    int encoder_old = 0; 
}myBaseSensorData;

// 오돔 계산 결과 데이처 구조체 형성
struct OdomCaculateData
{
    //motor params
    float distance_ratio=345;  //unit: m/encode  338.0
    float encode_sampling_time=0.05; //unit: s-> 20hz
    float cmd_vel_linear_max= 1.2; //unit: m/s
    float cmd_vel_angular_max=0.8; //unit: rad/s
    //odom result
    float position_x=0.0; //unit: m
    float position_y=0.0; //unit: m
    float orientaion=0.0; //unit: rad
    float velocity_linear=0.0; //unit: m/s
    float velocity_angular=0.0; //unit: rad/s
    
}myOdomCaculateData;

void calculateDeltaYaw(double& yaw, double& yaw_old, double& delta_yaw) {
    // Yaw 값을 -π ~ π 범위로 정규화
    double new_yaw = fmod(yaw + M_PI, 2.0 * M_PI);
    if (new_yaw < 0) new_yaw += 2.0 * M_PI;
    new_yaw -= M_PI;

    // Yaw_old 값을 -π ~ π 범위로 정규화
    double new_yaw_old = fmod(yaw_old + M_PI, 2.0 * M_PI);
    if (new_yaw_old < 0) new_yaw_old += 2.0 * M_PI;
    new_yaw_old -= M_PI;

    // Delta_yaw 계산
    delta_yaw = new_yaw - new_yaw_old;

    // 180도를 넘어서는 경우를 처리
    if (delta_yaw > M_PI) {
        delta_yaw -= 2.0 * M_PI;
    } else if (delta_yaw < -M_PI) {
        delta_yaw += 2.0 * M_PI;
    }

    // Yaw_old 업데이트
    yaw_old = yaw;
}

void imuCallback(const sensor_msgs::Imu::ConstPtr& msg) 
{
    static ros::Time last_time = ros::Time::now();
    ros::Time current_time = ros::Time::now();

    // 데이터 처리 간격을 0.0108초로 제한 (약 92Hz)
    if ((current_time - last_time).toSec() < 0.0108) {
        return;
    }
    last_time = current_time;
	
	 char buf[8];    
      tf2::Quaternion q(
        msg->orientation.x,
        msg->orientation.y,
        msg->orientation.z,
        msg->orientation.w);
      tf2::Matrix3x3 m(q);     
            
      m.getRPY(roll, pitch, yaw);
      yaw = yaw - DEG2RAD(imu_offset_angle);
      roll_d  = RAD2DEG(roll); 
      pitch_d = RAD2DEG(pitch);
      yaw_d   = RAD2DEG(yaw);
      calculateDeltaYaw(yaw, yaw_old, delta_yaw); // Call the calculateDeltaYaw function

      //ROS_INFO("yaw_d : %10f", yaw_d );
      //imu_read_flag_start ++;
      
}

void encoder1Callback(const std_msgs::Int32& encoder_data1)
{
	encoder1data = encoder_data1.data; // team1 car
  //ROS_INFO("encoder1 : %10f", encoder1data );
}
void encoder2Callback(const std_msgs::Int32& encoder_data2)
{
	//encoder2data = encoder_data2.data; 
  //ROS_INFO("encoder2 : %10f", encoder2data );
}


void CarControlCallback(const geometry_msgs::Twist& msg)
{
  
   steering_angle = (int)(msg.angular.z) ; //0410 
   
   if(steering_angle >= Left_MAX )   steering_angle = Left_MAX ;
   if(steering_angle <= Right_MAX )  steering_angle = Right_MAX ;
   
   motor_speed = msg.linear.x;
   if(motor_speed>=255)   motor_speed = 250;
   if(motor_speed<=-255)  motor_speed = -250;   
}


void CarSteerControlCallback(const std_msgs::Int16& angle)
{
  //steering_angle = (int)(angle.data) ;
  
  //if(steering_angle >= Left_MAX )   steering_angle = Left_MAX ;
  //if(steering_angle <= Right_MAX )  steering_angle = Right_MAX ;   
  
}



void odometry_cal(void)
{		
  int delta_encoder = myBaseSensorData.encoder - myBaseSensorData.encoder_old;
  float base_link_delta_x;   float base_link_delta_y;  float base_link_delta_l;
  float odom_delta_x;        float odom_delta_y;  
  float radius;
  float temp = yaw;
  float temp_d = yaw_d;
  //ROS_INFO("myBaseSensorData.encoder : %10d", myBaseSensorData.encoder );


  delta_yaw_d = yaw_d - yaw_d_old;
  //ROS_INFO("yaw_d : %10f", yaw );
  //delta_yaw = yaw - yaw_old;
  myOdomCaculateData.orientaion += delta_yaw;
  
  
  base_link_delta_l =  (float)delta_encoder ;  /// myOdomCaculateData.distance_
  //ROS_INFO("base_link_delta_l : %10f", base_link_delta_l );
  //ROS_INFO("delta_encoder : %10d", delta_encoder );
  //ROS_INFO("(float)delta_encoder : %10f", (float)delta_encoder );
  
  
  if(fabs(delta_yaw)>1.0e-7) 
  {
  //ROS_INFO("delta_yaw : %10f", delta_yaw );
  
	 radius = base_link_delta_l / delta_yaw;
  //ROS_INFO("radius : %10f", radius ); 
	 if(delta_yaw < 0)  base_link_delta_y = -radius*(1 - cos(delta_yaw));
	 else               base_link_delta_y =  radius*(1 - cos(delta_yaw));
	 
	 base_link_delta_x = radius * sin(delta_yaw);	
	 //ROS_INFO("base_link_delta_x : %s", "turn!" );   
  }
  else  
  {
  //ROS_INFO("base_link_delta_x : %s", "else!");
	base_link_delta_x = base_link_delta_l;
	base_link_delta_y = 0.0;
  }  
    base_link_delta_x /= myOdomCaculateData.distance_ratio;
    base_link_delta_y /= myOdomCaculateData.distance_ratio;
    ROS_INFO("delta_yaw : %10f", delta_yaw );
    ROS_INFO("yaw : %10f", yaw );
    ROS_INFO("yaw_old : %10f", yaw_old );
    //ROS_INFO("base_link_delta_x : %10f", base_link_delta_x); ///// 0410
    //ROS_INFO("base_link_delta_y : %10f", base_link_delta_y); ///// 0410
    
  odom_delta_x =  base_link_delta_x * cos(yaw_old)  - base_link_delta_y * sin(yaw_old);   // rotation_matrix
	odom_delta_y =  base_link_delta_x * sin(yaw_old)  + base_link_delta_y * cos(yaw_old);
  myOdomCaculateData.position_x += odom_delta_x;
  myOdomCaculateData.position_y += odom_delta_y;

    // 방향 업데이트
  //myOdomCaculateData.orientation = yaw;
  //myOdomCaculateData.orientation += delta_yaw;
}


int main(int argc, char **argv)
{
  char buf[2]; 
  
  ros::init(argc, argv, "Henes_Car_Control");

  ros::NodeHandle n;
  ros::NodeHandle priv_nh("~");
  
  
  std::string cmd_vel_topic = "cmd_vel";
  std::string odom_pub_topic = "odom";
  std::string imu_topic = "handsfree/imu";
  
  /*other*/
  ros::param::get("/imu_offest_angle",  imu_offset_angle);  
  
  ros::param::get("~cmd_vel_topic", cmd_vel_topic);
  ros::param::get("~odom_pub_topic", odom_pub_topic);
  ros::param::get("~imu_topic", imu_topic); 
 
  std_msgs::String msg;
  std_msgs::Int16 steerangle;
  std_msgs::Int16 carspeed;
    
  geometry_msgs::Twist teleop_cmd_vel_data;
  
  ros::Subscriber sub1 = n.subscribe("/cmd_vel", 10, &CarControlCallback);
  ros::Subscriber sub2 = n.subscribe("/Car_Control_cmd/Steering_angle_Int16",10, &CarSteerControlCallback);
  ros::Subscriber subEncoder1 = n.subscribe("/encoder1",10,&encoder1Callback); // front  encoder data susscribe
  ros::Subscriber subEncoder2 = n.subscribe("/encoder2",10,&encoder2Callback); // rear  encoder data susscribe
  ros::Subscriber subIMU = n.subscribe("handsfree/imu", 20, &imuCallback);  // imu data susscribe
 
  ros::Publisher teleop_cmd_vel_pub = n.advertise<geometry_msgs::Twist>("teleop_cmd_vel", 10);
  
  ros::Publisher car_control_pub1 = n.advertise<std_msgs::Int16>("Car_Control/SteerAngle_Int16", 10);
  ros::Publisher car_control_pub2 = n.advertise<std_msgs::Int16>("Car_Control/Speed_Int16", 10);

  ros::Publisher odom_pub = n.advertise<nav_msgs::Odometry>(odom_pub_topic, 20);
 
  ros::Rate loop_rate(10);  // 10
  
  int count = 0;
  
  //////////////////  odometry  //////////////////////
  std::string odom_frame_id = "odom";
  std::string odom_child_frame_id = "base_footprint";
  
  ////////////////  TF odometry  //////////////////////
  static tf::TransformBroadcaster odom_broadcaster;
  geometry_msgs::TransformStamped odom_trans;
  nav_msgs::Odometry odom;
  geometry_msgs::Quaternion odom_quat;
  
  //covariance matrix
  float covariance[36] = {0.01,   0,    0,     0,     0,     0,  // covariance on gps_x
                            0,  0.01, 0,     0,     0,     0,  // covariance on gps_y
                            0,  0,    99999, 0,     0,     0,  // covariance on gps_z
                            0,  0,    0,     99999, 0,     0,  // large covariance on rot x
                            0,  0,    0,     0,     99999, 0,  // large covariance on rot y
                            0,  0,    0,     0,     0,     0.01};  // large covariance on rot z 
  //load covariance matrix
  for(int i = 0; i < 36; i++)
  {
      odom.pose.covariance[i] = covariance[i];;
  }     
  
   ros::Duration(1).sleep();   // 1 sec stop for safety
  
  while (ros::ok())
  {
    //ROS_INFO("Speed : %3d | Steering : %2d", motor_speed_cmd, steering_angle );    
    //printf("imu offset %3.1lf \n",imu_offset_angle);
    if(steering_angle != steering_angle_old) 
    {
       teleop_cmd_vel_data.angular.z = steering_angle;
	     teleop_cmd_vel_data.linear.x  = motor_speed_cmd;
       teleop_cmd_vel_pub.publish(teleop_cmd_vel_data) ;
    }
    
    if(motor_speed != motor_speed_old)
    {    
       teleop_cmd_vel_data.angular.z = steering_angle;
	     teleop_cmd_vel_data.linear.x  = motor_speed_cmd;
       teleop_cmd_vel_pub.publish(teleop_cmd_vel_data) ;
    } 
   
    steering_angle_old = steering_angle;
    motor_speed_old = motor_speed_cmd ; 
    
    
   /* //////////////////// sonar obstacle detectioin //////////////////
    if( ( (sonar[0]>0) && ( sonar[0] <= Sonar_Obstacle_Range ) ) || ( (sonar[1]>0) && ( sonar[1] <= Sonar_Obstacle_Range) ) ||  ( (sonar[2]>0) && ( sonar[2] <= Sonar_Obstacle_Range) ) ) 
    {
       motor_speed_cmd  = 0;
       ROS_INFO("Sonar Obstacle detection : %3.2lf %3.2lf %3.2lf", sonar[0], sonar[1], sonar[2]);       
    }    
    else
    {
		 motor_speed_cmd = motor_speed;
	} 
	*/
	   
    steerangle.data = steering_angle;
    carspeed.data = motor_speed;

    car_control_pub1.publish(steerangle);
    car_control_pub2.publish(carspeed);
    
    // ROS_INFO("Sonar : %5.1lf %5.1lf %5.1lf", sonar[0], sonar[1], sonar[2]);
    
    myBaseSensorData.encoder = encoder1data;   //depend on encoder direction
	  odometry_cal();
	  myBaseSensorData.encoder_old =  myBaseSensorData.encoder;
	
    //odom_orientaion trans to odom_quat
    odom_quat = tf::createQuaternionMsgFromYaw(myOdomCaculateData.orientaion);//yaw trans quat
 
    //pub tf(odom->base_footprint)
    odom_trans.header.stamp = ros::Time::now();
    odom_trans.header.frame_id = odom_frame_id;     
    odom_trans.child_frame_id = odom_child_frame_id;       
    odom_trans.transform.translation.x = myOdomCaculateData.position_x;
    odom_trans.transform.translation.y = myOdomCaculateData.position_y;
    odom_trans.transform.translation.z = 0.0;
    odom_trans.transform.rotation = odom_quat;
    
    //pub odom
    odom.header.stamp = ros::Time::now(); 
    odom.header.frame_id = odom_frame_id;
    odom.child_frame_id = odom_child_frame_id;       
    odom.pose.pose.position.x = myOdomCaculateData.position_x;     
    odom.pose.pose.position.y = myOdomCaculateData.position_y;
    odom.pose.pose.position.z = 0.0;
    odom.pose.pose.orientation = odom_quat;
    odom_broadcaster.sendTransform(odom_trans);
    odom_pub.publish(odom);
    
    loop_rate.sleep();
    ros::spinOnce();
    ++count;
  }

  motor_speed_cmd = 0;
  return 0;
}




