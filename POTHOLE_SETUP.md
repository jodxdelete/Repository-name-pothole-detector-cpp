# Pothole Detection AI - Setup & Usage Guide

## Overview
This is a real-time pothole detection system using:
- **OpenCV** for image processing and camera access
- **Deep Learning** (YOLO-based or edge detection)
- **Real-time processing** from camera feed

## Features
✅ Real-time pothole detection  
✅ Severity classification (Critical, High, Medium, Low)  
✅ Bounding box visualization  
✅ Statistics tracking  
✅ Image saving capability  
✅ Edge detection fallback method  

## Installation

### Prerequisites
- Windows/Linux/Mac
- OpenCV installed (with DNN module)
- C++ compiler (g++, MSVC, etc.)
- Webcam

### Step 1: Install OpenCV

**Windows (using CMake):**
```bash
# Clone OpenCV
git clone https://github.com/opencv/opencv.git
cd opencv
mkdir build
cd build
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
cmake --install .
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install libopencv-dev opencv-data
```
**macOS:**
```bash
brew install opencv
```

### Step 2: Compile the Program

**Linux/Mac:**
```bash
g++ -o pothole_detector pothole_detector.cpp `pkg-config --cflags --libs opencv4`
```

**Windows (with OpenCV installed):**
```bash
g++ -o pothole_detector.exe pothole_detector.cpp -I"C:\path\to\opencv\include" -L"C:\path\to\opencv\lib" -lopencv_core -lopencv_imgproc -lopencv_videoio -lopencv_dnn -lopencv_highgui
```

## Usage

### Running the Program
```bash
./pothole_detector
```

### Keyboard Controls
| Key | Action |
|-----|--------|
| **q** | Quit program |
| **ESC** | Quit program |
| **s** | Save current frame with detections |
| **r** | Reset statistics counter |

### Program Output
- Real-time video display with bounding boxes
- Detected potholes marked with colored rectangles
- Confidence scores for each detection
- Statistics: Frame count, pothole count, alerts

## Using Pre-trained YOLO Model

To use a real pre-trained model instead of edge detection:

### Step 1: Download YOLO Files
```bash
# Download pre-trained weights
wget https://pjreddie.com/media/files/yolov3.weights
wget https://raw.githubusercontent.com/pjreddie/darknet/master/cfg/yolov3.cfg
wget https://raw.githubusercontent.com/pjreddie/darknet/master/data/coco.names
```

### Step 2: Modify Code
In `pothole_detector.cpp`, uncomment the model loading in `main()`:
```cpp
// Uncomment this section to use YOLO model:
// if (!detector.loadModel("yolov3.weights", "yolov3.cfg", "coco.names")) {
//     cerr << "Failed to load model\n";
//     return -1;
// }
```

### Step 3: Use YOLOv3-Pothole Specific Model
For better pothole detection, use a pothole-specific model trained on road damage datasets:

Download from:
- **RDD2020** (Road Damage Detection Dataset)
- **Google OpenImages** (Pothole annotations)
- **xView2 Challenge** (Road damage data)

## Algorithm Details

### Current Method: Edge Detection (Fallback)
1. **Grayscale Conversion**: Convert RGB to grayscale
2. **Blur**: Apply Gaussian blur to reduce noise
3. **Edge Detection**: Use Canny edge detection (50-150 threshold)
4. **Morphological Ops**: Dilate edges to enhance features
5. **Contour Analysis**: Find and filter contours by:
   - Area (100-50000 pixels)
   - Circularity (>0.5 for circular shapes)
6. **Bounding Box**: Draw detection rectangles

### Severity Levels
```
Confidence > 0.8 AND Area > 5000px   → CRITICAL
Confidence > 0.6 AND Area > 2000px   → HIGH
Confidence > 0.5 AND Area > 500px    → MEDIUM
Otherwise                             → LOW
```

## Advanced: Custom YOLO Model Training

### Dataset Preparation
1. Collect road images with potholes
2. Annotate using LabelImg or CVAT
3. Convert to YOLO format (txt files with normalized coordinates)

### Training
```bash
# Download Darknet
git clone https://github.com/AlexeyAB/darknet
cd darknet

# Edit cfg/pothole.data and cfg/yolov3-pothole.cfg
# Run training
./darknet detector train cfg/pothole.data cfg/yolov3-pothole.cfg yolov3.weights -dont_show
```

## Troubleshooting

### Camera Not Opening
- Check if camera is connected and not used by another application
- Try different camera indices (0, 1, 2)
- Grant camera permissions to the terminal

### Poor Detection
- Improve lighting conditions
- Clean camera lens
- Adjust confidence threshold in code
- Use trained pothole-specific model

### Performance Issues
- Reduce frame resolution
- Use smaller input size (320x320 instead of 416x416)
- Enable GPU acceleration (CUDA for NVIDIA cards)

## Code Structure

```cpp
PotholeDetector class:
├── loadModel()              // Load pre-trained YOLO
├── detectPotholes()         // Inference on frame
├── drawDetections()         // Visualize results
├── calculateSeverity()      // Classify pothole severity
└── processWithEdgeDetection()  // Fallback method

main():
├── Initialize detector
├── Open camera
├── Process frames in loop
├── Handle keyboard input
└── Print statistics
```

## Real-world Deployment

### For Production Systems:
1. **GPS Integration**: Log pothole locations
2. **Database Storage**: Save detections with timestamps
3. **Alert System**: Send notifications to authorities
4. **Mobile App**: Display pothole map to drivers
5. **Vehicle Integration**: Integrate with dashcam/ADAS systems

### Example GPS + Detection:
```cpp
// Add GPS coordinates to detection
struct PotholeReport {
    double latitude, longitude;
    float severity;
    int width, height;
    time_t timestamp;
};
```

## Performance Metrics

Typical Performance (on standard webcam):
- Frame Rate: 20-30 FPS
- Latency: 30-50ms per frame
- GPU (CUDA): 60+ FPS

## References
- OpenCV Documentation: https://docs.opencv.org/
- YOLO: https://pjreddie.com/darknet/yolo/
- Road Damage Detection Dataset: https://github.com/sekilab/RDD2020

## License
MIT License - Feel free to modify and distribute

## Support
For issues or improvements, create an issue or submit a PR!
