# unitreeg1_ROS_mic
A small ROS Noetic node that publishes the audio from the microphones of the G1 robot from Unitree Robotics


## Install

### Create the workspace

If you don't have a workscape create it with:

```ROS
mkdir -p ~/g1_mic_ws/src
cd ~/g1_mic_ws/src
```

### Clone the packages
Once in src you have to clone the SinfonIA Framework ROS messages and this package:
```bash
git clone https://github.com/ros-naoqi/naoqi_bridge_msgs.git
git clone https://github.com/dcuevasa/unitreeg1_ROS_mic/tree/main
```

### Compile workspace

```ROS
cd ~/g1_mic_ws
catkin_make
```

And source:

```ROS
source devel/setup.bash
```
