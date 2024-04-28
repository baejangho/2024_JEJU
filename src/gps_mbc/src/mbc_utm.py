#!/usr/bin/env python3
import rospy
import utm
from nmea_msgs.msg import Sentence
from std_msgs.msg import Float64

def listener():
    rospy.init_node('gps_listener', anonymous=True)

    pub_heading = rospy.Publisher('heading_topic', Float64, queue_size=30)
    pub_latitude = rospy.Publisher('latitude_topic', Float64, queue_size=30)
    pub_longitude = rospy.Publisher('longitude_topic', Float64, queue_size=30)
    pub_utm_x = rospy.Publisher('utm_x_topic', Float64, queue_size=30)
    pub_utm_y = rospy.Publisher('utm_y_topic', Float64, queue_size=30) 
    rospy.Subscriber('nmea_sentence', Sentence, lambda msg: nmea_callback(msg, pub_heading, pub_latitude, pub_longitude, pub_utm_x, pub_utm_y))
    rospy.spin()

def parse_nmea_sentence(nmea_sentence):
    parts = nmea_sentence.split(',')

    heading = None
    latitude = None
    longitude = None
    utm_coords = None

    try:
        if parts[0] == "$GNHDT":
            heading = float(parts[1].split('*')[0])  # Split off any checksum and convert to float
        elif parts[0] == "$GNGGA" and len(parts) > 4:
            latitude_deg = float(parts[2][:2])
            latitude_min = float(parts[2][2:])
            longitude_deg = float(parts[4][:3])
            longitude_min = float(parts[4][3:])

            latitude = latitude_deg + (latitude_min / 60.0)
            longitude = longitude_deg + (longitude_min / 60.0)

            if parts[3] == 'S':
                latitude = -latitude
            if parts[5] == 'W':
                longitude = -longitude

            utm_coords = utm.from_latlon(latitude, longitude)
    except ValueError:
        rospy.logwarn("Failed to parse NMEA sentence: " + nmea_sentence)

    return heading, latitude, longitude, utm_coords

def nmea_callback(msg, pub_heading, pub_latitude, pub_longitude, pub_utm_x, pub_utm_y):
    nmea_sentence = msg.sentence
    heading, latitude, longitude, utm_coords = parse_nmea_sentence(nmea_sentence)
    
    if heading is not None:
        pub_heading.publish(Float64(heading))
    if latitude is not None and longitude is not None:
        pub_latitude.publish(Float64(latitude))
        pub_longitude.publish(Float64(longitude))
    if utm_coords:
        pub_utm_x.publish(Float64(utm_coords[0]))
        pub_utm_y.publish(Float64(utm_coords[1]))

if __name__ == '__main__':
    listener()
