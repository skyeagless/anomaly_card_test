// demo2.cpp: 定义控制台应用程序的入口点。
// 本demo对工作证头像进行判别~

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
	/**********模板工作证和测试工作证展示*****************/
	Mat samp_im = imread("model.jpg");
	Mat im = imread("test.jpg");
	//翻转一下，有利于检测
	transpose(samp_im, samp_im);
	flip(samp_im, samp_im, 0);
	transpose(im, im);
	flip(im, im, 0);
	
	Mat imgray;
	cvtColor(im, imgray, COLOR_BGR2GRAY);
	Mat imbw;
	threshold(imgray, imbw, 0, 255, THRESH_BINARY+THRESH_OTSU);
	Mat bg1, bg2;
	Mat strel1 = getStructuringElement(MORPH_RECT, Size(20, 10)); //定义核
	Mat strel2 = getStructuringElement(MORPH_RECT, Size(25, 10));
	Mat strel3 = getStructuringElement(MORPH_RECT, Size(10, 5));
	morphologyEx(imbw, bg1, MORPH_CLOSE, strel1);
	morphologyEx(bg1, bg1, MORPH_CLOSE, strel2);
	morphologyEx(bg1, bg2, MORPH_OPEN, strel3);
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
		for (int x = 0; x < RGB_image.cols; x++) {
			int label = labels.at<int>(y, x);
			CV_Assert(0 <= label && label <= nccomps);
			RGB_image.at<Vec3b>(y, x) = colors[label];
		}
	}
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
	Mat bgd;
	cvtColor(imd, bgd, COLOR_BGR2GRAY);
	threshold(bgd, bgd, 0, 255, THRESH_BINARY + THRESH_OTSU);

	/*细定位*/
	int x1 = round(bgd.size().width / 3);
	int x2 = round(bgd.size().width * 2 / 3);
	int y1, y2;

	//找到第一个对应x的y点=1，以便获得卡片的夹角
	for (y1 = 0; y1 < bgd.size().height; y1++) {
		if (int(bgd.at<uchar>(y1, x1)) == 255)
			break;
	}
	for (y2 = 0; y2 < bgd.size().height; y2++) {
		if (int(bgd.at<uchar>(y2, x2)) == 255)
			break;
	}

	double angle = atan2((y2 - y1), (x2 - x1)) * 180 / 3.1415926;
	int len = max(imd.cols, imd.rows);
	Point2f center(len / 2., len / 2.);
	Mat rot_mat = getRotationMatrix2D(center, angle, 1.0);
	Mat imd_rot;
	warpAffine(imd, imd_rot, rot_mat, Size(len, len));

	Mat imd_rc = imd_rot(Rect(width*0.02, height*0.02, width*(0.985 - 0.02), height*(0.985 - 0.02)));
	imshow("细定位截图", imd_rc);

	/**************************细定位非文本分割*********************************/
	resize(imd_rc, imd_rc, Size(1300, 800));
	Mat imgray_cut;
	cvtColor(imd_rc, imgray_cut, COLOR_BGR2GRAY);
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
		if (stats_cut.at<int>(i, CC_STAT_AREA) < 200)
			colors_cut[i] = Vec3b(0, 0, 0); // 小于200的区域就置为黑色
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
	imshow("连通域标记图1", RGB_image_cut);

	/*头像信息筛选，第一大的连通域*/
	int txIndex = 1;
	for (i = 1; i < nccomps_cut; i++) {
		if (stats_cut.at<int>(txIndex, CC_STAT_AREA) < stats_cut.at<int>(i, CC_STAT_AREA))
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
	/*模版定位*/
	/*****************************************************************************************************************/
	Mat im_gray,im_bw;
	resize(samp_im, samp_im, Size(1300, 800));
	cvtColor(samp_im, im_gray, COLOR_BGR2GRAY);
	threshold(im_gray, im_bw, 0, 255, THRESH_BINARY_INV + THRESH_OTSU);
	//通过实验这个卷积核对文字可以，但对头像并不好，因为头像很容易跟下方建筑被分到一个连通域里去~
	Mat bg5, bg6;
	Mat strel7 = getStructuringElement(MORPH_RECT, Size(20, 30)); //定义核
	Mat strel8 = getStructuringElement(MORPH_RECT, Size(10, 30));
	Mat strel9 = getStructuringElement(MORPH_RECT, Size(5, 10));
	morphologyEx(im_bw, bg5, MORPH_CLOSE, strel7);
	morphologyEx(bg5, bg5, MORPH_CLOSE, strel8);
	morphologyEx(bg5, bg6, MORPH_OPEN, strel9);
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

	//筛选文本信息
	int maxIndex_m = 1;
	for (i = 1; i < nccomps_m; i++) {
		if (stats_m.at<int>(maxIndex_m, CC_STAT_AREA) < stats_m.at<int>(i, CC_STAT_AREA))
			maxIndex_m = i;
	}
	Mat samp_copy;
	samp_im.copyTo(samp_copy);
	/*
	x = stats_m.at<int>(maxIndex_m, CC_STAT_LEFT);
	y = stats_m.at<int>(maxIndex_m, CC_STAT_TOP);
	width = stats_m.at<int>(maxIndex_m, CC_STAT_WIDTH);
	height = stats_m.at<int>(maxIndex_m, CC_STAT_HEIGHT);
	rectangle(samp_copy, Rect(x, y, width, height), Scalar(0, 255, 255), 2, 8, 0);
	origin.x = x;
	origin.y = y + 20 + height;
	putText(samp_copy, "Touxiang", origin, FONT_HERSHEY_PLAIN, 1, Scalar(255, 255, 0), 2);
	*/
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
	//imshow("模版定位非头像", samp_copy);
	

	/**************************判断文本信息是否正常*******************************************/
	vector<bool> isChanged = { 0,0,0,0,0,0 };
	vector<int> bad_index;
	vector<int> good_index;
	int diffXY, diffwidth;
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
		diffwidth = abs(stats_m.at<int>(validIndex_m[i], CC_STAT_WIDTH) - stats_cut.at<int>(minindex, CC_STAT_WIDTH))
			+ abs(stats_m.at<int>(validIndex_m[i], CC_STAT_HEIGHT) - stats_cut.at<int>(minindex, CC_STAT_HEIGHT));
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
	/**************************判断图片信息是否正常（仅需要测试图片即可）*******************************************/
	x = 419;
	y = 319;
	width = 556;
	height = 377;
	Mat touxiang = imd_rc(Rect(x, y, width, height));
	imshow("tx", touxiang);

	cvtColor(touxiang, touxiang, COLOR_BGR2GRAY);
	Mat gray_crop_tar;
	threshold(touxiang, gray_crop_tar, 0, 255, THRESH_BINARY + THRESH_OTSU);
	imshow("gray_crop_tar", gray_crop_tar);
	Mat bg7, bg8;
	Mat strel10 = getStructuringElement(MORPH_RECT, Size(10, 30)); //定义核
	Mat strel11 = getStructuringElement(MORPH_RECT, Size(30, 10));
	Mat strel12 = getStructuringElement(MORPH_RECT, Size(5, 10));
	morphologyEx(gray_crop_tar, bg7, MORPH_CLOSE, strel10);
	morphologyEx(bg7, bg7, MORPH_CLOSE, strel1);
	morphologyEx(bg7, bg8, MORPH_OPEN, strel12);
	Mat filter_large_target;
	filter_large_target.create(gray_crop_tar.size(), CV_8U);
	filter_large_target = (255 - bg8);
	multiply(filter_large_target, gray_crop_tar, filter_large_target);
	imshow("bg8", bg8);
	imshow("滤除大面积目标", filter_large_target);

	Mat labels_f, stats_f, centroids_f;
	int nccomps_f = connectedComponentsWithStats(
		filter_large_target, labels_f,
		stats_f, centroids_f
	);
	cout << "Total Connected Components Detected in filter_large_target: " << nccomps_f << endl;
	vector<Vec3b> colors_f(nccomps_f + 1);
	colors_f[0] = Vec3b(0, 0, 0); //初始化颜色表
	for (i = 1; i < nccomps_f; i++) {
		colors_f[i] = Vec3b(rand() % 256, rand() % 256, rand() % 256);
		//if (stats_f.at<int>(i, CC_STAT_AREA) < 200)
			//colors_f[i] = Vec3b(0, 0, 0); // 小于200的区域就置为黑色
	}
	Mat RGB_image_f; //连通域标记图
	RGB_image_f = Mat::zeros(filter_large_target.size(), CV_8UC3);
	for (int y = 0; y < RGB_image_f.rows; y++) {
		for (int x = 0; x < RGB_image_f.cols; x++) {
			int label_f = labels_f.at<int>(y, x);
			CV_Assert(0 <= label_f && label_f <= nccomps_f);
			RGB_image_f.at<Vec3b>(y, x) = colors_f[label_f];
		}
	}
	imshow("连通域标记图f", RGB_image_f);
	string bad_message2 = "图像检测：正常";
	bool flag = false;
	for (i = 0; i < nccomps_f; i++) {
		if (stats_f.at<int>(i, CC_STAT_AREA) < 1000 && stats_f.at<int>(i, CC_STAT_AREA) > 10 &&
			stats_f.at<int>(i, CC_STAT_WIDTH) / stats_f.at<int>(i, CC_STAT_HEIGHT) < 5 &&
			stats_f.at<int>(i, CC_STAT_WIDTH) / stats_f.at<int>(i, CC_STAT_HEIGHT) > 0.2)
		{
			bad_message2 = "有白点";
			flag = true;
			break;
		}
	}



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
		/*else if (i == txIndex) {
			rectangle(anno_im, Rect(x, y, width, height), Scalar(0, 255, 255), 2, 8, 0);
			origin.x = x;
			origin.y = y + 20 + height;
			putText(anno_im, "Touxiang", origin, FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 255), 2);
		}*/
		else if(i != txIndex){
			rectangle(anno_im, Rect(x, y, width, height), Scalar(255, 0, 255), 2, 8, 0);
			origin.x = x;
			origin.y = y + 20 + height;
			putText(anno_im, to_string(i), origin, FONT_HERSHEY_PLAIN, 1, Scalar(255, 0, 255), 2);
		}

	}
	x = 419;
	y = 319;
	width = 556;
	height = 377;
	if (flag) {
		rectangle(anno_im, Rect(x, y, width, height), Scalar(0, 0, 255), 2, 8, 0);
		origin.x = x;
		origin.y = y + 20 + height;
		putText(anno_im, "Touxiang", origin, FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 255), 2);
	}
	else {
		rectangle(anno_im, Rect(x, y, width, height), Scalar(0, 255, 0), 2, 8, 0);
		origin.x = x;
		origin.y = y + 20 + height;
		putText(anno_im, "Touxiang", origin, FONT_HERSHEY_PLAIN, 1, Scalar(0, 255, 0), 2);
	}

	imshow("666", anno_im);
	cout << message << "||" << bad_message2 ;



	waitKey();
    return 0;
}

