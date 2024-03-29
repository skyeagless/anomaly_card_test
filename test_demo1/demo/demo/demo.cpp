// demo.cpp: 定义控制台应用程序的入口点。
//在 OpenCV 中，RGB 图像的通道顺序为 BGR 

#include "stdafx.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>


using namespace cv;
using namespace std;

int main()
{
	/**********图片基础展示*****************/
	Mat samp_im = imread("moban.jpg");
	Mat im = imread("shibie.jpg");
	imshow("模版图片", samp_im);
	imshow("原图RGB", im);
	Mat im_hsv;
	cvtColor(im, im_hsv, COLOR_BGR2HSV);
	imshow("原图HSV", im_hsv);
	Mat imgray;
	cvtColor(im, imgray, COLOR_BGR2GRAY);
	imshow("灰度图", imgray);
	Mat imbinary;
	threshold(imgray, imbinary, 128, 255, THRESH_BINARY);
	imshow("二值图", imbinary);
	Mat edge;
	Canny(imbinary, edge, 50, 250);
	imshow("边缘Canny算子", edge);

	/***********************定位证件（没有什么问题）*********************************/
	/*形态学开闭运算*/
	Mat bg1, bg2;
	Mat strel1 = getStructuringElement(MORPH_RECT, Size(20, 10)); //定义核
	Mat strel2 = getStructuringElement(MORPH_RECT, Size(25, 10)); 
	Mat strel3 = getStructuringElement(MORPH_RECT, Size(10, 5));
	morphologyEx(imbinary,bg1,MORPH_CLOSE,strel1);
	morphologyEx(bg1,bg1, MORPH_CLOSE, strel2);
	morphologyEx(bg1,bg2, MORPH_OPEN, strel3);
	imshow("图像闭运算", bg1);
	imshow("图像开运算", bg2);
	/*连通域统计与筛选,对二值图像进行区域提取，并计算区域特征参数。进行区域特征参数比较，提取区域*/
	//首先进行连通域分析,stats分别对应各个轮廓的x,y,width,height和面积,centroids则对应的是中心点,label则对应于表示是当前像素是第几个轮廓
	Mat labels, stats, centroids;
	int i, nccomps = connectedComponentsWithStats(
		bg2, labels,
		stats, centroids
	);//nccomps为总的连通域数量
	cout << "Total Connected Components Detected: " << nccomps << endl;
	vector<Vec3b> colors(nccomps + 1);
	colors[0] = Vec3b(0, 0, 0); //初始化颜色表
	for (i = 1; i < nccomps; i++) {
		colors[i] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
		if (stats.at<int>(i, CC_STAT_AREA) < 200)
			colors[i] = Vec3b(0, 0, 0); // 小于200的区域就置为黑色
	}
	Mat RGB_image; //连通域标记图
	RGB_image = Mat::zeros(im.size(), CV_8UC3);
	for (int y = 0; y < RGB_image.rows; y++) {
		for (int x = 0; x < RGB_image.cols; x++){
			int label = labels.at<int>(y, x);
			CV_Assert(0 <= label && label <= nccomps);
			RGB_image.at<Vec3b>(y, x) = colors[label];
		}
	}
	imshow("连通域标记图1", RGB_image);

	//筛选最大连通域坐标
	int maxIndex = 0;
	for (i = 1; i < nccomps; i++) {
		if (stats.at<int>(maxIndex, CC_STAT_AREA) < stats.at<int>(i, CC_STAT_AREA))
			maxIndex = i;
	}
	Mat im_copy;
	im.copyTo(im_copy);
	//提取最大连通域
	int x = stats.at<int>(maxIndex, CC_STAT_LEFT);
	int y = stats.at<int>(maxIndex, CC_STAT_TOP);
	int width = stats.at<int>(maxIndex, CC_STAT_WIDTH);
	int height = stats.at<int>(maxIndex, CC_STAT_HEIGHT); 
	rectangle(im_copy, Rect(x, y, width, height), Scalar(0, 255, 255), 2, 8, 0);
	Point origin;
	origin.x = x;
	origin.y = y + 20 + height;
	putText(im_copy, "Target", origin, FONT_HERSHEY_PLAIN, 1, Scalar(255, 255, 0), 2);
	imshow("目标提取图", im_copy);
	//裁切图
	Mat imd = im(Rect(x, y, width, height));
	Mat hsvd = im_hsv(Rect(x, y, width, height));
	Mat bgd;
	cvtColor(imd, bgd, COLOR_BGR2GRAY);
	threshold(bgd, bgd, 128, 255, THRESH_BINARY);
	imshow("截图RGB", imd);
	imshow("截图HSV", hsvd);
	imshow("截图二值", bgd);
	//分通道（RGB）显示，仅以B分量为例，其他复制小修即可
	Mat imd_r, imd_g, imd_b;
	vector<Mat> bluechannel, greenchannel, redchannel;
	split(im, bluechannel);
	split(im, greenchannel);
	split(im, redchannel);
	bluechannel[1] = Scalar(0);
	bluechannel[2] = Scalar(0);
	merge(bluechannel, imd_b);
	imshow("blue", imd_b);
	/*细定位*/
	int x1 = round(bgd.size().width / 3);
	int x2 = round(bgd.size().width * 2 / 3);
	int y1, y2;

	//找到第一个对应x的y点=1，以便获得卡片的夹角
	for (y1 = 0; y1 < bgd.size().height; y1++) {
		if (int(bgd.at<uchar>(y1,x1)) == 255)
			break;
	}
	for (y2 = 0; y2 < bgd.size().height; y2++) {
		if (int(bgd.at<uchar>(y2,x2)) == 255)
			break;
	}
    
	double angle = atan2((y2 - y1), (x2 - x1))* 180 / 3.1415926;
	int len = max(imd.cols, imd.rows);
	Point2f center(len / 2., len / 2.);
	Mat rot_mat = getRotationMatrix2D(center, angle, 1.0);
	Mat imd_rot;
	warpAffine(imd, imd_rot, rot_mat, Size(len, len));
	
	Mat imd_rc = imd_rot(Rect(width*0.02, height*0.02, width*(0.985 - 0.02), height*(0.985 - 0.02)));
	imshow("旋转图像", imd_rot);
	imshow("细定位截图", imd_rc);

	/**************************细定位非文本分割（连通域标记图需要调整）*********************************/
	resize(imd_rc, imd_rc,Size(1300, 800));
	Mat imgray_cut;
	cvtColor(imd_rc,imgray_cut, COLOR_BGR2GRAY);
	imshow("截取后的灰度图", imgray_cut);
	Mat bw_en;//就只采用Ostu阈值了
	threshold(imgray_cut, bw_en, 0, 255, THRESH_BINARY_INV + THRESH_OTSU);
	imshow("截取后的反向二值图", bw_en);

	//重复部分--形态学&连通域统计
	Mat bg3, bg4;
	Mat strel4 = getStructuringElement(MORPH_RECT, Size(10, 20)); //定义核
	Mat strel5 = getStructuringElement(MORPH_RECT, Size(10, 20));
	Mat strel6 = getStructuringElement(MORPH_RECT, Size(5, 10));
	morphologyEx(bw_en, bg3, MORPH_CLOSE, strel4);
	morphologyEx(bg3, bg3, MORPH_CLOSE, strel5);
	morphologyEx(bg3, bg4, MORPH_OPEN, strel6);
	imshow("截取图像闭运算", bg3);
	imshow("截取图像开运算", bg4);
	Mat labels_cut, stats_cut, centroids_cut;
	int nccomps_cut = connectedComponentsWithStats(
		bg4, labels_cut,
		stats_cut, centroids_cut
	);
	cout << "Total Connected Components Detected in cut_images: " << nccomps_cut << endl;
	vector<Vec3b> colors_cut(nccomps_cut + 1);
	colors_cut[0] = Vec3b(0, 0, 0); //初始化颜色表
	for (i = 1; i < nccomps_cut; i++) {
		colors_cut[i] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
		//if (stats_cut.at<int>(i, CC_STAT_AREA) < 500)
		//	colors_cut[i] = Vec3b(0, 0, 0); // 小于200的区域就置为黑色
	}
	Mat RGB_image_cut; //连通域标记图
	RGB_image_cut = Mat::zeros(bw_en.size(), CV_8UC3);
	for (int y = 0; y < RGB_image_cut.rows; y++) {
		for (int x = 0; x < RGB_image_cut.cols; x++) {
			int label_cut = labels_cut.at<int>(y, x);
			CV_Assert(0 <= label_cut && label_cut <= nccomps_cut);
			RGB_image_cut.at<Vec3b>(y, x) = colors_cut[label_cut];
		}
	}
	imshow("截取图片的连通域标记图", RGB_image_cut);
	/*重复部分结束*/

	/*头像信息筛选，第二大的连通域*/
	int cardIndex = 1;
	int txIndex = 1;
	for (i = 1; i < nccomps_cut; i++) {
		if (stats_cut.at<int>(cardIndex, CC_STAT_AREA) < stats_cut.at<int>(i, CC_STAT_AREA))
			cardIndex = i;
	}
	for (i = 1; i < nccomps_cut; i++) {
		if (stats_cut.at<int>(txIndex, CC_STAT_AREA) < stats_cut.at<int>(i, CC_STAT_AREA) && i!=cardIndex)
			txIndex = i;
	}
	Mat imd_rc_copy;
	imd_rc.copyTo(imd_rc_copy);
	//提取连通域
	x = stats_cut.at<int>(txIndex, CC_STAT_LEFT);
	y = stats_cut.at<int>(txIndex, CC_STAT_TOP);
	width = stats_cut.at<int>(txIndex, CC_STAT_WIDTH);
	height = stats_cut.at<int>(txIndex, CC_STAT_HEIGHT);
	rectangle(imd_rc_copy, Rect(x, y, width, height), Scalar(0, 255, 255), 2, 8, 0);
	origin.x = x;
	origin.y = y + 20 + height;
	putText(imd_rc_copy, "TouXiang", origin, FONT_HERSHEY_PLAIN, 1, Scalar(255, 255, 0), 2);
	imshow("截图头像提取图", imd_rc_copy);

	/*****************************************************************************************************************/
	/*模版定位（基本没有问题）*/
	/*****************************************************************************************************************/
	resize(samp_im, samp_im, Size(1300, 800));
	cvtColor(samp_im, imgray, COLOR_BGR2GRAY);
	Mat imbw;
	threshold(imgray, imbw, 0, 255, THRESH_BINARY_INV + THRESH_OTSU);
	imshow("黑白模板", imbw);

	Mat bg5, bg6;
	Mat strel7 = getStructuringElement(MORPH_RECT, Size(20, 30)); //定义核
	Mat strel8 = getStructuringElement(MORPH_RECT, Size(10, 30));
	Mat strel9 = getStructuringElement(MORPH_RECT, Size(5, 10));
	morphologyEx(imbw, bg5, MORPH_CLOSE, strel7);
	morphologyEx(bg5, bg5, MORPH_CLOSE, strel8);
	morphologyEx(bg5, bg6, MORPH_OPEN, strel9);
	imshow("图像闭运算", bg5);
	imshow("图像开运算", bg6);
	/*连通域统计与筛选,对二值图像进行区域提取，并计算区域特征参数。进行区域特征参数比较，提取区域*/
	//首先进行连通域分析,stats分别对应各个轮廓的x,y,width,height和面积,centroids则对应的是中心点,label则对应于表示是当前像素是第几个轮廓
	Mat labels_m, stats_m, centroids_m;
	int nccomps_m = connectedComponentsWithStats(
		bg6, labels_m,
		stats_m, centroids_m
	);//nccomps为总的连通域数量
	cout << "Total Connected Components Detected in model image: " << nccomps << endl;
	vector<Vec3b> colors_m(nccomps_m + 1);
	colors_m[0] = Vec3b(0, 0, 0); //初始化颜色表
	for (i = 1; i < nccomps_m; i++) {
		colors_m[i] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
		if (stats_m.at<int>(i, CC_STAT_AREA) < 200)
			colors_m[i] = Vec3b(0, 0, 0); // 小于200的区域就置为黑色
	}
	Mat RGB_image_m; //连通域标记图
	RGB_image_m = Mat::zeros(samp_im.size(), CV_8UC3);
	for (int y = 0; y < RGB_image_m.rows; y++) {
		for (int x = 0; x < RGB_image_m.cols; x++) {
			int label_m = labels_m.at<int>(y, x);
			CV_Assert(0 <= label_m && label_m <= nccomps_m);
			RGB_image_m.at<Vec3b>(y, x) = colors_m[label_m];
		}
	}
	imshow("连通域标记图", RGB_image_m);

	//筛选头像
	int maxIndex_m = 1;
	for (i = 1; i < nccomps_m; i++) {
		if (stats_m.at<int>(maxIndex_m, CC_STAT_AREA) < stats_m.at<int>(i, CC_STAT_AREA))
			maxIndex_m = i;
	}
	Mat samp_copy;
	samp_im.copyTo(samp_copy);
	x = stats_m.at<int>(maxIndex_m, CC_STAT_LEFT);
	y = stats_m.at<int>(maxIndex_m, CC_STAT_TOP);
	width = stats_m.at<int>(maxIndex_m, CC_STAT_WIDTH);
	height = stats_m.at<int>(maxIndex_m, CC_STAT_HEIGHT);
	rectangle(samp_copy, Rect(x, y, width, height), Scalar(0, 255, 255), 2, 8, 0);
	origin.x = x;
	origin.y = y + 20 + height;
	putText(samp_copy, "Touxiang", origin, FONT_HERSHEY_PLAIN, 1, Scalar(255, 255, 0), 2);

	//筛选其他连通域(就是那些文字，非maxIndex就是文字，当然小于1000的干扰信息也要排除）
	vector<int> validIndex_m;
	for (i = 1; i < nccomps_m; i++) {
		if (stats_m.at<int>(i, CC_STAT_AREA) < 1000 || i == maxIndex_m)
			continue;
		validIndex_m.push_back(i);
	}
	for (i = 0; i < validIndex_m.size(); i++) {
		x = stats_m.at<int>(validIndex_m[i], CC_STAT_LEFT);
		y = stats_m.at<int>(validIndex_m[i], CC_STAT_TOP);
		width = stats_m.at<int>(validIndex_m[i], CC_STAT_WIDTH);
		height = stats_m.at<int>(validIndex_m[i], CC_STAT_HEIGHT);
		rectangle(samp_copy, Rect(x, y, width, height), Scalar(0, 255, 255), 2, 8, 0);
		origin.x = x;
		origin.y = y + 20 + height;
		putText(samp_copy, to_string(i), origin, FONT_HERSHEY_PLAIN, 1, Scalar(255, 255, 0), 2);
	}
	imshow("模版定位", samp_copy);
	
	/**************************判断文本信息是否正常*******************************************/
	//姓名，社会保障号码，性别，民族，卡号，有效期，下方号码
	vector<bool> isChanged = { 1,0,0,1,0,1,0 };
	vector<int> bad_index;
	vector<int> good_index;
	int diffXY,diffwidth;
	string message = "文本检测:正常";
	int minindex;
	//模版文本的信息在stats_m 的 validIndex_m里面，测试文本的信息在stat_cut里的txIndex里，比较两者的diffXY值，最小值即为相应的索引，若果此时minvalue仍很大的话就可能有问题，记录bad_index
	for (i = 0; i < validIndex_m.size(); i++) {
		//内循环找到最小的index
		int mindiffXY = 1000000000;
		for (int j = 1; j < nccomps_cut; j++) {
			diffXY = abs(stats_m.at<int>(validIndex_m[i], CC_STAT_LEFT) - stats_cut.at<int>(j, CC_STAT_LEFT)) +
				abs(stats_m.at<int>(validIndex_m[i], CC_STAT_TOP) - stats_cut.at<int>(j, CC_STAT_TOP));
			if (diffXY < mindiffXY) {
				mindiffXY = diffXY;
				minindex = j;
			}
		}
		//外循环判断故障
		diffwidth = abs(stats_m.at<int>(validIndex_m[i], CC_STAT_WIDTH) - stats_cut.at<int>(minindex, CC_STAT_WIDTH));
		if (mindiffXY > 50) {
			message = "文本检测:打印缺漏，或打印位置不准确";
			bad_index.push_back(minindex);
		}
		else if (diffwidth > 40 && isChanged[i] == 0) {
			message = "文本检测:有文本缺字";
			bad_index.push_back(minindex);
		}
		else {
			good_index.push_back(minindex);
		}
	}

	/*******************************图像故障判别****************************************/
	//模版头像的信息在stats_m 的 maxIndex_m里面，测试头像的信息在stat_cut的txIndex里面，头像只有一个，因此不需要外循环
	string message_im = "图像检测：正常";
	int diffXY_im, diffwidth_im;
	int mindiffXY_im = 1000000000;
	diffXY_im = abs(stats_m.at<int>(maxIndex_m, CC_STAT_LEFT) - stats_cut.at<int>(txIndex, CC_STAT_LEFT)) +
		abs(stats_m.at<int>(maxIndex_m, CC_STAT_TOP) - stats_cut.at<int>(txIndex, CC_STAT_TOP));
	diffwidth_im = abs(stats_m.at<int>(maxIndex_m, CC_STAT_WIDTH) - stats_cut.at<int>(txIndex, CC_STAT_WIDTH)) +
		abs(stats_m.at<int>(maxIndex_m, CC_STAT_HEIGHT) - stats_cut.at<int>(txIndex, CC_STAT_HEIGHT));
	if (diffXY_im > 200)
		message_im = "图像检测：打印全部缺漏，或打印位置不正确";
	else if (diffwidth_im > 100)
		message_im = "图像检测：打印缺漏";
	/*注意看本cpp最底部的代码注释，如果需要的话可以参考*/

	/*显示结果*/
	Mat anno_im;
	imd_rc.copyTo(anno_im);
	for (i = 1; i < nccomps_cut; i++) {
		x = stats_cut.at<int>(i, CC_STAT_LEFT);
		y = stats_cut.at<int>(i, CC_STAT_TOP);
		width = stats_cut.at<int>(i, CC_STAT_WIDTH);
		height = stats_cut.at<int>(i, CC_STAT_HEIGHT);
		if (find(good_index.begin(), good_index.end(), i) != good_index.end()) {
			rectangle(anno_im, Rect(x, y, width, height), Scalar(0, 255, 0), 2, 8, 0);
			origin.x = x;
			origin.y = y + 20 + height;
			putText(anno_im, to_string(i), origin, FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0), 2);
		}
		else if (find(bad_index.begin(), bad_index.end(), i) != bad_index.end()) {
			rectangle(anno_im, Rect(x, y, width, height), Scalar(0, 0, 255), 2, 8, 0);
			origin.x = x;
			origin.y = y + 20 + height;
			putText(anno_im, to_string(i), origin, FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 255), 2);
		}
		else if (i == txIndex) {
			rectangle(anno_im, Rect(x, y, width, height), Scalar(0, 255, 255), 2, 8, 0);
			origin.x = x;
			origin.y = y + 20 + height;
			putText(anno_im,"Touxiang", origin, FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 255), 2);
		}
		else {
			rectangle(anno_im, Rect(x, y, width, height), Scalar(255, 0, 255), 2, 8, 0);
			origin.x = x;
			origin.y = y + 20 + height;
			putText(anno_im, to_string(i), origin, FONT_HERSHEY_PLAIN, 1, Scalar(255, 0, 255), 2);
		}

	}

	imshow("666", anno_im);
	cout << message << "||" << message_im;



	waitKey();
    return 0;
}




