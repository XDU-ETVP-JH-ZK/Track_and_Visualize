#include "Launch.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/types_c.h>
#include <corecrt_io.h>
#include <atlstr.h> 
#include <Windows.h>
using namespace std;
using namespace cv;


void getFilesName(string& File_Directory, string& FileType, vector<string>& FilesName)
{
	string buffer = File_Directory + "\\*" + FileType;

	_finddata_t c_file;   // 存放文件名的结构体

	intptr_t hFile;
	hFile = _findfirst(buffer.c_str(), &c_file);   //找第一个文件命
	if (hFile == -1L)   // 检查文件夹目录下存在需要查找的文件
	{
		cout << "所选目录下不存在" << FileType << "文件" << endl;
		ofstream no("./data/no.txt");
		no << "所选目录不存在" << FileType << "文件" << endl;
		return;
	}
	else
	{
		string fullFilePath;
		do
		{
			fullFilePath.clear();

			//名字
			fullFilePath = File_Directory + "\\" + c_file.name;
			//cout << fullFilePath << endl;
			FilesName.push_back(fullFilePath);

		} while (_findnext(hFile, &c_file) == 0);  //如果找到下个文件的名字成功的话就返回0,否则返回-1  
		_findclose(hFile);
	}
}



void m_calibration(vector<string>& FilesName, Size board_size, Size square_size, Mat& cameraMatrix, Mat& distCoeffs, vector<Mat>& rvecsMat, vector<Mat>& tvecsMat)
{
	int point_count = board_size.height * board_size.width;
	//cout << "开始提取角点………………" << endl;
	int image_count = 0;                                            // 图像数量 
	Size image_size;                                                // 图像的尺寸 

	vector<Point2f> image_points;                                   // 缓存每幅图像上检测到的角点
	vector<vector<Point2f>> image_points_seq;                       // 保存检测到的所有角点

	for (int i = 0; i < FilesName.size(); i++)
	{
		image_count++;

		// 用于观察检验输出
		cout << i+1 << " image_count = " << image_count << endl;
		Mat imageInput = imread(FilesName[i]);
		if (image_count == 1)  //读入第一张图片时获取图像宽高信息
		{
			image_size.width = imageInput.cols;
			image_size.height = imageInput.rows;
		}
		else if (imageInput.cols != image_size.width || imageInput.rows != image_size.height)
		{
			ofstream diff("./data/diff.txt");
			diff << "存在不同分辨率的图片" << endl;
			return;
		}

		/* 提取角点 */
		//CV_CALIB_CB_ADAPTIVE_THRESH | CV_CALIB_CB_NORMALIZE_IMAGE
		bool ok = findChessboardCorners(imageInput, board_size, image_points);
		if (0 == ok)
		{
			ofstream ofs("./data/error.txt", ios::app);
			ofs << FilesName[i] << endl; //找不到角点
			image_count--;
		}
		else
		{
			Mat view_gray;
			//cout << "imageInput.channels()=" << imageInput.channels() << endl;
			cvtColor(imageInput, view_gray, CV_RGB2GRAY);

			/* 亚像素精确化 */
			find4QuadCornerSubpix(view_gray, image_points, Size(5, 5)); //对粗提取的角点进行精确化
			//cv::cornerSubPix(view_gray, image_points, cv::Size(11, 11), cv::Size(-1, -1), cv::TermCriteria(CV_TERMCRIT_ITER + CV_TERMCRIT_EPS, 30, 0.001));

			image_points_seq.push_back(image_points);  //保存亚像素角点

			/* 在图像上显示角点位置 */
			drawChessboardCorners(imageInput, board_size, image_points, true);
			//drawChessboardCorners(imageInput, board_size, image_points, false);

			if (image_count < 10){
				string str = "./data/dispicture00" + to_string(image_count) + ".jpg";
				imwrite(str, imageInput);
			}
			else if(image_count >= 10){
				string str = "./data/dispicture0" + to_string(image_count) + ".jpg";
				imwrite(str, imageInput);
			}
		}
	}
	cout << image_count << endl;
	int rest = FilesName.size() - image_count;
	//if (rest > 0) {
	//	ofstream ofs("./data/error.txt", ios::app);
	//	ofs << "一共" << rest << "张图片" << endl;
	//}
	if (image_count == 0) {
		ofstream af("./data/all.txt");
		af << "所有图片提取角点失败，请检查是否输入了正确的靶标大小" << endl;
		remove("./data/error.txt");
		return;
	}

	/*棋盘三维信息*/
	vector<vector<Point3f>> object_points_seq;                     // 保存标定板上角点的三维坐标
	int i, j, k;
	for (int t = 0; t < image_count; t++)
	{
		vector<cv::Point3f> object_points;
		for (int i = 0; i < board_size.height; i++)
		{
			for (int j = 0; j < board_size.width; j++)
			{
				cv::Point3f realPoint;
				/* 假设标定板放在世界坐标系中z=0的平面上 */
				realPoint.x = i * square_size.width;
				realPoint.y = j * square_size.height;
				realPoint.z = 0;
				object_points.push_back(realPoint);
			}
		}
		object_points_seq.push_back(object_points);
	}
	calibrateCamera(object_points_seq, image_points_seq, image_size, cameraMatrix, distCoeffs, rvecsMat, tvecsMat, 0);

	double m[3][3];
	for (size_t i = 0; i < cameraMatrix.rows; i++)
	{
		double* data = cameraMatrix.ptr<double>(i);
		for (size_t j = 0; j < cameraMatrix.cols; j++)
			m[i][j] = double(data[j]);
	}
	double ans[5];
	for (size_t i = 0; i < distCoeffs.rows; i++)
	{
		double* data = distCoeffs.ptr<double>(i);
		for (size_t j = 0; j < distCoeffs.cols; j++)
			ans[j] = double(data[j]);
	}

	ofstream kout("./data/K.txt");		// 保存内参矩阵的文件 	
	ofstream dout("./data/distCoeffs.txt");		// 保存内参矩阵的文件 	
	ofstream eout("./data/total_err.txt");		//保存平均误差
	kout << setprecision(16);	
	dout << setprecision(16);
	eout << setprecision(8);

	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			kout << m[i][j] << " ";
	for (int i = 0; i < 5; i++)
		dout << ans[i] << " ";

	double total_err = 0.0;    // 总误差
	double err = 0.0;    // 单张图片误差
	vector<Point2f> img_point;    // 重新计算得到的投影点
	for (int i = 0; i < image_count; i++)
	{
		vector<Point3f> tempPointSet = object_points_seq[i];
		/* 通过得到的摄像机内外参数，对空间的三维点进行重新投影计算，得到新的投影点 */
		projectPoints(tempPointSet, rvecsMat[i], tvecsMat[i], cameraMatrix, distCoeffs, img_point);
		/* 计算新的投影点和旧的投影点之间的误差*/
		vector<Point2f> tempImagePoint = image_points_seq[i];
		Mat tempImagePointMat = Mat(1, tempImagePoint.size(), CV_32FC2);
		Mat image_points2Mat = Mat(1, img_point.size(), CV_32FC2);

		for (int j = 0; j < tempImagePoint.size(); j++)
		{
			image_points2Mat.at<Vec2f>(0, j) = Vec2f(img_point[j].x, img_point[j].y);
			tempImagePointMat.at<Vec2f>(0, j) = Vec2f(tempImagePoint[j].x, tempImagePoint[j].y);
		}
		err = norm(image_points2Mat, tempImagePointMat, NORM_L2);
		total_err += err /= point_count;
	}
	cout<<"总体平均误差"<<total_err/image_count<<endl;
	eout<< total_err / image_count << endl;
}


