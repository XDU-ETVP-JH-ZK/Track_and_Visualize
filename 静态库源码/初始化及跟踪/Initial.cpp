#include "pch.h"
#include "reconstruction.h"

using namespace std;
using namespace cv;

bool rcs::CalHomographyMatrix(vector<Point2d> imagePoints, vector<Point2d> objectPoints, Eigen::Matrix3d &homoMatrixH) 
{
	cv::Mat H = cv::findHomography(objectPoints, imagePoints, cv::RANSAC, 5.0);
	if (H.empty())
	{
		cout << "errro: H1 matrix cannot be estimated." << endl;
		return 0;
	}
	cv2eigen(H, homoMatrixH);
	return 1;
}

bool rcs::validHCalculation(string dstFile, Eigen::Matrix3d homoMatrixH)
{
	ofstream fout(dstFile);
	if (fout.is_open())
	{
		for (int i = 0; i < homoMatrixH.rows(); i++)
			for (int j = 0; j < homoMatrixH.cols(); j++)
				fout << homoMatrixH(i, j) << " ";
		fout.close();
	}
	else
	{
		cout << "error: File not be opened." << endl;
		return 0;
	}
	return 1;
}