/*头像，芯片等非文字信息筛选
	//筛选合理连通域,通过面积和宽高比来进行筛选，筛选出了头像，芯片和右上角的银联框
	vector<int> validIndex;
	for (i = 1; i < nccomps_cut; i++) {
		if (stats_cut.at<int>(i, CC_STAT_AREA) > 20000 &&
			stats_cut.at<int>(i, CC_STAT_WIDTH) / stats_cut.at<int>(i, CC_STAT_HEIGHT) > 0.2 &&
			stats_cut.at<int>(i, CC_STAT_WIDTH) / stats_cut.at<int>(i, CC_STAT_HEIGHT) < 1.5)
			validIndex.push_back(i);
	}
	Mat imd_rc_copy;
	imd_rc.copyTo(imd_rc_copy);
	for (i = 0; i < validIndex.size(); i++) {
		x = stats_cut.at<int>(validIndex[i], CC_STAT_LEFT);
		y = stats_cut.at<int>(validIndex[i], CC_STAT_TOP);
		width = stats_cut.at<int>(validIndex[i], CC_STAT_WIDTH);
		height = stats_cut.at<int>(validIndex[i], CC_STAT_HEIGHT);
		rectangle(imd_rc_copy, Rect(x, y, width, height), Scalar(0, 255, 255), 2, 8, 0);
		origin.x = x;
		origin.y = y + 20 + height;
	}
	//putText(imd_rc_copy, "Target", origin, FONT_HERSHEY_PLAIN, 1, Scalar(255, 255, 0), 2);
	imshow("非文本截图目标提取图", imd_rc_copy);*/


	/*
	白点判断(由于原模板头像尺寸与信用卡上的头像大小不同，因此感觉只能另外用一个工程来做工作牌，那个尺寸基本相同，
	才能直接使用像素相减的方法判断白点)
	x = stats_cut.at<int>(txIndex, CC_STAT_LEFT);
	y = stats_cut.at<int>(txIndex, CC_STAT_TOP);
	width = stats_cut.at<int>(txIndex, CC_STAT_WIDTH);
	height = stats_cut.at<int>(txIndex, CC_STAT_HEIGHT);
	Mat touxiang_img = imd_rc(Rect(x, y, width, height)); //目标头像
	x = stats_m.at<int>(maxIndex_m, CC_STAT_LEFT);
	y = stats_m.at<int>(maxIndex_m, CC_STAT_TOP);
	width = stats_m.at<int>(maxIndex_m, CC_STAT_WIDTH);
	height = stats_m.at<int>(maxIndex_m, CC_STAT_HEIGHT);
	Mat touxiang_model = samp_im(Rect(x, y, width, height));//模板头像
	imshow("touxiang_img", touxiang_img);
	imshow("touxiang_model", touxiang_model);
	*/


