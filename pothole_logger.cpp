#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <ctime>
#include <iomanip>

using namespace std;

// Structure to store pothole detection report
struct PotholeReport {
    int frameNumber;
    int xCoord, yCoord;
    int width, height;
    float confidence;
    string severity;
    long long timestamp;
    double latitude;   // For GPS integration
    double longitude;
    string roadName;
    
    // Convert to JSON format
    string toJSON() {
        ostringstream oss;
        oss << "{\n";
        oss << "  \"frame\": " << frameNumber << ",\n";
        oss << "  \"location\": {\"x\": " << xCoord << ", \"y\": " << yCoord << "},\n";
        oss << "  \"size\": {\"width\": " << width << ", \"height\": " << height << "},\n";
        oss << "  \"area\": " << (width * height) << ",\n";
        oss << "  \"confidence\": " << fixed << setprecision(3) << confidence << ",\n";
        oss << "  \"severity\": \"" << severity << "\",\n";
        oss << "  \"timestamp\": " << timestamp << ",\n";
        oss << "  \"coordinates\": {\"lat\": " << fixed << setprecision(6) << latitude << ", \"lng\": " << longitude << "},\n";
        oss << "  \"roadName\": \"" << roadName << "\"\n";
        oss << "}";
        return oss.str();
    }
    
    // Convert to CSV format
    string toCSV() {
        ostringstream oss;
        oss << frameNumber << "," << xCoord << "," << yCoord << "," << width << "," 
            << height << "," << fixed << setprecision(3) << confidence << "," 
            << severity << "," << timestamp << "," << fixed << setprecision(6) 
            << latitude << "," << longitude << "," << roadName;
        return oss.str();
    }
};

// Data Logger Class
class DetectionLogger {
private:
    string logFile;
    string jsonFile;
    string csvFile;
    vector<PotholeReport> reports;
    
public:
    DetectionLogger(string prefix = "pothole_detection") {
        time_t now = time(0);
        tm* timeinfo = localtime(&now);
        ostringstream oss;
        oss << prefix << "_" << put_time(timeinfo, "%Y%m%d_%H%M%S");
        
        logFile = oss.str() + ".log";
        jsonFile = oss.str() + ".json";
        csvFile = oss.str() + ".csv";
        
        // Initialize CSV with header
        ofstream csv(csvFile, ios::app);
        csv << "Frame,X,Y,Width,Height,Confidence,Severity,Timestamp,Latitude,Longitude,RoadName\n";
        csv.close();
        
        cout << "Logger initialized:\n";
        cout << "  LOG: " << logFile << "\n";
        cout << "  JSON: " << jsonFile << "\n";
        cout << "  CSV: " << csvFile << "\n\n";
    }
    
    void addReport(PotholeReport report) {
        reports.push_back(report);
        
        // Log to file immediately
        ofstream log(logFile, ios::app);
        log << "[" << report.timestamp << "] Frame " << report.frameNumber 
            << ": Pothole detected at (" << report.xCoord << ", " << report.yCoord 
            << ") - Severity: " << report.severity << "\n";
        log.close();
        
        // Append to CSV
        ofstream csv(csvFile, ios::app);
        csv << report.toCSV() << "\n";
        csv.close();
    }
    
    void saveJSON() {
        ofstream json(jsonFile);
        json << "{\n  \"detections\": [\n";
        
        for (size_t i = 0; i < reports.size(); i++) {
            json << "    " << reports[i].toJSON();
            if (i < reports.size() - 1) json << ",";
            json << "\n";
        }
        
        json << "  ],\n";
        json << "  \"summary\": {\n";
        json << "    \"total_detections\": " << reports.size() << ",\n";
        
        // Calculate severity distribution
        int critical = 0, high = 0, medium = 0, low = 0;
        for (auto& r : reports) {
            if (r.severity == "CRITICAL") critical++;
            else if (r.severity == "HIGH") high++;
            else if (r.severity == "MEDIUM") medium++;
            else if (r.severity == "LOW") low++;
        }
        
        json << "    \"critical_count\": " << critical << ",\n";
        json << "    \"high_count\": " << high << ",\n";
        json << "    \"medium_count\": " << medium << ",\n";
        json << "    \"low_count\": " << low << "\n";
        json << "  }\n}\n";
        json.close();
        
        cout << "JSON report saved: " << jsonFile << "\n";
    }
    
    void printStatistics() {
        cout << "\n=== Detection Statistics ===\n";
        cout << "Total Potholes Detected: " << reports.size() << "\n\n";
        
        if (reports.empty()) {
            cout << "No potholes detected.\n";
            return;
        }
        
        // Severity distribution
        int critical = 0, high = 0, medium = 0, low = 0;
        float avgConfidence = 0;
        int totalArea = 0;
        
        for (auto& r : reports) {
            if (r.severity == "CRITICAL") critical++;
            else if (r.severity == "HIGH") high++;
            else if (r.severity == "MEDIUM") medium++;
            else if (r.severity == "LOW") low++;
            
            avgConfidence += r.confidence;
            totalArea += r.width * r.height;
        }
        
        cout << "Severity Breakdown:\n";
        cout << "  Critical: " << critical << " (" << (100.0*critical/reports.size()) << "%)\n";
        cout << "  High:     " << high << " (" << (100.0*high/reports.size()) << "%)\n";
        cout << "  Medium:   " << medium << " (" << (100.0*medium/reports.size()) << "%)\n";
        cout << "  Low:      " << low << " (" << (100.0*low/reports.size()) << "%)\n\n";
        
        cout << "Average Confidence: " << fixed << setprecision(3) 
             << (avgConfidence / reports.size()) << "\n";
        cout << "Average Pothole Size: " << (totalArea / reports.size()) << " pixels\n";
        cout << "Largest Pothole: " << getTotalArea() << " pixels\n";
    }
    
