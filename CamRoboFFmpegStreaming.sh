#/bin/bash
#######################################
# FFMPEG STREAMING
#######################################
# NOTE
#   Change IP address according to your environment.
#######################################
echo 'Before starting ffmpeg, you must standby UDP streaming with vlc mediaplayer. udp://@:1234'

ffmpeg -f alsa -ac 1 -thread_queue_size 8192 -i hw:1                    \
       -f v4l2 -input_format yuyv422 -video_size 640x480 -i /dev/video0 -vf "rotate=PI" \
       -c:v h264_omx -b:v 512k -c:a aac -b:a 64k -async 100 -f mpegts   \
       udp://192.168.1.100:1234
