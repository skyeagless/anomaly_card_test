// demo.cpp: �������̨Ӧ�ó������ڵ㡣
//�� OpenCV �У�RGB ͼ���ͨ��˳��Ϊ BGR 

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
	/**********ͼƬ����չʾ*****************/
	Mat samp_im = imread("moban.jpg");
	Mat im = imread("shibie.jpg");
	imshow("ģ��ͼƬ", samp_im);
	imshow("ԭͼRGB", im);
	Mat im_hsv;
	cvtColor(im, im_hsv, COLOR_BGR2HSV);
	imshow("ԭͼHSV", im_hsv);
	Mat imgray;
	cvtColor(im, imgray, COLOR_BGR2GRAY);
	imshow("�Ҷ�ͼ", imgray);
	Mat imbinary;
	threshold(imgray, imbinary, 128, 255, THRESH_BINARY);
	imshow("��ֵͼ", imbinary);
	Mat edge;
	Canny(imbinary, edge, 50, 250);
	imshow("��ԵCanny����", edge);

	/***********************ģ���ı��ָ�*********************************/
	

	waitKey();
	return 0;
}