/*
//通过自己RGB图像差分对比，得到是否缺色结果
代码虽然可以运行，但是有一个问题就是，由于脸的颜色是黄色=红色+绿色，因此B通道可能会很弱，出现连通域的断开
单纯使用最大连通域质心位置进行比较感觉效果可能一般，当然也可能是形态学核尺寸大小的问题
y = stats_cut.at<int>(txIndex, CC_STAT_TOP);
width = stats_cut.at<int>(txIndex, CC_STAT_WIDTH);
height = stats_cut.at<int>(txIndex, CC_STAT_HEIGHT);
Mat touxiang_img = imd_rc(Rect(x, y, width, height));
resize(touxiang_img, touxiang_img, Size(360,280));

Mat touxiang_r, touxiang_g, touxiang_b;
vector<Mat> t_bluechannel, t_greenchannel, t_redchannel;
split(touxiang_img, t_bluechannel);
split(touxiang_img, t_greenchannel);
split(touxiang_img, t_redchannel);

t_bluechannel[1] = Scalar(0);
t_bluechannel[2] = Scalar(0);
merge(t_bluechannel, touxiang_b);
t_greenchannel[0] = Scalar(0);
t_greenchannel[2] = Scalar(0);
merge(t_greenchannel, touxiang_g);
t_redchannel[0] = Scalar(0);
t_redchannel[1] = Scalar(0);
merge(t_redchannel, touxiang_r);

cvtColor(touxiang_b, touxiang_b, COLOR_RGB2GRAY);
cvtColor(touxiang_g, touxiang_g, COLOR_RGB2GRAY);
cvtColor(touxiang_r, touxiang_r, COLOR_RGB2GRAY);

threshold(touxiang_b, touxiang_b, 0, 255, THRESH_BINARY + THRESH_OTSU);
threshold(touxiang_g, touxiang_g, 0, 255, THRESH_BINARY + THRESH_OTSU);
threshold(touxiang_r, touxiang_r, 0, 255, THRESH_BINARY + THRESH_OTSU);

Mat strel_1 = getStructuringElement(MORPH_RECT, Size(10, 20));
Mat strel_2 = getStructuringElement(MORPH_RECT, Size(10, 25));
Mat strel_3 = getStructuringElement(MORPH_RECT, Size(5, 10));

Mat bg_b1, bg_b2;
morphologyEx(touxiang_b, bg_b1, MORPH_CLOSE, strel_1);
morphologyEx(bg_b1, bg_b1, MORPH_CLOSE, strel_2);
morphologyEx(bg_b1, bg_b2, MORPH_OPEN, strel_3);

Mat labels_b, stats_b, centroids_b;
int nccomps_b = connectedComponentsWithStats(
bg_b2, labels_b,
stats_b, centroids_b
);

Mat bg_g1, bg_g2;
morphologyEx(touxiang_g, bg_g1, MORPH_CLOSE, strel_1);
morphologyEx(bg_g1, bg_g1, MORPH_CLOSE, strel_2);
morphologyEx(bg_g1, bg_g2, MORPH_OPEN, strel_3);
Mat labels_g, stats_g, centroids_g;
int nccomps_g = connectedComponentsWithStats(
bg_g2, labels_g,
stats_g, centroids_g
);

Mat bg_r1, bg_r2;
morphologyEx(touxiang_r, bg_r1, MORPH_CLOSE, strel_1);
morphologyEx(bg_r1, bg_r1, MORPH_CLOSE, strel_2);
morphologyEx(bg_r1, bg_r2, MORPH_OPEN, strel_3);
Mat labels_r, stats_r, centroids_r;
int nccomps_r = connectedComponentsWithStats(
bg_r2, labels_r,
stats_r, centroids_r
);

imshow("bg_b2", bg_b2);
imshow("bg_g2", bg_g2);
imshow("bg_r2", bg_r2);
*/