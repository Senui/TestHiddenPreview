#include <jni.h>
#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <android/log.h>
#include <time.h>

#define  LOG_TAG    "JNI_PART"

#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace std;
using namespace cv;

Mat * mCanny = NULL;

extern "C" {

void detector(Mat input, vector< vector<int> > &returnMatrix);
void segment(Mat fullImage, vector<Mat> &segmentedImages, vector< vector<int> > &returnMatrix );
void processSegment(Mat segment, vector<int> &detectedBits);
void detectBits(vector<int>& inputPixels, vector<int>& detectedBits);
void getCorrectedPixelsOffset(Mat input, vector<int>& correctPixels);
void decodeBits(vector<int>& inputPixels, vector<int>& detectedBits);
void fillHole(const Mat srcBw, Mat &dstBw);

JNIEXPORT jintArray JNICALL Java_com_example_testhiddenpreview_Decoder_decode(JNIEnv* env, jobject thiz, jint width, jint height,jbyteArray NV21FrameData, jint centerRow, jint centerColumn, jint blobRadius );

JNIEXPORT jintArray JNICALL Java_com_example_testhiddenpreview_Decoder_decode(JNIEnv* env, jobject thiz, jint width, jint height,jbyteArray NV21FrameData, jint centerRow, jint centerColumn, jint blobRadius ){


	jbyte * pNV21FrameData = env->GetByteArrayElements(NV21FrameData, 0);

	// get image
	//Mat gray_image(height, width, CV_8UC1, (unsigned char *) pNV21FrameData);
	//Mat gray_image_copy(height, width, CV_8UC1, (unsigned char *) pNV21FrameData);
	Mat gray_image_copy = imread("greyImage_2blobs.jpg", CV_LOAD_IMAGE_GRAYSCALE);
	Mat gray_image = imread("greyImage_2blobs.jpg", CV_LOAD_IMAGE_GRAYSCALE);

	string file = "/storage/emulated/0/";
	imwrite(file + "greyImage.jpg", gray_image_copy);

	// -- Detect N blobs and return positions + radii in returnMatrix--
  vector<vector<int> > returnMatrix(5, vector<int>(3));
  detector(gray_image_copy, returnMatrix);

  // -- Segment image in N pieces and put each segment in segmentedImages--
  vector<Mat> segmentedImages(returnMatrix.size());
  segment(gray_image, segmentedImages, returnMatrix);

  // -- Process each segment and decode id --
  int codeLength = 2; // Length of id (see Arduino code)
  vector<vector<int> > detectedBits(returnMatrix.size(), vector<int>(codeLength));

  for (int i = 0; i < segmentedImages.size(); i ++){
	detectedBits[i].clear();
	  processSegment(segmentedImages[i], detectedBits[i]);
  }


	//prepare result
	jint fill[detectedBits.size()][detectedBits[0].size()];

	for(int i = 0; i < detectedBits.size(); i++)
		for (int j = 0; j < detectedBits[0].size(); j++)
			fill[i][j] = detectedBits[i][j];


	jintArray result;
	result = env->NewIntArray(detectedBits.size());
	//ONLY FIRST BLOB SENT BACK FOR NOW
	env->SetIntArrayRegion(result, 0, detectedBits.size(), fill[0]);

	env->ReleaseByteArrayElements(NV21FrameData, pNV21FrameData, JNI_ABORT);

	//LOGE("DONE");
	return result;
}

void decodeBits(vector<int>& inputPixels, vector<int> &detectedBits){

	//find peaks and falls
	std::vector<int> peaks;
	std::vector<int> falls;

	int freq = 1500; // fill in preamble freq from arduino code
	int factor = ceil(24000/freq);

	int dif;

	for (int i = 0; i < inputPixels.size()-1; i++){

		if(inputPixels[i] == 0 && inputPixels[i+1] == 255){
			peaks.push_back(i);
			//printf("Peak On: %d \n", i);
		}

		else if(inputPixels[i] == 255 && inputPixels[i+1] == 0 && peaks.size() !=0){
			falls.push_back(i);
			//printf("Fall On: %d \n", i);
		}
	}

	if(peaks.size() > falls.size())
		peaks.erase(peaks.begin() + peaks.size() - 1);

	int j = 1;
	int firstPreamble=0;
	int secondPreamble=0;

	for (int i = 0; i < peaks.size(); i++){

		dif = falls[i] - peaks[i];
		//cout << dif << ' ';

		if ( dif >= 2.5 *factor ){
			if( j == 1 ){
				if(i == 0)
					continue;
				firstPreamble = i;
				j++;
			}
			else{
				secondPreamble = i;
				break;
			}
		}
	}

	if(secondPreamble == 0)
		return;

	//cout << "first " << firstPreamble << " second " << secondPreamble<< "\n" ;


	j = 1;
	int addZero = 0;


	while(true){

		dif = peaks[firstPreamble +j] - falls[firstPreamble + j -1];

		//cout << dif << ' ';

		if(dif > 1.3 * factor ){
			if(j == 1 || firstPreamble + j == secondPreamble){
				//cout << "0" << "\n";
				detectedBits.push_back(0);
			}
			else{
				detectedBits.push_back(0);
				detectedBits.push_back(0);

				//cout << "0" << "\n";
				//cout << "/0" << "\n";
			}
		}
		else{
			if((j != 1 && firstPreamble + j != secondPreamble) || addZero == 1){
				//cout << "0" << "\n";
				detectedBits.push_back(0);
			}
		}


		if (firstPreamble + j == secondPreamble){
		  /*for (int i = 0; i < detectedBits.size(); i++)
			  cout << detectedBits[i] << ' ';*/
			break;
		}

		dif = falls[firstPreamble +j] - peaks[firstPreamble + j];

		//cout << dif << ' ';

		if(dif > 1.5 * factor ){
			detectedBits.push_back(1);
			detectedBits.push_back(1);
			if(firstPreamble + j +1 == secondPreamble)
				addZero = 1;
			//cout << "1" << "\n";
			//cout << "/1" << "\n";
		}
		else{
			//cout << "1" << "\n";
			detectedBits.push_back(1);
		}

		j++;
	}
}

void getCorrectedPixelsOffset(Mat input, vector<int>& correctPixels ){

	int top = 0;
	int bottom = input.rows;

	for(int i = top ; i < bottom ; i++){
		correctPixels.push_back(input.at<unsigned char>(i, 0.5*input.cols) );
	}
}

}

