#!/usr/bin/env python3
# -*- coding:utf-8 -*-
## lane_detection/scripts/control_wecar.py
import rospy
import numpy as np
import math
from std_msgs.msg import Int32
from std_msgs.msg import String
from std_msgs.msg import Float32
from geometry_msgs.msg import Twist
from sensor_msgs.msg import Joy
from sensor_msgs.msg import LaserScan


class move_limo:
    def __init__(self):
        # Joy_init
        rospy.init_node('control', anonymous=True)
        self.ref_x = 180
        self.ref_dist_to_left = 140
        self.BASE_SPEED = 0
        self.LATERAL_GAIN = -0.8
        self.manaul = 0
        self.auto = 0
        self.speed = 0
        self.steering = 0
        self.angle = 0
        self.P_gain = 1.7
        self.flag = 0
        self.dist_obstacle = 0

        # Obstacle_init
        self.x_list = []
        self.y_list = []
        self.MAX_ANGLE_DEG = 90
        self.MIN_ANGLE_DEG = -90
        self.MAX_OBST_ANGLE_DEG = 10
        self.MIN_OBST_ANGLE_DEG = -10

        # Topic_Pub_lidar & cmd_vel
        self.pub_obstacle = rospy.Publisher("/lidar_obstacle",String,queue_size=1)
        self.pub_center_n = rospy.Publisher("/center_n",Float32,queue_size=1)
        self.pub_right_n = rospy.Publisher("/right_n",Float32,queue_size=1)
        self.pub_left_n = rospy.Publisher("/left_n",Float32,queue_size=1)
        self.drive_pub = rospy.Publisher("cmd_vel", Twist, queue_size=1)

        rospy.Subscriber("scan",LaserScan,self.scan_callback)
        rospy.Subscriber("/joy", Joy, self.send_joy)
        rospy.Subscriber("/lane/lane_left", Int32, self.lane_L)
        rospy.Subscriber("/lane/lane_right", Int32, self.lane_R)
        rospy.Subscriber("/lane/lane_center", Int32, self.lane_C)

        rospy.Timer(rospy.Duration(0.03), self.drive_control)
    
    def send_joy(self, data):
        self.joy_btn = data.buttons
        self.joy_axes = data.axes
        #print(self.joy_btn)
        #print(self.joy_axes)
        drive = Twist()


        self.auto = self.joy_btn[4]
        self.manaul = self.joy_btn[5]

    def scan_callback(self, data):
        self.x_list = []
        self.y_list = []
        for i,n in enumerate(data.ranges): # i와 n에 차례대로 인덱스와 값을 저장 예를 들어 i = 0, n = 첫번째 각에서 측정 거리 , i = 1, n = 두번째에서 거리
            angle = data.angle_min + data.angle_increment * i 
            angle_deg = angle * 180 / math.pi
            x = -n * math.cos(angle) # 레이저의 각도와 레이저가 측정한 길이를 곱해 x,y좌표로 바꾼다
            y = n * math.sin(angle)
            if y > -0.20 and y < 0.20 and x > 0: # 정면 -20cm ~ 20cm 사이의 값들을 다 리스트로 저장한다
                self.x_list.append(x)
                self.y_list.append(y)
        
        #print(data.angle_increment*180 / math.pi*760)
        # print(len(data.ranges)) # 760개
        # print(data.angle_min) # -3.141592...
        # print(data.angle_increment) # 0.008278241381049156
        # print(data.angle_max) # 3.141592... 
        # 즉 정리하면-> 라이다의 라디안 범위 -pi ~ pi => *180 / pi 하면 -180~180도의 값이 들어옴
        # 3.141592... / 0.008278241.. = 380 이니 총 760개의 리스트가 들어오는 것이 맞음
        # 이를 각도로 치환해서 더해주지 말고 분해능 값에 따라 나뉘어져 있는 요소만큼 더해주는 식으로 코드를 짠것

        if len(self.x_list) == 0: 
            self.dist_obstacle = 3 # 초기 장애물 최소 거리를 그냥 3으로 잡아놓은듯
        else:
            self.dist_obstacle = min(self.x_list) # 정면 y범위에서 제일 가까운 x값 검출
        return self.dist_obstacle
    

    def drive_control(self, event):
        drive = Twist()
        if self.manaul == 1:
            self.speed = self.joy_axes[4]
            #print(self.speed)

            self.steering = self.joy_axes[0]
            drive.linear.x = int(self.speed * 100)
            drive.angular.z = int(self.steering * 40)
        

        elif self.auto == 1:
            if self.dist_obstacle < 1: # 50 cm 이하면 정지
                drive.linear.x = 0
                drive.angular.z = 0
            else:
                self.angle = self.P_gain*(343 - self.center_x)  # 아니면 정상 주행
                self.speed = 50
                drive.linear.x = self.speed 
                drive.angular.z = self.angle

        else:
            drive.linear.x = 0
            drive.angular.z = 0

        self.drive_pub.publish(drive)    
            
    def lane_C(self, data):
        self.center_x = data.data
    
    def lane_R(self, data):
        if data.data == 1:
            self.right_x = 0.0
        else:
            self.right_x = data.data 

    def lane_L(self, data):
        if data.data == -1:
            self.left_x = 0.0
        else:
            self.left_x = data.data
            
if __name__ == '__main__':
    MoveCar = move_limo()
    try:
        rospy.spin()
    except KeyboardInterrupt:
        print("program down")
