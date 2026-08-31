#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <vector>
#include <cmath>

using namespace std;
using namespace cv;
using namespace cv::dnn;

class PotholeDetector {
private:
    Net net;
    vector<string> classNames;
    vector<Scalar> colors;
    float confThreshold = 0.5;  // Confidence threshold
    float nmsThreshold = 0.4;   // Non-maximum suppression threshold
    int inpWidth = 416;         // Width of network's input image
    int inpHeight = 416;        // Height of network's input image
    
public:
    PotholeDetector() {
        // Initialize class names for detection
        classNames = {"pothole", "road_damage", "crack"};
        
        // Generate random colors for each class
        for (int i = 0; i < classNames.size(); i++) {
            colors.push_back(Scalar(rand() % 256, rand() % 256, rand() % 256));
        }
    }
    
    // Load pre-trained YOLO model
    bool loadModel(string weightsPath, string configPath, string namesPath) {
        try {
            // Load YOLO network
            net = readNetFromDarknet(configPath, weightsPath);
            
            // Set backend to CPU (or CUDA if available)
            net.setPreferableBackend(DNN_BACKEND_OPENCV);
            net.setPreferableTarget(DNN_TARGET_CPU);
            
            // Load class names from file
            if (!namesPath.empty()) {
                ifstream ifs(namesPath.c_str());
                string line;
                while (getline(ifs, line)) {
                    classNames.push_back(line);
                }
                ifs.close();
            }
            
            cout << "Model loaded successfully!\n";
            cout << "Input size: " << inpWidth << "x" << inpHeight << "\n";
            return true;
        } catch (const exception& e) {
            cerr << "Error loading model: " << e.what() << "\n";
            return false;
        }
    }
    
    // Detect potholes in a frame
    vector<vector<int>> detectPotholes(Mat& frame, vector<float>& confidences, vector<string>& detectedLabels) {
        vector<vector<int>> detections;
        confidences.clear();
        detectedLabels.clear();
        
        if (net.empty()) {
            cerr << "Network not loaded!\n";
            return detections;
        }
        
        // Create blob from frame
        Mat blob = blobFromImage(frame, 1/255.0, Size(inpWidth, inpHeight), Scalar(0,0,0), true, false);
        
        // Set input to network
        net.setInput(blob);
        
        // Forward pass
        vector<Mat> outs;
        outs = net.forward(net.getUnconnectedOutLayersNames());
        
        // Process output
        float scaleX = (float)frame.cols / inpWidth;
        float scaleY = (float)frame.rows / inpHeight;
        
        vector<int> classIds;
        vector<float> scores;
        vector<Rect> boxes;
        
        for (size_t i = 0; i < outs.size(); ++i) {
            float* data = (float*)outs[i].data;
            for (int j = 0; j < outs[i].rows; ++j, data += outs[i].cols) {
                Mat scores_row = outs[i].row(j).colRange(5, outs[i].cols);
                Point classIdPoint;
                double confidence;
                minMaxLoc(scores_row, 0, &confidence, 0, &classIdPoint);
                
                if (confidence > confThreshold) {
                    int centerX = (int)(data[0] * scaleX);
                    int centerY = (int)(data[1] * scaleY);
                    int width = (int)(data[2] * scaleX);
                    int height = (int)(data[3] * scaleY);
                    int left = centerX - width / 2;
                    int top = centerY - height / 2;
                    
                    classIds.push_back(classIdPoint.x);
                    scores.push_back((float)confidence);
                    boxes.push_back(Rect(left, top, width, height));
                }
            }
        }
        
        // Apply Non-Maximum Suppression
        vector<int> indices;
        NMSBoxes(boxes, scores, confThreshold, nmsThreshold, indices);
        
        // Collect final detections
        for (int idx : indices) {
            vector<int> detection = {boxes[idx].x, boxes[idx].y, boxes[idx].width, boxes[idx].height, classIds[idx]};
            detections.push_back(detection);
            confidences.push_back(scores[idx]);
            
            if (classIds[idx] < classNames.size()) {
                detectedLabels.push_back(classNames[classIds[idx]]);
            } else {
                detectedLabels.push_back("Unknown");
            }
        }
        
        return detections;
    }
    
