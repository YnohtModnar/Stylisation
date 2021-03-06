#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/core/mat.hpp>
#include <string>
#include "tests.hpp"
#include "gradient.hpp"
#include "utils.hpp"
#include "tensor.hpp"
#include "dog.hpp"
#include "etf.hpp"
#include "fdog.hpp"
#include "fbl.hpp"
#include "quantif.hpp"
#include "args/fblArgs.hpp"
#include "args/fdogArgs.hpp"
#include "args/kMeanArgs.hpp"
// #include <lime-master/sources/lime.hpp>

Mat testColorAndDoG(Mat& img, Mat& color) {
	Mat imgdouble;
	img.convertTo(imgdouble, CV_32F, 1 / 255.0);
	Mat dog = DoG(imgdouble, 5.0, 3.0);
	//namedWindow("DoG",WINDOW_NORMAL);
	//imshow("DoG",dog);
	double min, max;
	minMaxLoc(dog, &min, &max);
	float threshold = max * 4 / 100;
	Mat edges = zeroCrossingMat(dog, threshold);
	float an = 3;
	Mat element = getStructuringElement(MORPH_ELLIPSE, Size(an*2+1, an*2 + 1),
										Point(an, an));
	//dilate(edges,edges,element);
	//namedWindow("passage par zero dog, threshold 4 percent",WINDOW_NORMAL);
	//imshow("passage par zero dog, threshold 4 percent", edges);
	Mat coloredEdges(color.rows, color.cols, color.type());// = Quantification(color, 20);
	// float pas(255. / 50);
	//dilate(coloredEdges,coloredEdges,element);
	//GaussianBlur(coloredEdges, coloredEdges, Size(an * 2 + 1, an * 2 + 1), 3.5);
	//Mat coloredEdges(color);
	cvtColor(color, coloredEdges, CV_BGR2Lab);
	for (int i(0);i < color.rows;i++) {
		for (int j(0);j < color.cols;j++) {
			if (edges.at<uchar>(i, j) == 255)
				coloredEdges.at<Vec3b>(i, j)[0] *= 0.75;
			// else {
			// 	coloredEdges.at<Vec3b>(i, j) = color.at<Vec3b>(i, j);
			// 	coloredEdges.at<Vec3b>(i, j)[1] = round((float)color.at<Vec3b>(i, j)[1] / pas)*pas;
			// 	coloredEdges.at<Vec3b>(i, j)[2] = round((float)color.at<Vec3b>(i, j)[2] / pas)*pas;
			// }
		}
	}
	//namedWindow("Colored edges",WINDOW_NORMAL);
	//imshow("Colored edges", coloredEdges);
	cvtColor(coloredEdges, coloredEdges, CV_Lab2BGR);
	return coloredEdges;
}

void testConvol() {
	Mat kernel(3, 3, CV_32F);
	Mat test(9, 9, CV_32F, 0.0);
	test.at<float>(4, 4) = 1.0;
	test.at<float>(3, 3) = 1.0;
	for(int i = 1; i < 10; i++)
		kernel.at<float>(i - 1) = i;
	std::cout << kernel << '\n';
	std::cout << test << '\n';
	std::cout << "Convolution result: " << convolution(test,kernel,3,3) << '\n';
}

void testDoG(Mat& img) {
	Mat imgdouble;
	img.convertTo(imgdouble, CV_32F, 1/255.0);
	Mat dog = DoG(imgdouble, 5.0, 3.0);
	imshow("DoG",dog);
	// std::cout << dog << '\n';
	Mat test = zeroCrossingMat(dog);
	imshow("passage par zero dog, threshold 0 percent", test);
	imwrite("0percentDog.jpg", test);
	double min, max;
	minMaxLoc(dog, &min, &max);
	float threshold = max * 4/100;
	Mat testThreshold = zeroCrossingMat(dog, threshold);
	imshow("passage par zero dog, threshold 4 percent", testThreshold);
	imwrite("4percentDog.jpg", testThreshold);
}

