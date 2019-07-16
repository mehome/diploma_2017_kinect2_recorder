# Diploma 2017. Kinect 2 Recorder
To record depth (and color) videos using Kinect 2

### Details
* Video file container: mkv
* Color video: wmv2 codec and yuv420p pixel format
* Depth video: ffv1 codec and gray16 pixel format
* If you record color and depth videos, they will be saved as 2 video streams in one video file
* File name format: 'YYYY-MM-DD-HH-MM-SS' (local beginning date & time)

### Features
* Ability to set mode: none, color, depth, color and depth (before you start recording)
* Ability to set size (for each mode, before you start recording)
* Ability to set directory path (before you start recording)
* Ability to set size (before you start recording)
* Ability to record video with/without fixed time of recording (you can stop it, even it recording time is fixed)

### Dependencies
1. Kinect for Windows SDK 2.0
2. OpenCV 3.2
3. FFmpeg 3.2.2
4. QT

### Contributors
1. [Alexander Menkin](https://github.com/miloiloloo)
2. [Victor Kulikov](https://github.com/kulikovv) (VideoIO is from [ProcessingSDK](https://github.com/kulikovv/ProcessingSDK))

### Other
[License](https://github.com/miloiloloo/diploma_2017_kinect2_recorder/blob/master/LICENSE)
