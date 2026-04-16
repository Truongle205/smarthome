SMART HOME
Introduction

This project implements a Smart Home system using Yolo:Uno as the central controller. The system is designed to monitor environmental conditions, control household devices, support focused study sessions with Pomodoro, and send remote alerts.

Main Features


1. Environmental Data Collection (UC1)
-Reads temperature and humidity from sensors
-Sends data to the server for storage and monitoring
-Operates periodically


2. LCD Display (UC2)
-Displays temperature and humidity
-Displays Pomodoro timer
-Updates when new data is available


3. Light Control
-Manual control (UC3): turn on/off via user command
-Automatic control (UC4): scheduled operation (on at 23:00, off at 06:00)


4. Fan Control (UC5)
-Automatically turns on when temperature exceeds a threshold
-Can also be controlled manually


5. Pomodoro Mode (UC6)
-25 minutes work and 5 minutes break cycle
-Helps improve focus and productivity


6. Door Monitoring (UC7)
-Detects door open/close state
-Updates status in real time


7. Notification System (UC8)
-Sends alerts via Gmail or Telegram when the door is opened
-Works through server communication


8. Anti-Distraction Mechanism (UC9)
-Uses tilt sensor to detect device movement
-Triggers buzzer if abnormal movement is detected during Pomodoro
-Helps maintain user focus


System Architecture
-Controller: Yolo:Uno
-Sensors: temperature/humidity, door sensor, tilt sensor
-Actuators: relay (light, fan), LCD, buzzer
-Server: data storage and notification handling
-Connectivity: WiFi

Workflow
-Sensors collect environmental data
-Yolo:Uno processes and sends data to the server
-Server stores data and triggers actions
-Users interact with the system or receive notifications

Error Handling
-Network failure: data is stored locally and resent later
-Sensor failure: system skips or reports error
-Device failure: retry or notify user

Objectives
-Build a basic IoT-based Smart Home system
-Integrate multiple functionalities into one platform
-Apply the system to real-life scenarios