void testTensor(Mat& img) {
	Mat dx, dy;
	Mat doubleImg;
	Point2f gradientVector, normalVector;
	Mat isophote(img.rows, img.cols, CV_32F);
	Mat normal(img.rows, img.cols, CV_32F);
	Mat coherence(img.rows, img.cols, CV_32F);
	Mat dpx(img.rows, img.cols, CV_32F);
	Mat* smoothedTensor = new Mat[3];
	Mat* tensorArray;
	Mat tensor(2,2,CV_32F);
	Mat u8Normal, u8Isophote, u8Coherence, u8DpX;
	double min, max;

	img.convertTo(doubleImg, CV_32F, 1/255.0);
	Sobel(doubleImg,dx, CV_32F, 1, 0, 1);
	Sobel(doubleImg,dy, CV_32F, 0, 1, 1);

	tensorArray = tensorStructureArray(dx, dy);

	GaussianBlur(tensorArray[0], smoothedTensor[0], Size(0, 0), 1);
	GaussianBlur(tensorArray[1], smoothedTensor[1], Size(0, 0), 1);
	GaussianBlur(tensorArray[2], smoothedTensor[2], Size(0, 0), 1);

	for (int i = 0; i < img.rows; i++) {
		for (int j = 0; j < img.cols; j++) {
			tensor.at<float>(0, 0) = smoothedTensor[0].at<float>(i, j);
			tensor.at<float>(0, 1) = smoothedTensor[2].at<float>(i, j);
			tensor.at<float>(1, 0) = smoothedTensor[2].at<float>(i, j);
			tensor.at<float>(1, 1) = smoothedTensor[1].at<float>(i, j);
			eigen(tensor, &gradientVector, &normalVector,
				  &isophote.at<float>(i, j), &normal.at<float>(i, j));
			coherence.at<float>(i, j) = coherenceNorm(isophote.at<float>(i, j),
													  normal.at<float>(i, j));
			dpx.at<float>(i, j) = dpX(isophote.at<float>(i, j),
									  normal.at<float>(i, j), 0.1, 0.0001);
		}
	}
	minMaxLoc(normal, &min, &max);
	std::cout << "Normal:" << '\n';
	std::cout << "min: " << min << " max: " << max << '\n';
	normal.convertTo(u8Normal, CV_8UC1, 255.0/max);
	minMaxLoc(isophote, &min, &max);
	std::cout << "Isophote" << '\n';
	std::cout << "min: " << min << " max: " << max << '\n';
	isophote.convertTo(u8Isophote, CV_8UC1, 255.0/max);
	minMaxLoc(isophote, &min, &max);
	std::cout << "Coherence Norm:" << '\n';
	minMaxLoc(coherence, &min, &max);
	std::cout << "min: " << min << " max: " << max << '\n';
	coherence.convertTo(u8Coherence, CV_8UC1, 255.0/max);
	std::cout << "DpX Norm:" << '\n';
	minMaxLoc(dpx, &min, &max);
	std::cout << "min: " << min << " max: " << max << '\n';
	dpx.convertTo(u8DpX, CV_8UC1, 255.0/max);

	namedWindow("normale", WINDOW_NORMAL);
	imshow("normale", u8Normal);
	namedWindow("isophote", WINDOW_NORMAL);
	imshow("isophote", u8Isophote);
	namedWindow("Coherence", WINDOW_NORMAL);
	imshow("Coherence", coherence);
	namedWindow("dpx", WINDOW_NORMAL);
	imshow("dpx", u8DpX);

	//threshold
	Mat imgToShow = thresholdUpperValues(dpx, 0.7);
	namedWindow("Norm thresholded", WINDOW_NORMAL);
	imshow("Norm thresholded", imgToShow);

	Mat imgModified = img.clone();
	for (int i = 0; i < img.rows; i++)
		for (int j = 0; j < img.cols; j++)
			if(imgToShow.at<uchar>(i, j)==255)
				imgModified.at<uchar>(i, j) = 0;


	namedWindow("img modif", WINDOW_NORMAL);
	imshow("img modif", imgModified);

	waitKey();
}


Mat testDrawing(Mat& img, Mat& color) {
	Mat imgdouble;
	img.convertTo(imgdouble, CV_32F, 1 / 255.0);
	Mat dog = DoG(imgdouble, 5.0, 3.0);
	//namedWindow("DoG",WINDOW_NORMAL);
	//imshow("DoG",dog);
	double min, max;
	minMaxLoc(dog, &min, &max);
	float threshold = max * 4 / 100;
	Mat edges = zeroCrossingMat(dog, threshold);
	Mat canvas(color.rows, color.cols, color.type());
	for (int i(0); i < canvas.rows; i++) {
		for (int j(0); j < canvas.cols; j++) {
			canvas.at<Vec3b>(i, j)[0] = 255;
			canvas.at<Vec3b>(i, j)[1] = 255;
			canvas.at<Vec3b>(i, j)[2] = 255;
		}
	}
	int length(5);
	for (int i(5);i < edges.rows-5;i++) {
		for (int j(5); j < edges.cols - 5; j++) {
			if (edges.at<uchar>(i, j) == 255) {
				Scalar c(color.at<Vec3b>(i, j)[0], color.at<Vec3b>(i, j)[1],
						 color.at<Vec3b>(i, j)[2]);
				if (edges.at<uchar>(i, j + 2) == 255) {
					if (edges.at<uchar>(i + 2, j + 2) == 255) {
						line(canvas, Point(j - length, i - length),
							 Point(j + length, i + length), c);
					}
					else if (edges.at<uchar>(i - 2, j + 2) == 255) {
						line(canvas, Point(j - length, i + length),
							 Point(j + length, i - length), c);
					}
					else {
						line(canvas, Point(j - length, i),
							 Point(j + length, i), c);
					}
				}
				else if (edges.at<uchar>(i + 2, j) == 255)
					line(canvas, Point(j, i - length), Point(j, i + length), c);
			}
			//line(color,Point(j,i-5.5),Point(j+1.5,i+5),Scalar(12.,12.,12.));
		}
	}
	//namedWindow("Drawing test", WINDOW_NORMAL);
	//imshow("Drawing test", color);

	return canvas;
}