void detector(Mat input, vector< vector<int> > &returnMatrix){

	Mat blurred_image;
	blur(input, blurred_image, Size(100, 100));

	Mat otsu;
	threshold(blurred_image, otsu, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

	Mat cimg = input;

	vector<vector<Point> > contours;
	vector<Point2i> center;
	vector<int> radius;

	findContours(otsu.clone(), contours, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);

	size_t count = contours.size();

	for( size_t i=0; i < count; i++)
	{
			Point2f c;
			float r;
			minEnclosingCircle( contours[i], c, r);

			if (r >= 50)
			{
					center.push_back(c);
					radius.push_back(r);
			}
	}

	int count2 = center.size();
	cv::Scalar red(255,255,255);

	returnMatrix.resize(count2);

	for( int i = 0; i < count2; i++)
	{
			circle(input, center[i], radius[i], red, 3);
			returnMatrix[i][0] = center[i].x;
			returnMatrix[i][1] = center[i].y;
			returnMatrix[i][2] = radius[i];
	}
}

void segment(Mat fullImage, vector<Mat> &segmentedImages, vector< vector<int> > &returnMatrix ){

	int p1, p2;
	for (int i = 0; i < segmentedImages.size(); i++){
		int radius = returnMatrix[i][2];
		p1 = returnMatrix[i][0] - ceil(1.25*radius); // x-coordinate of top left point
		p2 = returnMatrix[i][1] - ceil(1.25*radius); // y-coordinate of top left point

		Rect rect(p1, p2, ceil(2.5*radius), ceil(2.5*radius));
		fullImage(rect).copyTo(segmentedImages[i]);
		//imshow("roi_img", segmentedImages[i]);
		//waitKey(0);
	}
}

void processSegment(Mat segment, vector<int> &detectedBits){

	// Enhance image contrast
	Mat adaptive_equalized;
	Ptr<CLAHE> clahe = createCLAHE();
	clahe->setClipLimit(10);
	clahe->setTilesGridSize(Size(8,8));
	clahe->apply(segment, adaptive_equalized);

	// Blur image
	Mat blurred_segment;
	blur(adaptive_equalized, blurred_segment, Size(3, 3));

	//OTSU image (converts gray to binary)
	Mat adaptive, adaptive0;
	//adaptiveThreshold(blurred_segment, adaptive0, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 105, 2);
	threshold(blurred_segment, adaptive, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
	/*imshow("ad", adaptive);
	waitKey(0);
	imshow("ad", adaptive0);
	waitKey(0);*/

	// Remove noise
	Mat filled_adaptive;
	fillHole(adaptive, filled_adaptive);
	//imshow("ad", filled_adaptive);
	//waitKey(0);

	// Get decode line
	vector<int> correctedPixels;
	getCorrectedPixelsOffset(filled_adaptive, correctedPixels);
	//imwrite("line.jpg", correctedPixels);

	decodeBits(correctedPixels, detectedBits);
}

void fillHole(const Mat srcBw, Mat &dstBw)
{
	Size m_Size = srcBw.size();
	Mat Temp=Mat::zeros(m_Size.height+2,m_Size.width+2,srcBw.type());
	srcBw.copyTo(Temp(Range(1, m_Size.height + 1), Range(1, m_Size.width + 1)));

	floodFill(Temp, Point(0, 0), Scalar(255));

	Mat cutImg;
	Temp(Range(1, m_Size.height + 1), Range(1, m_Size.width + 1)).copyTo(cutImg);

	dstBw = srcBw | (~cutImg);
}


