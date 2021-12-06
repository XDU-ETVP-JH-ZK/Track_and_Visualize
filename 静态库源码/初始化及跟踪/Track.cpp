﻿// StaticLib1.cpp : 定义静态库的函数。
//

#include "pch.h"
#include "reconstruction.h"

using namespace std;

string rcs::tEnumToString(trackerType tracker)
{
	if (tracker == KCF)
		return "KCF";
	if (tracker == BOOSTING)
		return "BOOSTING";
	if (tracker == MIL)
		return "MIL";
	if (tracker == TLD)
		return "TLD";
	if (tracker == MEDIANFLOW)
		return "MEDIANFLOW";
	if (tracker == MOSSE)
		return "MOSSE";
	if (tracker == CSRT)
		return "CSRT";
	else
		return NULL;
}
string rcs::fEnumToString(featureType feature)
{
	if (feature == SIFT)
		return "SIFT";
	if (feature == SURF)
		return "SURF";
	if (feature == ORB)
		return "ORB";
	else
		return NULL;
}

rcs::myTracker::myTracker(trackerType track_s, featureType feature_s, solveMethod solve_m)
{ 

	this->flag = 0;
	this->frameNum = 0;
	this->log = "";
	this->track_s = track_s;
	this->feature_s = feature_s;
	this->solve_m = solve_m;
	this->matchImg = NULL;
}

bool rcs::myTracker::Track(cv::Mat& frame, Eigen::Matrix3d cameraMatrix, cv::Mat distCoeffs, Eigen::Matrix3d homoMatrixH, Eigen::Matrix3d& rMat, Eigen::Vector3d& tVec)
{
	string t = rcs::tEnumToString(track_s);
	string f = rcs::fEnumToString(feature_s);

	vector<cv::Point2d> fixedFramePoints;
	vector<cv::Point2d> currentFramePoints;

	if (flag == 0)
	{
		flag = 1;
		frameNum = 1;
		if (track_s == KCF)
			tracker = cv::TrackerKCF::create();
		if (track_s == BOOSTING)
			tracker = cv::TrackerBoosting::create();
		if (track_s == MIL)
			tracker = cv::TrackerMIL::create();
		if (track_s == TLD)
			tracker = cv::TrackerTLD::create();
		if (track_s == MEDIANFLOW)
			tracker = cv::TrackerMedianFlow::create();

		if (track_s == MOSSE)
			tracker = cv::TrackerMOSSE::create();
		if (track_s == CSRT)
			tracker = cv::TrackerCSRT::create();
		
		rect = cv::selectROI("ROI Selector", frame, false);
		if (rect.empty()) 
		{
			flag = 0;
			rMat.setZero();
			tVec.setZero();
			log = "Cancel selection.";
			return 0;
		}
		initRect = rect;
		initFrame = frame(initRect);
		cv::cvtColor(initFrame, initFrame, cv::COLOR_BGR2GRAY);
		cv::destroyWindow("ROI Selector");
		/* 初始化跟踪器 */
		tracker->init(frame, rect);

		if (DetectAndMatchImagePoints(feature_s, initFrame, initFrame, initRect.x, initRect.y, fixedFramePoints, currentFramePoints))
		{
			vector<cv::Point2d> objPoints = CalObjPoints(fixedFramePoints, homoMatrixH);
			bool calSuccess;
			if (solve_m == Zhang)
				calSuccess = CalExtrinsicMatrixUseZhang(currentFramePoints, objPoints, cameraMatrix, rMat, tVec);
			else if (solve_m == PnP)
				calSuccess = CalExtrinsicMatrixUsePnP(currentFramePoints, objPoints, cameraMatrix, distCoeffs, rMat, tVec);
			if (calSuccess)
			{
				log = "succeed";
				return 1;
			}
		}
		rMat.setZero();
		tVec.setZero();
		return 0;
	}
	else
	{
		frameNum++;
		bool ok = tracker->update(frame, rect);
		
		cv::putText(frame, "Frame: " + to_string(frameNum), cv::Point(20, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 0, 0), 1.5);
		if (ok && rect.x > 0 && rect.y > 0 && rect.x + rect.width <= frame.cols && rect.y + rect.height <= frame.rows)
		{
			cv::rectangle(frame, rect, cv::Scalar(255, 0, 0));
			cv::Mat roi = frame(rect);
			cv::cvtColor(roi, roi, cv::COLOR_BGR2GRAY);
			/* 特征点检测，匹配 */			
			if (DetectAndMatchImagePoints(feature_s, initFrame, roi, rect.x, rect.y, fixedFramePoints, currentFramePoints))
			{
				vector<cv::Point2d> objPoints = CalObjPoints(fixedFramePoints, homoMatrixH);
				bool calSuccess;
				if (solve_m == Zhang)
					calSuccess = CalExtrinsicMatrixUseZhang(currentFramePoints, objPoints, cameraMatrix, rMat, tVec);
				else if (solve_m == PnP)
					calSuccess = CalExtrinsicMatrixUsePnP(currentFramePoints, objPoints, cameraMatrix, distCoeffs, rMat, tVec);
				if (calSuccess)
				{
					log = "succeed";
					return 1;
				}
			}
		}
		else
		{
			log = "error: Frame " + to_string(frameNum) + ", Tracking failure detected.";
		}		
	}
	rMat.setZero();
	tVec.setZero();
	return 0;
}

