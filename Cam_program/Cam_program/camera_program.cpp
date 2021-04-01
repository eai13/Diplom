#define _CRT_SECURE_NO_WARNINGS
#define _CRT_DEPRECATE_NO_WARNINGS

#define DISABLED			0
#define ENABLED				1
#define DISCONNECTED		0
#define CONNECTED			1

#define CAMERA				0
#define CONN_ENABLE			1
#define CONN_SET			2
#define DATA				3

#define MMperPIX 1.84
#define PI 3.14159265

#include <filesystem>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <string>
#include <opencv2/aruco.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <time.h>
#include <direct.h>
#include <Windows.h>
#include "measurements.h"

HANDLE map_file = NULL;
float * map_file_data;

cv::VideoCapture camera;
td::TransferData transfer;
cv::Point2d arucoCorner[3];
std::vector<int> markerIds;
std::vector<std::vector<cv::Point2f>> markerCorners, rejectedCandidates;

int main() {
	// Connecting to the map file
	map_file = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "Camera_map");
	std::cout << "Connecting to the map file: ";
	while (map_file == NULL) map_file = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, "Camera_map");
	std::cout << "CONNECTED" << std::endl;
	map_file_data = (float*)MapViewOfFile(map_file, FILE_MAP_ALL_ACCESS, NULL, NULL, 256);

	// Start checking the flags from the main process
	while (true) {
		if (camera.isOpened()) camera.release();
		std::cout << "Waiting for connection command: ";
		// Waiting for the connection command from the main process
		while (map_file_data[CONN_ENABLE] != ENABLED) std::cout << "";
		std::cout << "RECEIVED" << std::endl;
		// Establishing connection
		std::cout << "Connection status: ";
		if (!camera.open(map_file_data[CAMERA])) {
			std::cout << "FAILED" << std::endl;
			map_file_data[CONN_SET] = DISCONNECTED;
			std::cout << "DISCONNECTED" << std::endl;
			continue;
		}
		else {
			std::cout << "SUCCESSFUL" << std::endl;
			map_file_data[CONN_SET] = CONNECTED;
		}
		// Camera setup
		camera.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
		camera.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
		camera.set(cv::CAP_PROP_AUTOFOCUS, 0);
		camera.set(cv::CAP_PROP_AUTO_EXPOSURE, 1);
		// Data preparation
		transfer.currAngle = (double)0;
		transfer.currGlobalCartesian.x = 0;
		transfer.currGlobalCartesian.y = 0;
		for (int i = 0; i < 3; i++) {
			arucoCorner[i].x = 0;
			arucoCorner[i].y = 0;
		}
		// Aruco parameters
		cv::Ptr<cv::aruco::DetectorParameters> parameters = cv::aruco::DetectorParameters::create();
		cv::Ptr<cv::aruco::Dictionary> dictionary = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_4X4_50);
		// Frames
		cv::Mat currentVideoFrame;
		cv::Mat outputImage;
		// Reading the first video frame
		if (!camera.read(currentVideoFrame)) {
			std::cout << "Connection status: FAILED" << std::endl;
			map_file_data[CONN_SET] = DISCONNECTED;
			std::cout << "DISCONNECTED" << std::endl;
			continue;
		}
		// Cutting the video frame
		currentVideoFrame = currentVideoFrame(cv::Rect(340, 0, 1150, 1080));
		markerIds.clear();
		cv::aruco::detectMarkers(currentVideoFrame, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);
		currentVideoFrame.copyTo(outputImage);
		if (!markerIds.empty()) {
			for (int i = 0; i < 3; i++) {
				arucoCorner[i].x = markerCorners[0][i].x - 0.105 * (markerCorners[0][i].x - 575);
				arucoCorner[i].y = markerCorners[0][i].y - 0.105 * (markerCorners[0][i].y - 540);
			}
		}
		else {
			for (int i = 0; i < 3; i++) {
				arucoCorner[i].x = 1;
				arucoCorner[i].y = 1;
			}
		}
		float dist = sqrt(pow(arucoCorner[0].x - arucoCorner[1].x, 2) + pow(arucoCorner[0].y - arucoCorner[1].y, 2));
		// calculating current cartesian position in pixels 
		transfer.prevGlobalCartesian.x = transfer.currGlobalCartesian.x;
		transfer.prevGlobalCartesian.y = transfer.currGlobalCartesian.y;
		transfer.currGlobalCartesian.x = (arucoCorner[0].x + arucoCorner[2].x) / 2;
		transfer.currGlobalCartesian.y = (arucoCorner[0].y + arucoCorner[2].y) / 2;
		// finding the angle of the aruco vector              
		transfer.prevAngle = transfer.currAngle;
		transfer.Angle(&transfer, arucoCorner);
		transfer.DeltaEigen(&transfer);
		while (1) {
			std::cout << "inf while" << std::endl;
			std::cout << map_file_data[CONN_ENABLE] << std::endl;
			if ((!camera.read(currentVideoFrame)) || (map_file_data[CONN_ENABLE] == DISABLED)) {
				std::cout << "Connection status: FAILED" << std::endl;
				map_file_data[CONN_SET] = DISCONNECTED;
				break;
			}
			currentVideoFrame = currentVideoFrame(cv::Rect(340, 0, 1150, 1080));
			/*cv::imshow("image", currentVideoFrame);
			cv::waitKey(30);*/
			markerIds.clear();
			cv::aruco::detectMarkers(currentVideoFrame, dictionary, markerCorners, markerIds, parameters, rejectedCandidates);
			if (markerCorners.empty() == 0) {
				if (!markerIds.empty()) {
					std::cout << "non empty" << std::endl;
					for (int i = 0; i < 3; i++) {
						arucoCorner[i].x = markerCorners[0][i].x - 0.105 * (markerCorners[0][i].x - 575);
						arucoCorner[i].y = markerCorners[0][i].y - 0.105 * (markerCorners[0][i].y - 540);
						//std::cout << "[ " << arucoCorner[i].x << " ; " << arucoCorner[i].y << " ]" << std::endl;
					}
				}
				else {
					for (int i = 0; i < 3; i++) {
						arucoCorner[i].x = 1;
						arucoCorner[i].y = 1;
					}
				}
				float dist = sqrt(pow(arucoCorner[0].x - arucoCorner[1].x, 2) + pow(arucoCorner[0].y - arucoCorner[1].y, 2));
				transfer.prevGlobalCartesian.x = transfer.currGlobalCartesian.x;
				transfer.prevGlobalCartesian.y = transfer.currGlobalCartesian.y;
				transfer.currGlobalCartesian.x = (arucoCorner[0].x + arucoCorner[2].x) / 2;
				transfer.currGlobalCartesian.y = (arucoCorner[0].y + arucoCorner[2].y) / 2;
				transfer.prevAngle = transfer.currAngle;
				transfer.Angle(&transfer, arucoCorner);
				if (abs(transfer.currAngle - transfer.prevAngle) > 160) {
					if (transfer.currAngle > transfer.prevAngle) {
						transfer.deltaAngle = -1 * (360 - transfer.currAngle + transfer.prevAngle);
					}
					else {
						transfer.deltaAngle = 360 - transfer.prevAngle + transfer.currAngle;
					}
				}
				else {
					transfer.deltaAngle = transfer.currAngle - transfer.prevAngle;
				}
				transfer.DeltaEigen(&transfer);
				// Pushing data to the map file
				map_file_data[DATA] = transfer.currGlobalCartesian.x;
				map_file_data[DATA + 1] = transfer.currGlobalCartesian.y;
				map_file_data[DATA + 2] = transfer.currAngle;
				map_file_data[DATA + 3] += transfer.deltaEigenCartesian.x;
				map_file_data[DATA + 4] += transfer.deltaEigenCartesian.y;
				map_file_data[DATA + 5] += transfer.deltaAngle;
			}
			else {
				for (int i = 0; i < 6; i++) map_file_data[DATA + i] = -1;
			}
		}
		map_file_data[CONN_SET] = DISCONNECTED;
		std::cout << "DISCONNECTED" << std::endl;
	}
	return 0;
}