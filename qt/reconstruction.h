#pragma once

#include <Eigen/Dense>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/eigen.hpp>
#include <opencv2/tracking/tracker.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>
#include <iostream>
#include <fstream>
#include <math.h>
#include <string>
#include <vector>

namespace rcs
{
	//initial
	bool CalHomographyMatrix(std::vector<cv::Point2d> imagePoints, std::vector<cv::Point2d> objectPoints, Eigen::Matrix3d &homoMatrixH);
	bool validHCalculation(std::string dstFile, Eigen::Matrix3d homoMatrixH);
	//track
	enum trackerType { KCF, BOOSTING, MIL, TLD, MEDIANFLOW, MOSSE, CSRT };
	enum featureType { SIFT, SURF, ORB };
	enum solveMethod { Zhang, PnP };

	std::string tEnumToString(trackerType tracker);
	std::string fEnumToString(featureType feature);
	
	class myTracker
	{
	public:
		int frameNum;
		std::string log;
		//cv::Mat grayImg, allPointsImg, img1, img2;
		//cv::Mat beforeMatchImg;
		cv::Mat matchImg, frame_0, frame_n;
		myTracker(trackerType track_s, featureType feature_s, solveMethod solve_m);
		bool Track(cv::Mat &img, Eigen::Matrix3d cameraMatrix, cv::Mat distCoeffs, Eigen::Matrix3d homoMatrixH, Eigen::Matrix3d& rMat, Eigen::Vector3d& tVec);
	private:
		int flag;		
		cv::Ptr<cv::Tracker> tracker;
		trackerType track_s;
		featureType feature_s;
		solveMethod solve_m;
		cv::Mat initFrame;
		cv::Rect2d initRect, rect;
		cv::detail::ImageFeatures feature1, feature2;

		bool DetectAndMatchImagePoints(featureType feature_s, cv::Mat imgFixedFrame, cv::Mat imgCurrentFrame, double roiX, double roiY, std::vector<cv::Point2d> &fixedFramePoints, std::vector<cv::Point2d> &currentFramePoints);
		std::vector<cv::Point2d> CalObjPoints(std::vector<cv::Point2d> imagePoints, Eigen::Matrix3d homoMatrixH);
		bool CalExtrinsicMatrixUseZhang(std::vector<cv::Point2d> imagePoints, std::vector<cv::Point2d> objectPoints, Eigen::Matrix3d cameraMatrix, Eigen::Matrix3d &rMat, Eigen::Vector3d &tVec);
		bool CalExtrinsicMatrixUsePnP(std::vector<cv::Point2d> imagePoints, std::vector<cv::Point2d> objectPoints, Eigen::Matrix3d cameraMatrix, cv::Mat distCoeffs, Eigen::Matrix3d &rMat, Eigen::Vector3d &tVec);
	};
}
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