bool rcs::myTracker::DetectAndMatchImagePoints(featureType feature_s, cv::Mat imgFixedFrame, cv::Mat imgCurrentFrame, double roiX, double roiY, vector<cv::Point2d> &fixedFramePoints, vector<cv::Point2d> &currentFramePoints)
{
	if (feature_s == SIFT)
	{
		cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
		sift->detectAndCompute(imgFixedFrame, cv::noArray(), feature1.keypoints, feature1.descriptors);
		sift->detectAndCompute(imgCurrentFrame, cv::noArray(), feature2.keypoints, feature2.descriptors);
	}
	if (feature_s == SURF)
	{
		cv::Ptr<cv::xfeatures2d::SURF> surf = cv::xfeatures2d::SURF::create();
		surf->detectAndCompute(imgFixedFrame, cv::noArray(), feature1.keypoints, feature1.descriptors);
		surf->detectAndCompute(imgCurrentFrame, cv::noArray(), feature2.keypoints, feature2.descriptors);
	}
	if (feature_s == ORB)
	{
		cv::Ptr<cv::ORB> orb = cv::ORB::create();
		orb->detectAndCompute(imgFixedFrame, cv::noArray(), feature1.keypoints, feature1.descriptors);
		orb->detectAndCompute(imgCurrentFrame, cv::noArray(), feature2.keypoints, feature2.descriptors);
	}
	if (feature1.keypoints.size() < 20 || feature2.keypoints.size() < 20)
	{
		log = "errro: Frame " + to_string(frameNum) + ", Less than 20 key points.";
		return 0;
	}
	cv::Ptr<cv::DescriptorMatcher> matcher;
	if (feature_s == ORB)
		matcher = cv::BFMatcher::create(cv::NORM_HAMMING, true);
	else
		matcher = cv::BFMatcher::create(cv::NORM_L2, true);
	std::vector<cv::DMatch> matches;
	matcher->match(feature1.descriptors, feature2.descriptors, matches);
	/* 按distance升序排序 */
	sort(matches.begin(), matches.end(), [](cv::DMatch x, cv::DMatch y) { return x.distance < y.distance; });
	if (matches.size() < 20)
	{
		log = "errro: Frame " + to_string(frameNum) + ", Less than 20 matched points.";
		return 0;
	}
	std::vector<cv::DMatch> goodMatch;
	for (int i = 0; i < 20; i++)
	{
		goodMatch.push_back(matches[i]);
	}
	std::vector<cv::Point2d> points1;
	std::vector<cv::Point2d> points2;
	for (int i = 0; i < goodMatch.size(); i++)
	{
		points1.push_back(feature1.keypoints[goodMatch[i].queryIdx].pt);
		points2.push_back(feature2.keypoints[goodMatch[i].trainIdx].pt);
	}
	/*筛选前匹配效果*/
	cv::drawMatches(img1, feature1.keypoints, img2, feature2.keypoints, goodMatch, beforeMatchImg, cv::Scalar::all(-1), cv::Scalar::all(-1), vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
	/* 匹配点筛选 */
	cv::Mat H_CV = cv::findHomography(points1, points2, cv::RANSAC, 5.0);
	if (H_CV.empty())
	{
		log = "errro: Frame " + to_string(frameNum) + ", H matrix cannot be estimated(fixedFrame to currentFrame).";
		return 0;
	}
	Eigen::Matrix3d H;
	cv2eigen(H_CV, H);
	Eigen::MatrixXd matrix(3, points1.size());
	for (int i = 0; i < points1.size(); i++)
		matrix.col(i) = Eigen::Vector3d(points1[i].x, points1[i].y, 1);
	matrix = H * matrix;
	/* 重投影点 */
	std::vector<cv::Point2d> reproPoints;
	std::vector<cv::DMatch> goodMatch_1;
	for (int i = 0; i < matrix.cols(); i++)
	{
		reproPoints.push_back(cv::Point2d(matrix.col(i)[0] / matrix.col(i)[2], matrix.col(i)[1] / matrix.col(i)[2]));
		/*筛选阈值 = 1*/
		if (sqrt(pow((reproPoints[i].x - points2[i].x), 2) + pow((reproPoints[i].y - points2[i].y), 2)) < 1)
		{
			fixedFramePoints.push_back(cv::Point2d(points1[i].x += initRect.x, points1[i].y += initRect.y));
			currentFramePoints.push_back(cv::Point2d(points2[i].x += roiX, points2[i].y += roiY));
			goodMatch_1.push_back(goodMatch[i]);
		}
	}
	/* 筛选后匹配点数 >= 8 */
	if (goodMatch_1.size() < 8)
	{
		log = "error: Frame " + to_string(frameNum) + ", Less than 8 good matched points.";
		return 0;
	}

	/*筛选后匹配效果*/
	cv::drawMatches(imgFixedFrame, feature1.keypoints, imgCurrentFrame, feature2.keypoints, goodMatch_1, matchImg, cv::Scalar::all(-1), cv::Scalar::all(-1), vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
	return 1;
}

vector<cv::Point2d> rcs::myTracker::CalObjPoints(vector<cv::Point2d> imagePoints, Eigen::Matrix3d homoMatrixH)
{
	Eigen::MatrixXd matrix(3, imagePoints.size());
	for (int i = 0; i < imagePoints.size(); i++)
		matrix.col(i) = Eigen::Vector3d(imagePoints[i].x, imagePoints[i].y, 1);
	matrix = homoMatrixH.inverse() * matrix;
	vector<cv::Point2d> objPoints;
	for (int i = 0; i < matrix.cols(); i++)
	{
		objPoints.push_back(cv::Point2d(matrix.col(i)[0] / matrix.col(i)[2], matrix.col(i)[1] / matrix.col(i)[2]));
	}
	return objPoints;
}

bool rcs::myTracker::CalExtrinsicMatrixUseZhang(vector<cv::Point2d> imagePoints, vector<cv::Point2d> objectPoints, Eigen::Matrix3d cameraMatrix, Eigen::Matrix3d& rMat, Eigen::Vector3d& tVec)
{
	cv::Mat H_CV = cv::findHomography(objectPoints, imagePoints, cv::RANSAC, 5.0);
	if (H_CV.empty())
	{
		log = "error: Frame " + to_string(frameNum) + ", H matrix cannot be estimated.";
		return 0;
	}
	Eigen::Matrix3d H;
	cv::cv2eigen(H_CV, H);
	Eigen::Vector3d h1 = H.col(0);
	Eigen::Vector3d h2 = H.col(1);
	Eigen::Vector3d h3 = H.col(2);

	Eigen::Matrix3d K_inv = cameraMatrix.inverse();
	double lamda1 = 1 / (K_inv*h1).norm();
	double lamda2 = 1 / (K_inv*h2).norm();

	Eigen::Vector3d R1 = (lamda1 + lamda2) / 2 * K_inv*h1;
	Eigen::Vector3d R2 = (lamda1 + lamda2) / 2 * K_inv*h2;
	Eigen::Vector3d R3 = R1.cross(R2);
	/* 旋转矩阵 */
	rMat.col(0) = R1;
	rMat.col(1) = R2;
	rMat.col(2) = R3;
	/* 旋转矩阵的行列式约束范围 */
	if (0.99 < rMat.determinant() && rMat.determinant() < 1.01)
	{
		/* 平移矢量 */
		tVec = (lamda1 + lamda2) / 2 * K_inv*h3;
		return 1;
	}
	else
	{
		log = "error: Frame " + to_string(frameNum) + ", Invalid R matrix.";
		return 0;
	}
}

bool rcs::myTracker::CalExtrinsicMatrixUsePnP(vector<cv::Point2d> imagePoints, vector<cv::Point2d> objectPoints, Eigen::Matrix3d cameraMatrix, cv::Mat distCoeffs, Eigen::Matrix3d& rMat, Eigen::Vector3d& tVec)
{
	cv::Mat cameraMatrix_mat, rvec, tvec;
	eigen2cv(cameraMatrix, cameraMatrix_mat);
	std::vector<cv::Point3d> objPoints;
	for (cv::Point2d point : objectPoints) 
		objPoints.push_back(cv::Point3d(point.x, point.y, 0));
	if (cv::solvePnP(objPoints, imagePoints, cameraMatrix_mat, distCoeffs, rvec, tvec))
	{
		cv::Rodrigues(rvec, rvec);
		cv::cv2eigen(rvec, rMat);
		cv::cv2eigen(tvec, tVec);
		log = "succeed";
		return 1;
	}
	else
	{
		log = "error: PnP cannot calculate extrinsic matrix.";
		return 0;
	}
	
}

