# Breathership
**Authors:** [Liyah Coleman](https://colemanliyah.github.io/) and [Brian Bishop](https://2b3.myportfolio.com/)  

---

## Project Overview
Breathership is an interactive installation using IMU sensors noninvasively placed on each user’s stomach—like butterflies in your stomach—to track breathing. When both users’ breaths synchronize, a programmable air system inflates a balloon, creating a visual representation of their shared rhythm.

![Full Image](images/prototype.jpg)

---

## How It Works

### Hardware
- **Arduino Nano 33 IoT** with built-in **LSM6DS3 IMU** (gyroscope + accelerometer).  
- **Programmable Air** encased with pumps to blow and vent air.  
- Fabricated **butterfly wings** to position the Arduino on the belly for accurate breath detection.

![Full Image](images/polorized_dark.jpg)
![Full Image](images/polorized_light.jpg)

### Software / Process
1. **Calibration** – Exponential moving average to calibrate and recalibrate in real time.
2. **Breath Detection** – Differences in pitch and roll indicate inhale (difference decreasing) or exhale (difference increasing).  
3. **Visual Feedback** – 
    - When in sync, a balloon will inflate
    - When out of syn,c the balloon will stop inflating 
4. **Slow Interaction** – Process is over whenever the pair says it's over.

---

## Features
- Real-time **inhale and exhale detection**.  
- Dynamic calibration for each user’s breath range.  
- Interactive, meditative **visual feedback** without screens or phones.  

---

## Images / Media
![Full Image](images/demo.gif)

---

## Blog / More Information
For a detailed write-up and exploration of the project, see our blog:  
[Project Blog]()

---

## License
MIT license 
