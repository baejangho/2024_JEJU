# 씨너렉스 gps수신기 (TDR-3000)로 gps 수신 및 utm 변환값을 받기 위한 라이브러리.

**[필요항목]**  
https://wiki.ros.org/nmea_navsat_driver 패키지 필요  
https://pypi.org/project/utm/0.4.0/ 파이썬 utm 필요  

**[실행]**  
roslaunch gps_mbc mbc.launch

**[topic list]**  
/utm_x  
/utm_y  
/latitude_topic  
/longitude_topic  
/heading_topic  
참고) heading_topic은 echo로 확인하여야 함.
