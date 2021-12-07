#include "pangolin_lib.h"
using namespace std;
using namespace Eigen;

//定义一个全局变量，用于保存生成的位姿
vector<vector<float>> pose_fin;
//给定初始速度和初始状态（运行）
double slam_speed = 1.0;

struct RotationMatrix
{
	Matrix3d matrix = Matrix3d::Identity();
};

ostream& operator << (ostream& out, const RotationMatrix& r)
{
	out.setf(ios::fixed);
	Matrix3d matrix = r.matrix;
	out << '=';
	out << "[" << setprecision(2) << matrix(0, 0) << "," << matrix(0, 1) << "," << matrix(0, 2) << "],"
		<< "[" << matrix(1, 0) << "," << matrix(1, 1) << "," << matrix(1, 2) << "],"
		<< "[" << matrix(2, 0) << "," << matrix(2, 1) << "," << matrix(2, 2) << "]";
	return out;
}

istream& operator >> (istream& in, RotationMatrix& r)
{
	return in;
}

struct TranslationVector
{
	Vector3d trans = Vector3d(0, 0, 0);
};

ostream& operator << (ostream& out, const TranslationVector& t)
{
	out << "=[" << t.trans(0) << ',' << t.trans(1) << ',' << t.trans(2) << "]";
	return out;
}

istream& operator >> (istream& in, TranslationVector& t)
{
	return in;
}

//从groundtruth文件获取位姿信息
int get_pose(string path_to_dataset, vector<vector<float>>& pose, int index);
int get_pose2(string path_to_dataset, vector<vector<float>>& pose, int index);

//生成地图
void get_map();

std::vector<std::string> split(std::string str, std::string pattern)
{
	std::string::size_type pos;
	std::vector<std::string> result;
	str += pattern;//扩展字符串以方便操作
	int size = str.size();
	for (int i = 0; i < size; i++)
	{
		pos = str.find(pattern, i);
		if (pos < size)
		{
			std::string s = str.substr(i, pos - i);
			result.push_back(s);
			i = pos + pattern.size() - 1;
		}
	}
	return result;
}

void pview(string path_to_dataset)
{
	regex number("\\d+");

	smatch result;
	string::const_iterator iterStart = path_to_dataset.begin();
	string::const_iterator iterEnd = path_to_dataset.end();
	string temp;
	while (regex_search(iterStart, iterEnd, result, number))
	{
		temp = result[0];
		iterStart = result[0].second;	//更新搜索起始位置,搜索剩下的字符串
	}

	int index = atoi(temp.c_str());

	string line;
	ifstream f(path_to_dataset);
	getline(f, line);
	vector<string> a = split(line, " ");

	if (a.size() == 12) {
		vector<vector<float>> pose;
		get_pose2(path_to_dataset, pose, index);
		std::thread render2;
		render2 = std::thread(get_map);
		for (int i = 0; i < index; i++) {
			float temp[12]; vector< float>pose_temp;
			pose_temp.push_back(pose[i][3]); pose_temp.push_back(pose[i][4]); pose_temp.push_back(pose[i][5]);
			pose_temp.push_back(pose[i][6]); pose_temp.push_back(pose[i][7]); pose_temp.push_back(pose[i][8]);
			pose_temp.push_back(pose[i][9]); pose_temp.push_back(pose[i][10]); pose_temp.push_back(pose[i][11]);
			pose_temp.push_back(pose[i][0]); pose_temp.push_back(pose[i][1]); pose_temp.push_back(pose[i][2]);
			pose_fin.push_back(pose_temp);
			pose_temp.clear();
			Sleep(100 / slam_speed);
		}
		pose.clear();
		render2.join();
	}

	else {
		//定义暂时读取位姿信息存储的vector，读取总帧数和读取间隔
		vector<vector<float>> pose;
		//取得pose
		get_pose(path_to_dataset, pose, index);
		//定义线程
		std::thread render_loop;
		//开启线程
		render_loop = std::thread(get_map);
		//转换原groundtruth下的数据格式（四元数）到适合pangolin的数据格式（旋转矩阵）
		for (int i = 0; i < index; i++)
		{
			//存储四元数
			Eigen::Quaterniond quaternion(pose[i][3], pose[i][4], pose[i][5], pose[i][6]);
			//存储旋转矩阵
			Eigen::Matrix3d rotation_matrix;
			//四元数转化旋转矩阵
			rotation_matrix = quaternion.matrix();
			//定义一个暂时的pose_temp存储12个位姿数据，9个旋转矩阵的元素，3各位置元素
			vector<float> pose_temp;
			//旋转矩阵元素
			pose_temp.push_back(rotation_matrix(0, 0));	pose_temp.push_back(rotation_matrix(1, 0));	pose_temp.push_back(rotation_matrix(2, 0));
			pose_temp.push_back(rotation_matrix(0, 1));	pose_temp.push_back(rotation_matrix(1, 1));	pose_temp.push_back(rotation_matrix(2, 1));
			pose_temp.push_back(rotation_matrix(0, 2));	pose_temp.push_back(rotation_matrix(1, 2));	pose_temp.push_back(rotation_matrix(2, 2));
			//位置元素
			pose_temp.push_back(pose[i][0]);			pose_temp.push_back(pose[i][1]);				pose_temp.push_back(pose[i][2]);
			//将pose_temp存入全局变量pose用于构图，也就是每一行的pose都是一个pose_temp，12个数，最后会有index行
			pose_fin.push_back(pose_temp);
			//清空pose_temp内存
			pose_temp.clear();
			//暂定
			//利用序列更新时间长短变化改变更新地图速度
			Sleep(100 / slam_speed);
		}
		//清空pose内存
		pose.clear();
		//收束线程
		render_loop.join();
	}
	return;
}