    // Draw detections on frame
    void drawDetections(Mat& frame, vector<vector<int>>& detections, vector<float>& confidences, vector<string>& labels) {
        for (size_t i = 0; i < detections.size(); i++) {
            int x = detections[i][0];
            int y = detections[i][1];
            int width = detections[i][2];
            int height = detections[i][3];
            int classId = detections[i][4];
            
            // Draw bounding box
            Scalar color = (classId < colors.size()) ? colors[classId] : Scalar(0, 255, 0);
            rectangle(frame, Rect(x, y, width, height), color, 2);
            
            // Prepare label
            string label = format("%.2f", confidences[i]);
            if (i < labels.size()) {
                label = labels[i] + ": " + label;
            }
            
            // Draw label background
            int baseLine = 0;
            Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.6, 1, &baseLine);
            rectangle(frame, Point(x, y - labelSize.height - baseLine), 
                     Point(x + labelSize.width, y), color, FILLED);
            
            // Put text
            putText(frame, label, Point(x, y - baseLine), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255,255,255), 1);
        }
    }
    
    // Calculate severity based on pothole size
    string calculateSeverity(int width, int height, float confidence) {
        int area = width * height;
        
        if (confidence > 0.8 && area > 5000) {
            return "CRITICAL";
        } else if (confidence > 0.6 && area > 2000) {
            return "HIGH";
        } else if (confidence > 0.5 && area > 500) {
            return "MEDIUM";
        } else {
            return "LOW";
        }
    }
    
    // Process video stream with simple model when pre-trained not available
    void processWithEdgeDetection(Mat& frame, vector<vector<int>>& detections) {
        detections.clear();
        
        // Convert to grayscale
        Mat gray;
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        
        // Apply Gaussian blur
        Mat blurred;
        GaussianBlur(gray, blurred, Size(5, 5), 1.5);
        
        // Edge detection using Canny
        Mat edges;
        Canny(blurred, edges, 50, 150);
        
        // Morphological operations to enhance features
        Mat kernel = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
        Mat dilated;
        dilate(edges, dilated, kernel, 2);
        
        // Find contours
        vector<vector<Point>> contours;
        findContours(dilated.clone(), contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
        
        // Analyze contours to find potholes
        for (const auto& contour : contours) {
            double area = contourArea(contour);
            
            // Filter by area (potential potholes)
            if (area > 100 && area < 50000) {
                Rect bbox = boundingRect(contour);
                double circularity = 4 * M_PI * area / (pow(arcLength(contour, true), 2) + 1e-5);
                
                // Potholes tend to be circular
                if (circularity > 0.5) {
                    vector<int> detection = {bbox.x, bbox.y, bbox.width, bbox.height, 0};
                    detections.push_back(detection);
                }
            }
        }
    }
    
    int getInputWidth() { return inpWidth; }
    int getInputHeight() { return inpHeight; }
};

int main() {
    cout << "\n=== Pothole Detection System ===\n";
    cout << "Real-time Pothole Detection using Camera\n\n";
    
    PotholeDetector detector;
    
    // Try to open camera
    VideoCapture camera(0);
    
    if (!camera.isOpened()) {
        cerr << "Error: Cannot open camera!\n";
        cerr << "Try: \n";
        cerr << "1. Check if camera is connected\n";
        cerr << "2. Check camera permissions\n";
        cerr << "3. Try another camera index (1, 2, etc.)\n";
        return -1;
    }
    
    cout << "Camera opened successfully!\n";
    cout << "Instructions:\n";
    cout << "- Press 'q' to quit\n";
    cout << "- Press 's' to save detection image\n";
    cout << "- Press 'r' to reset statistics\n\n";
    
    // Set camera properties
    camera.set(CAP_PROP_FRAME_WIDTH, 640);
    camera.set(CAP_PROP_FRAME_HEIGHT, 480);
    camera.set(CAP_PROP_FPS, 30);
    
    Mat frame;
    int totalPotholesDetected = 0;
    int frameCount = 0;
    int saveCounter = 0;
    
    // Main detection loop
    while (true) {
        if (!camera.read(frame)) {
            cerr << "Failed to read frame!\n";
            break;
        }
        
        frameCount++;
        
        // Detect potholes using edge detection (fallback method)
        vector<vector<int>> detections;
        vector<float> confidences;
        vector<string> labels;
        
        // Use the fallback edge detection method
        detector.processWithEdgeDetection(frame, detections);
        
        // Prepare dummy confidences and labels for visualization
        for (size_t i = 0; i < detections.size(); i++) {
            confidences.push_back(0.75f);
            labels.push_back("pothole");
        }
        
        totalPotholesDetected += detections.size();
        
        // Draw detections
        detector.drawDetections(frame, detections, confidences, labels);
        
        // Add statistics on frame
        string stats = "Frame: " + to_string(frameCount) + " | Potholes: " + to_string(detections.size()) + 
                      " | Total: " + to_string(totalPotholesDetected);
        putText(frame, stats, Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2);
        
        // Show severity information
        if (!detections.empty()) {
            string alert = "ALERT: Potholes Detected!";
            putText(frame, alert, Point(10, 70), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 0, 255), 2);
        }
        
        // Display frame
        imshow("Pothole Detection - Press 'q' to quit", frame);
        
        int key = waitKey(30);
        
        if (key == 'q' || key == 27) {  // 'q' or ESC
            cout << "Exiting...\n";
            break;
        } else if (key == 's') {  // Save image
            string filename = "pothole_detection_" + to_string(saveCounter++) + ".jpg";
            imwrite(filename, frame);
            cout << "Image saved: " << filename << "\n";
        } else if (key == 'r') {  // Reset statistics
            totalPotholesDetected = 0;
            frameCount = 0;
            cout << "Statistics reset!\n";
        }
    }
    
    // Release resources
    camera.release();
    destroyAllWindows();
    
    cout << "\n=== Summary ===\n";
    cout << "Total frames processed: " << frameCount << "\n";
    cout << "Total potholes detected: " << totalPotholesDetected << "\n";
    cout << "Average per frame: " << (frameCount > 0 ? totalPotholesDetected / frameCount : 0) << "\n";
    
    return 0;
}
