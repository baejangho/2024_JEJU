#!/usr/bin/env python3
# license removed for brevity

import rospy
from datetime import datetime

# from nmea_msgs.msg import Sentence
from rtcm_msgs.msg import Message

from base64 import b64encode
from threading import Thread

from http.client import HTTPConnection
from http.client import IncompleteRead

""" This is to fix the IncompleteRead error
    http://bobrochel.blogspot.com/2010/11/bad-servers-chunked-encoding-and.html"""
import http.client


def patch_http_response_read(func):
    def inner(*args):
        try:
            return func(*args)
        except http.client.IncompleteRead as e:
            return e.partial

    return inner


http.client.HTTPResponse.read = patch_http_response_read(http.client.HTTPResponse.read)


class ntripconnect(Thread):
    def __init__(self, ntc):
        super(ntripconnect, self).__init__()
        self.ntc = ntc
        self.stop = False

    def run(self):
        usrPass = self.ntc.ntrip_user + ":" + str(self.ntc.ntrip_pass)
        encoded_u = b64encode(usrPass.encode()).decode()

        headers = {
            "Ntrip-Version": "Ntrip/2.0",
            'User-Agent': 'NTRIP ntrip_ros',
            'Connection': 'close',
            # 'Authorization': 'Basic ' + b64encode(self.ntc.ntrip_user + ':' + str(self.ntc.ntrip_pass))
            "Authorization": "Basic %s" % encoded_u,
        }
        connection = HTTPConnection(self.ntc.ntrip_server)
        connection.request(
            "GET", "/" + self.ntc.ntrip_stream, self.ntc.nmea_gga, headers
        )
        response = connection.getresponse()
        if response.status != 200:
            raise Exception("blah")
        #buf = b""
        rmsg = Message()
        restart_count = 0
        while not self.stop:
            """
            data = response.read(100)
            pos = data.find('\r\n')
            if pos != -1:
                rmsg.message = buf + data[:pos]
                rmsg.header.seq += 1
                rmsg.header.stamp = rospy.get_rostime()
                buf = data[pos+2:]
                self.ntc.pub.publish(rmsg)
            else: buf += data
            """

            """ This now separates individual RTCM messages and publishes each one on the same topic """
            data = response.read(1)
            if len(data) != 0:
                if data[0] == 211:
                    buf = []
                    buf.append(data[0])
                    data = response.read(2)
                    buf.append(data[0])
                    buf.append(data[1])
                    cnt = data[0] * 256 + data[1]
                    data = response.read(2)
                    buf.append(data[0])
                    buf.append(data[1])
                    typ = (data[0] * 256 + data[1]) / 16
                    print(str(datetime.now()), cnt, typ)
                    cnt = cnt + 1
                    for x in range(cnt):
                        data = response.read(1)
                        buf.append(data[0])
                    rmsg.message = buf
                    rmsg.header.seq += 1
                    rmsg.header.stamp = rospy.get_rostime()
                    self.ntc.pub.publish(rmsg)
                    buf = []
                else:
                    print(data)
            else:
                """ If zero length data, close connection and reopen it """
                restart_count = restart_count + 1
                print("Zero length ", restart_count)
                connection.close()
                connection = HTTPConnection(self.ntc.ntrip_server)
                connection.request(
                    "GET", "/" + self.ntc.ntrip_stream, self.ntc.nmea_gga, headers
                )
                response = connection.getresponse()
                if response.status != 200:
                    raise Exception("blah")
                buf = ""
        connection.close()

class ntripclient:
    def __init__(self):
        rospy.init_node("ntripclient", anonymous=True)

        self.rtcm_topic = rospy.get_param("~rtcm_topic", "rtcm")
        self.nmea_topic = rospy.get_param("~nmea_topic", "nmea")

        self.ntrip_server = rospy.get_param("~ntrip_server")
        self.ntrip_user = rospy.get_param("~ntrip_user")
        self.ntrip_pass = rospy.get_param("~ntrip_pass")
        self.ntrip_stream = rospy.get_param("~ntrip_stream")
        # self.nmea_gga = "GPGGA,%02d%02d%04.2f,%02d%011.8f,%1s,%03d%011.8f,%1s,1,05,0.19,+00400,M,%5.3f,M,,"

        now = datetime.utcnow()
        ggaString= "GPGGA,%02d%02d%04.2f,3734.087,N,12702.603,E,1,12,1.0,0.0,M,0.0,M,," % \
            (now.hour,now.minute,now.second)
        checksum = self.calcultateCheckSum(ggaString)
        self.nmea_gga = "$%s*%s" % (ggaString, checksum)

        self.pub = rospy.Publisher(self.rtcm_topic, Message, queue_size=10)

        self.connection = None
        self.connection = ntripconnect(self)
        self.connection.start()

    def calcultateCheckSum(self, stringToCheck):
        xsum_calc = 0
        for char in stringToCheck:
            xsum_calc = xsum_calc ^ ord(char)
        return "%02X" % xsum_calc

    def run(self):
        rospy.spin()
        if self.connection is not None:
            self.connection.stop = True


if __name__ == "__main__":
    c = ntripclient()
    c.run()
