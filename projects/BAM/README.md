# Breath Activated Meditation (BAM) - Wearable Microcontroller
**Authors:** [Liyah Coleman](https://colemanliyah.github.io/) and [Brian Bishop](https://2b3.myportfolio.com/)  

---

## Project Overview
A screen-free wearable that guides breathing and meditation. Using IMU sensor data, it tracks users’ breath in real time and syncs a polarized lens to subtly darken and brighten, leading users through a timed meditation.

![Full Image](images/prototype.jpg)

---

## How It Works

### Hardware
- **Arduino Nano 33 IoT** with built-in **LSM6DS3 IMU** (gyroscope + accelerometer).  
- **Polarized lenses** mounted on a servo motor for visual output.  
- Wearable **necklace design** to position the Arduino near the chest for accurate breath detection.

![Full Image](images/polorized_dark.jpg)
![Full Image](images/polorized_light.jpg)

### Software / Process
1. **Calibration** – Measures the user’s baseline chest motion to define a starting point for inhale/exhale detection.  
2. **Breath Detection** – Differences in pitch and roll indicate inhale (difference decreasing) or exhale (difference increasing).  
3. **Visual Feedback** – 
    - The first four long breaths rotate the lens to darken.  
    - The next four breaths allow meditation without feedback.  
    - The final four breaths rotate the lens back to lighten.  
4. **Cycle Completion** – A full inhale/exhale cycle triggers the system’s output, such as moving the lens, activating LEDs, or sending a signal.

---

## Features
- Real-time **inhale and exhale detection**.  
- Dynamic calibration for each user’s breath range.  
- Interactive, meditative **visual feedback** without screens or phones.  
- Potential to extend into **biofeedback art installations**.

---

## Images / Media
![Full Image](images/demo.gif)

---

## Blog / More Information
For a detailed write-up and exploration of the project, see our blog:  
[Project Blog](https://sites.google.com/view/liyahsportfolio/home/wearable-meditation?authuser=0)

---

## License
MIT license 