void testQuantif(Mat& img) {
	float an = 10;
	Mat element = getStructuringElement(MORPH_ELLIPSE, Size(an * 2 + 1,
										an * 2 + 1), Point(an, an));
	Mat color = Quantification(img, 10);
	dilate(color, color, element);
	GaussianBlur(color, color, Size(an * 2 + 1, an * 2 + 1), 3.5);
	cv::namedWindow("Colored edges", WINDOW_NORMAL);
	cv::imshow("Colored edges", color);
}

void testAbstraction(Mat& color, string imgName) {
	Mat floatGray, floatColor, gradX, gradY, ucEdges;
	Mat etf, fdog, fbl, abstraction, quant;
	Mat gVectMap = Mat::zeros(color.rows, color.cols, CV_32FC2);
	Mat isoVectMap = Mat::zeros(color.rows, color.cols, CV_32FC2);
	Mat gHat = cv::Mat::zeros(color.rows, color.cols, CV_32FC1);
	double min, max;
	float gx, gy, mag;
	FdogArgs fdogA;
	FblArgs fblA;
	KMeanArgs kmArgs;
	std::string csvInfo = imgName + ";" + std::to_string(color.rows) + "x" +
						  std::to_string(color.cols) + ";";

	color.convertTo(floatColor, CV_32FC3, 1/255.0);
	cvtColor(floatColor, floatGray, COLOR_BGR2GRAY);

	// get gradient
	Sobel(floatGray, gradX, CV_32F, 1, 0);
	Sobel(floatGray, gradY, CV_32F, 0, 1);

	for (int y = 0; y < color.rows; y++) {
		for (int x = 0; x < color.cols; x++) {
			gx = gradX.at<float>(y, x);
			gy = gradY.at<float>(y, x);

			mag = sqrt(gx*gx + gy*gy);

			gHat.at<float>(y, x) = mag;
			// construction de vecteurs orthogonaux aux gradient -> tangent au contour
			isoVectMap.at<float>(y, x * 2 + 0) = -gy / (mag + 1.0e-8);
			isoVectMap.at<float>(y, x * 2 + 1) = gx / (mag + 1.0e-8);
		}
	}

	etf = computeETF(isoVectMap, gHat, 5, 3);

	fdogA.setArgs();
	fdogA.print();
	csvInfo += fdogA.csvFormatted();
	fdog = fDoG(floatGray, etf, fdogA.getArg(0), fdogA.getArg(1),
				fdogA.getArg(2), fdogA.getArg(3), fdogA.getArg(4));
	minMaxLoc(fdog, &min, &max);
	fdog.convertTo(ucEdges, CV_8UC1, 255.0/max);
	namedWindow("fdog", WINDOW_NORMAL);
	imshow("fdog", ucEdges);
	imwrite("fdog.png", ucEdges);
	waitKey();

	fblA.setArgs();
	fblA.print();
	csvInfo += fblA.csvFormatted();
	fbl = computeFBL(floatColor, etf, fblA.getArg(0), fblA.getArg(1),
					 fblA.getArg(2), fblA.getArg(3), fblA.getArg(4));
	minMaxLoc(fbl, &min, &max);
	fbl.convertTo(fbl, CV_8UC3, 255/max);
	namedWindow("fbl", WINDOW_NORMAL);
	imshow("fbl", fbl);
	imwrite("fbl.png", fbl);

	kmArgs.setArgs();
	csvInfo += fblA.csvFormatted();
	quant = LuminanceQuant(fbl, kmArgs.getArg(0));
	namedWindow("quantif", WINDOW_NORMAL);
	imshow("quantif", quant);
	imwrite("quantif.png", quant);
	waitKey();

	abstraction = quant.clone();
	// fusion of quantification and edges
	for (int y = 0; y < quant.rows; y++)
		for (int x = 0; x < quant.cols; x++)
			if (ucEdges.at<uchar>(y, x) == 0)
				abstraction.at<Vec3b>(y, x) = 0;

	namedWindow("abstraction", WINDOW_NORMAL);
	imshow("abstraction", abstraction);
	imwrite("abstraction.png", abstraction);
	waitKey();
	std::cout << "CSV formatted parameter used (order: imgInfos, fdogParams, fblParams, quantisation)\n";

	std::cout << setprecision(4) << csvInfo << '\n';
}
