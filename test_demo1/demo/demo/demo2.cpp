// demo.cpp: 定义控制台应用程序的入口点。
//在 OpenCV 中，RGB 图像的通道顺序为 BGR 

#include "stdafx.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <vector>


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

	/***********************模版文本分割*********************************/
	

	waitKey();
	return 0;
}