void calib(string path, int x_num, int y_num, int d)
{
	CString datapath = "data";
	CreateDirectory(datapath, NULL);

	string File_Directory = path;
	string FileType = ".jpg";    // 需要查找的文件类型
	string FileType2 = ".png";
	string FileType3 = ".bmp";
	vector<string>FilesName;
	getFilesName(File_Directory, FileType, FilesName);   // 标定所用图像文件的路径
	if (_access("./data/no.txt", 0) != -1) 
	{
		remove("./data/no.txt");
		getFilesName(File_Directory, FileType2, FilesName);
	}
	if (_access("./data/no.txt", 0) != -1)
	{
		remove("./data/no.txt");
		getFilesName(File_Directory, FileType3, FilesName);
	}
		
	Size board_size = Size(x_num, y_num);
	Size square_size = Size(d, d);                       // 实际测量得到的标定板上每个棋盘格的物理尺寸，单位mm

	Mat cameraMatrix = Mat(3, 3, CV_32FC1, Scalar::all(0));        // 摄像机内参数矩阵
	Mat distCoeffs = Mat(1, 5, CV_32FC1, Scalar::all(0));          // 摄像机的5个畸变系数：k1,k2,p1,p2,k3
	vector<Mat> rvecsMat;                                          // 存放所有图像的旋转向量，每一副图像的旋转向量为一个mat
	vector<Mat> tvecsMat;                                          // 存放所有图像的平移向量，每一副图像的平移向量为一个mat

	m_calibration(FilesName, board_size, square_size, cameraMatrix, distCoeffs, rvecsMat, tvecsMat);

}