int get_pose(string path_to_dataset, vector<vector<float>>& pose, int index)
{
	//检测文件是否存在
	ifstream fin(path_to_dataset);
	if (!fin)
	{
		cerr << "I cann't find txt!" << endl;
		return 1;
	}
	//循环取值给pose，取帧数量为index
	for (int i = 0; i < index; i++)
	{
		//定义暂时量用于读取操作，定义pose_temp用于向pose添加数据，设定选取间隔为interval
		float temp[8]; vector< float>pose_temp;
		//循环读取文件每行数据，直到满足interval行
		fin >> temp[0] >> temp[1] >> temp[2] >> temp[3] >> temp[4] >> temp[5] >> temp[6];
		//先把7个数给pose_temp，3个位置元素，还有四元数，注意我先加入的temp[7]也就是四元数的实部
		pose_temp.push_back(temp[0]); pose_temp.push_back(temp[1]); pose_temp.push_back(temp[2]);
		pose_temp.push_back(temp[6]); pose_temp.push_back(temp[3]); pose_temp.push_back(temp[4]); pose_temp.push_back(temp[5]);
		//把pose_temp堆入pose
		pose.push_back(pose_temp);
		//清空pose_temp内存
		pose_temp.clear();
	}
	return 1;
}

int get_pose2(string path_to_dataset, vector<vector<float>>& pose, int index)
{
	ifstream fin(path_to_dataset);
	if (!fin)
	{
		cerr << "Can not find the txt file!" << endl;
		return 1;
	}
	for (int i = 0; i < index; i++)
	{
		float temp[12]; vector< float>pose_temp;
		fin >> temp[0] >> temp[1] >> temp[2] >> temp[3] >> temp[4] >> temp[5] >> temp[6] >> temp[7] >> temp[8] >> temp[9] >> temp[10] >> temp[11];
		pose_temp.push_back(temp[0]); pose_temp.push_back(temp[1]); pose_temp.push_back(temp[2]);
		pose_temp.push_back(temp[3]); pose_temp.push_back(temp[4]); pose_temp.push_back(temp[5]);
		pose_temp.push_back(temp[6]); pose_temp.push_back(temp[7]); pose_temp.push_back(temp[8]);
		pose_temp.push_back(temp[9]); pose_temp.push_back(temp[10]); pose_temp.push_back(temp[11]);
		pose.push_back(pose_temp);
		pose_temp.clear();
	}
	return 1;
}