    int getTotalArea() {
        int maxArea = 0;
        for (auto& r : reports) {
            maxArea = max(maxArea, r.width * r.height);
        }
        return maxArea;
    }
    
    int getReportCount() {
        return reports.size();
    }
};

// Analytics Class
class PotholeAnalytics {
public:
    // Classify severity based on characteristics
    static string classifySeverity(int width, int height, float confidence) {
        int area = width * height;
        float aspectRatio = (float)max(width, height) / (min(width, height) + 1);
        
        // Multi-factor severity analysis
        if (confidence > 0.85 && area > 7000) {
            return "CRITICAL";
        } else if ((confidence > 0.75 && area > 3500) || (confidence > 0.9 && area > 2000)) {
            return "HIGH";
        } else if ((confidence > 0.65 && area > 1000) || (confidence > 0.8 && area > 500)) {
            return "MEDIUM";
        } else if (area > 200) {
            return "LOW";
        } else {
            return "INSIGNIFICANT";
        }
    }
    
    // Estimate repair priority
    static int estimateRepairCost(string severity) {
        if (severity == "CRITICAL") return 500;      // $500
        if (severity == "HIGH") return 300;          // $300
        if (severity == "MEDIUM") return 150;        // $150
        if (severity == "LOW") return 50;            // $50
        return 0;
    }
    
    // Generate road conditions report
    static void generateRoadReport(vector<PotholeReport>& reports, string roadName) {
        cout << "\n=== Road Condition Report ===\n";
        cout << "Road: " << roadName << "\n";
        cout << "Inspection Date: " << __DATE__ << " " << __TIME__ << "\n\n";
        
        int totalCost = 0;
        int critical = 0, high = 0, medium = 0, low = 0;
        
        for (auto& r : reports) {
            if (r.roadName == roadName) {
                if (r.severity == "CRITICAL") critical++;
                else if (r.severity == "HIGH") high++;
                else if (r.severity == "MEDIUM") medium++;
                else if (r.severity == "LOW") low++;
                
                totalCost += estimateRepairCost(r.severity);
            }
        }
        
        int total = critical + high + medium + low;
        
        cout << "Potholes Found: " << total << "\n";
        cout << "  - Critical: " << critical << "\n";
        cout << "  - High: " << high << "\n";
        cout << "  - Medium: " << medium << "\n";
        cout << "  - Low: " << low << "\n\n";
        
        cout << "Estimated Repair Cost: $" << totalCost << "\n";
        
        if (critical > 0) {
            cout << "⚠️ WARNING: Critical potholes require immediate attention!\n";
        }
    }
};

// Example usage function
void demonstrateLogging() {
    cout << "\n=== Pothole Detection Logging System Demo ===\n\n";
    
    DetectionLogger logger("pothole_inspection");
    
    // Simulate some detections
    vector<PotholeReport> detections = {
        {1, 150, 200, 85, 90, 0.92f, "CRITICAL", 1630703400, 28.6139, 77.2090, "MG Road"},
        {5, 280, 120, 65, 70, 0.87f, "HIGH", 1630703405, 28.6140, 77.2091, "MG Road"},
        {12, 420, 350, 45, 50, 0.75f, "MEDIUM", 1630703412, 28.6141, 77.2092, "MG Road"},
        {18, 320, 280, 30, 35, 0.68f, "LOW", 1630703418, 28.6142, 77.2093, "MG Road"},
        {25, 200, 400, 120, 110, 0.95f, "CRITICAL", 1630703425, 28.6143, 77.2094, "MG Road"}
    };
    
    cout << "Logging " << detections.size() << " detections...\n\n";
    
    for (auto& detection : detections) {
        logger.addReport(detection);
        cout << "✓ Logged pothole at (" << detection.xCoord << ", " << detection.yCoord 
             << ") - " << detection.severity << "\n";
    }
    
    logger.saveJSON();
    logger.printStatistics();
    
    // Generate road report
    PotholeAnalytics::generateRoadReport(detections, "MG Road");
    
    // Show repair cost estimates
    cout << "\n=== Repair Cost Estimates ===\n";
    cout << "CRITICAL: $" << PotholeAnalytics::estimateRepairCost("CRITICAL") << "\n";
    cout << "HIGH: $" << PotholeAnalytics::estimateRepairCost("HIGH") << "\n";
    cout << "MEDIUM: $" << PotholeAnalytics::estimateRepairCost("MEDIUM") << "\n";
    cout << "LOW: $" << PotholeAnalytics::estimateRepairCost("LOW") << "\n";
}

int main() {
    demonstrateLogging();
    return 0;
}
