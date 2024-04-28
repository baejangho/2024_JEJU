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


class PID_class:
    def __init__(self):
        # Joy_init
        rospy.init_node('control', anonymous=True)
        self.BASE_SPEED = 0
        self.steering = 0
        self.angle = 0
        self.angular_z = 0

		# Encoder and PID related initialization
        self.encoder_value = 0
        self.target_speed = 0 # 목표속도 m/s으로 설정
        self.P = 100
        self.I = 10
        self.D = 0.0
        self.last_encoder_time = rospy.get_time()
        self.last_actual_speed_m_s = 0.0
        self.error_integral = 0
        self.actual_speed_m_s = 0
        self.toggle = ''

        self.drive_pub = rospy.Publisher("cmd_vel", Twist, queue_size=1)
        rospy.Subscriber("/way_topic", Twist, self.Way_topic_callback)
        rospy.Subscriber("/parking_topic", Twist, self.parking_topic_callback)
        rospy.Subscriber("/toggle_topic", String, self.toggle_topic_callback)
        rospy.Subscriber("/cpm1", Float32, self.encoder_callback)  # m/s
        rospy.Subscriber("/lane_topic", Float32, self.lane_topic_callback) 
        rospy.Timer(rospy.Duration(0.02), self.drive_PID_control)

    def toggle_topic_callback(self, data):
        self.toggle = data.data
    
    def encoder_callback(self, data):
        # 엔코더 값은 m/s로 받아오고 있음
        self.actual_speed_m_s = data.data
        
    def Way_topic_callback(self, data):
        if self.toggle == 'way': 
            self.target_speed = data.linear.x
            self.angular_z = data.angular.z
    
    def lane_topic_callback(self, data):
        if self.toggle == 'lane':
            self.target_speed = data.linear.x
            self.angular_z = data.angular.z
    
    def parking_topic_callback(self, data):
        if self.toggle == 'parking':
            self.target_speed = data.linear.x
            self.angular_z = data.angular.z

    def drive_PID_control(self, event):

        drive = Twist()
        target = self.target_speed
    
        current_time = rospy.get_time()
        time_step = current_time - self.last_encoder_time
        self.error_integral += (target  - self.actual_speed_m_s) * time_step  # 오차 적분 업데이트
        #derivative = (self.actual_speed_m_s - self.last_actual_speed_m_s) / time_step  # 미분값 계산
        self.pid_output = self.P * (target - self.actual_speed_m_s) + self.I * self.error_integral# + self.D * derivative)
        if self.pid_output > 200:
            self.error_integral = self.error_integral - (self.pid_output - 200)
            self.pid_output = 200
        if self.pid_output < -200:
            self.error_integral = self.error_integral - (self.pid_output + 200)
            self.pid_output = -200

        print('I error:',self.error_integral)
        drive.linear.x = int(self.pid_output)  # pwm
        print('pwm:',self.pid_output)
        print('vel:',self.actual_speed_m_s)
        drive.angular.z = int(self.angular_z)
        #self.last_actual_speed_m_s = self.actual_speed_m_s  # 이전 속도 업데이트
        self.last_encoder_time = current_time  # 이전 시간 업데이트  

        self.drive_pub.publish(drive)    
            

            
if __name__ == '__main__':
    MoveCar = PID_class()
    try:
        rospy.spin()
    except KeyboardInterrupt:
        print("program down")