void get_map()
{
	//生成一个gui界面，定义大小
	pangolin::CreateWindowAndBind("Main", 1024, 768);
	//进行深度测试，保证某视角下像素只有一种颜色，不混杂
	glEnable(GL_DEPTH_TEST);

	//放置一个相机-48.9073 -34.1683 130.757
	pangolin::OpenGlRenderState s_cam(
		pangolin::ProjectionMatrix(1024, 768, 420, 420, 900, 640, 0.2, 2000),
		pangolin::ModelViewLookAt(-48.9073, -34.1683, 130.757, 0, 0, 0, pangolin::AxisY)
	);

	//创建视角窗口
	pangolin::Handler3D handler(s_cam);
	pangolin::View& d_cam = pangolin::CreateDisplay()
		.SetBounds(0.0, 1.0, 0.0, 1.0, -640.0f / 480.0f)
		.SetHandler(&handler);

	//设计显示面板
	pangolin::CreatePanel("menu").SetBounds(0.0, 0.3, 0.0, 0.3);
	//第一个参数为按钮的名字，第二个为默认状态，第三个为最低值，第四个为最高值
	pangolin::Var<double> a_int("menu.slam_speed", 1, 0.3, 10);
	//第一个参数为按钮的名字，第二个为默认状态，第三个为是否有选择框
	pangolin::Var<bool> menu("menu.Trajectory", true, true);
	pangolin::Var<bool> menu2("menu.KeyFrame", true, true);
	//pangolin::Var<bool> goon("menu.go_on", true, true);
	//设计文本输出于面板
	string r1, r2, r3, t1;
	pangolin::Var<string> rm1("menu.R", r1);
	pangolin::Var<string> rm2("menu. ", r2);
	pangolin::Var<string> rm3("menu.  ", r3);
	pangolin::Var<string> tt("menu.t", t1);
	// 设置按钮
	//pangolin::Var<bool> save_window("menu.Save_Window", false, false);
	//pangolin::Var<bool> save_cube("menu.Save_Cube", false, false);
	//pangolin::Var<std::function<void(void)> > reset("menu.ESC");

	//初始化
	pangolin::GlTexture imageTexture(640, 480, GL_RGB, false, 0, GL_BGR, GL_UNSIGNED_BYTE);
	while (!pangolin::ShouldQuit())
	{
		//清除颜色缓冲和深度缓冲
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		d_cam.Activate(s_cam);

		//背景先弄成白色的吧，我觉得白色比较好看
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

		//使用变换矩阵画图
		int k = 0;
		for (k = 0; k < pose_fin.size(); k++)
		{
			//使用位置变换矩阵
			glPushMatrix();
			//变换如该矩阵，注意这个变换矩阵是转置的
			std::vector<GLfloat> Twc = { pose_fin[k][0],pose_fin[k][1],pose_fin[k][2],0,
									  pose_fin[k][3],pose_fin[k][4],pose_fin[k][5],0,
									  pose_fin[k][6],pose_fin[k][7],pose_fin[k][8],0,
									  pose_fin[k][9],pose_fin[k][10],pose_fin[k][11],1 };
			//变换
			glMultMatrixf(Twc.data());
			//每次变换后绘制相机
			if (menu2) {
				glLineWidth(2);
				glBegin(GL_LINES);
				glColor3f(1.0f, 0.f, 0.f);
				glVertex3f(0, 0, 0);		glVertex3f(0.1, 0, 0);
				glColor3f(0.f, 1.0f, 0.f);
				glVertex3f(0, 0, 0);		glVertex3f(0, 0.2, 0);
				glColor3f(0.f, 0.f, 1.0f);
				glVertex3f(0, 0, 0);		glVertex3f(0, 0, 0.1);
				glColor3f(0.f, 1.f, 1.f);
				glVertex3f(0, 0, 0);		glVertex3f(0.1, 0.2, 0);
				glVertex3f(0.1, 0, 0);		glVertex3f(0, 0.2, 0);
				glVertex3f(0, 0.2, 0);		glVertex3f(0.1, 0.2, 0);
				glVertex3f(0.1, 0, 0);		glVertex3f(0.1, 0.2, 0);
				glEnd();
			}
			glPopMatrix();
		}

		//运行R，t输出
		if (pose_fin.size() > 0)
		{
			r1 = to_string(pose_fin[k - 1][0]) + " " + to_string(pose_fin[k - 1][1]) + " " + to_string(pose_fin[k - 1][2]);
			r2 = to_string(pose_fin[k - 1][3]) + " " + to_string(pose_fin[k - 1][4]) + " " + to_string(pose_fin[k - 1][5]);
			r3 = to_string(pose_fin[k - 1][6]) + " " + to_string(pose_fin[k - 1][7]) + " " + to_string(pose_fin[k - 1][8]);
			t1 = to_string(pose_fin[k - 1][9]) + " " + to_string(pose_fin[k - 1][10]) + " " + to_string(pose_fin[k - 1][11]);
			rm1 = r1; rm2 = r2; rm3 = r3; tt = t1;
			//RotationMatrix R;
			//for (int i = 0; i < 3; i++)
			//	for (int j = 0; j < 3; j++)
			//		R.matrix(j, i) = double(pose_fin[k - 1][3 * j + i]);
			//rotation_matrix = R;

			//TranslationVector t;
			//t.trans = Vector3d(pose_fin[k - 1][9], pose_fin[k - 1][10], pose_fin[k - 1][11]);
			//t.trans = -R.matrix * t.trans;
			//translation_vector = t;
		}

		//速度变更
		slam_speed = a_int;

		//绘制连接的线，并根据选项决定是否绘制绿线
		if (menu)
		{
			glLineWidth(2);
			glBegin(GL_LINES);
			glColor3f(0.f, 0.f, 0.f);
			for (int i = 0; i < pose_fin.size() - 1; i++)
			{
				glVertex3f(pose_fin[i][9], pose_fin[i][10], pose_fin[i][11]);
				glVertex3f(pose_fin[i + 1][9], pose_fin[i + 1][10], pose_fin[i + 1][11]);
			}
			glEnd();
		}

		//交换帧和并推进事件
		pangolin::FinishFrame();
	}
}