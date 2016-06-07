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

vector<vector<double> > lampLocations {
		{-24.5, 168, 0},
		{2.5, 130, 8},
		{36, 166, 5}
};

extern "C" {

float pow2roundup (int x);
int roundUp(int numToRound, int multiple);
Mat clahe(Mat gray_image);
Mat histogramEqualization(Mat input);
void detector(Mat input, vector< vector<int> > &returnMatrix);
Mat adaptiveThreshold(Mat input);
Mat blur(Mat input);
void getMiddlePixels(Mat input, int centerRadius[3], vector<int>& pixels);
void centerToZero(vector<int>& inputPixels, vector<int>& outputPixels);
void detectBits(vector<int>& inputPixels, vector<int>& detectedBits);
double averagePixelsForLightCheck (Mat input);

double averageBlobPixelsForLight(Mat input, int column, int row ,int radius);

void avoidBlobCenterOffset(vector<int>& inputPixels, vector<int>& offsetBlobStart);
void getCorrectedPixelsCenterOffset(Mat input, int centerRadius[3], vector<int>& offsetBlobStart, vector<int>& correctPixels );

int avoidBlobOffset(Mat input, int centerRadius[3]);
//void getCorrectedPixelsOffset(Mat input, int centerRadius[3], int offset, vector<int>& correctPixels );
//void decodeBits(vector<int>& inputPixels, vector<int>& detectedBits);

void segment(Mat fullImage, vector<Mat> &segmentedImages, vector< vector<int> > &returnMatrix );
void processSegment(Mat segment, vector<int> &detectedBits);
void fillHole(const Mat srcBw, Mat &dstBw);
void getCorrectedPixelsOffset(Mat input, vector<int>& correctPixels );
void decodeBits(vector<int>& inputPixels, vector<int> &detectedBits);
void angleOfArrival(double loc[], vector< vector<int> > &returnMatrix);


JNIEXPORT void JNICALL Java_com_example_testhiddenpreview_CamCallback_fftProcessing(JNIEnv* env, jobject thiz, jint width, jint height,jbyteArray NV21FrameData);

JNIEXPORT jintArray JNICALL Java_com_example_testhiddenpreview_CamCallback_ImageProcessing(JNIEnv* env, jobject thiz, jint width, jint height,jbyteArray NV21FrameData);

JNIEXPORT jintArray JNICALL Java_com_example_testhiddenpreview_CamCallback_ImageProcessing2(JNIEnv* env, jobject thiz, jint width, jint height,jbyteArray NV21FrameData);


JNIEXPORT jintArray JNICALL Java_com_example_testhiddenpreview_DistanceCalculator_blobDetector(JNIEnv* env, jobject thiz, jint width, jint height,jbyteArray NV21FrameData);

JNIEXPORT jint JNICALL Java_com_example_testhiddenpreview_LightChecker_lightCheck(JNIEnv* env, jobject thiz, jint width, jint height,jbyteArray NV21FrameData);

JNIEXPORT jintArray JNICALL Java_com_example_testhiddenpreview_Decoder_decode(JNIEnv* env, jobject thiz, jint width, jint height,jbyteArray NV21FrameData, jint centerRow, jint centerColumn, jint blobRadius );

JNIEXPORT jintArray JNICALL Java_com_example_testhiddenpreview_Decoder_decode(JNIEnv* env, jobject thiz, jint width, jint height,jbyteArray NV21FrameData, jint centerRow, jint centerColumn, jint blobRadius ){

	jbyte * pNV21FrameData = env->GetByteArrayElements(NV21FrameData, 0);

	// get image
	Mat gray_image(height, width, CV_8UC1, (unsigned char *) pNV21FrameData);
	Mat gray_image_copy = gray_image.clone();

	clock_t begin, end;
	double time_spent;

	vector<vector<int> > returnMatrix(5, vector<int>(3)); // contains center positions and radii of blobs
	detector(gray_image_copy, returnMatrix);

	// -- Segment image in N pieces and put each segment in segmentedImages--
	vector<Mat> segmentedImages(returnMatrix.size());
	segment(gray_image, segmentedImages, returnMatrix);

  // -- Process each segment and decode id --
  int codeLength = 3; // Length of id (see Arduino code)
  int allCodes = 1*codeLength;

  if (returnMatrix.size() != 0){ //if we got some result
		vector<vector<int> > detectedBits(returnMatrix.size(), vector<int>(codeLength));

		for (unsigned int i = 0; i < segmentedImages.size(); i ++){
			detectedBits[i].clear();
			if (countNonZero(segmentedImages[i] > 1))
				processSegment(segmentedImages[i], detectedBits[i]);
			else { //case when blob is on the image border (TODO: make better solution)
				vector<int> emptyVector(codeLength, 0);
				detectedBits[i] = emptyVector;
			}
		}

		double location[3];
		int dimensions = sizeof(location)/sizeof(*location);
		angleOfArrival(location, returnMatrix); // TODO: use least squares method for localization


		//////////////////////////
		// SEND BACK DETECTED BITS (START)
		//////////////////////////

		//prepare result (TODO: send all detected bits back or compute AoA here)
		jint fill[codeLength];

		// fill only first detected blob
		for(int i=0; i < codeLength; i++){
			fill[i] = detectedBits[0][i];
		}

		// fill for all blobs
		/*int p = 0;
		for (unsigned int j = 0; j < detectedBits.size(); j++){
			cout << "Lamp " << j << ": ";
			for (int k = 0; k < codeLength; k++)
			{
				fill[p] = detectedBits[j][k];
				p++;
			}
		}*/

		jintArray result;
		result = env->NewIntArray(codeLength);
		env->SetIntArrayRegion(result, 0, codeLength, fill);

		env->ReleaseByteArrayElements(NV21FrameData, pNV21FrameData, JNI_ABORT);
		return result;

		//////////////////////////
		// SEND BACK DETECTED BITS (END)
		//////////////////////////


		//////////////////////////
		// SEND BACK LOCATION (START)
		//////////////////////////
		/*jdouble fillCoordinates[dimensions];
		for(int i = 0; i < dimensions; i++){
			fillCoordinates[i] = location[i];
		}

		jdoubleArray coordinates;
		coordinates = env->NewDoubleArray(dimensions);
		env->SetDoubleArrayRegion(coordinates, 0, dimensions, fillCoordinates);

		env->ReleaseByteArrayElements(NV21FrameData, pNV21FrameData, JNI_ABORT);
		return coordinates;*/
  }
  else{
  	jint fill[1] = {0};
  	jintArray result;
		result = env->NewIntArray(1);
		env->SetIntArrayRegion(result, 0, 1, fill);
  	return result;
  }
}


JNIEXPORT jint JNICALL Java_com_example_testhiddenpreview_LightChecker_lightCheck(JNIEnv* env, jobject thiz, jint width, jint height,jbyteArray NV21FrameData){

	jbyte * pNV21FrameData = env->GetByteArrayElements(NV21FrameData, 0);

	int result = 0;
	// get image
	Mat gray_image(height, width, CV_8UC1, (unsigned char *) pNV21FrameData);


	if(averagePixelsForLightCheck(gray_image) > 0)
		result = 1;



	env->ReleaseByteArrayElements(NV21FrameData, pNV21FrameData, JNI_ABORT);

	return result;

}

JNIEXPORT jintArray JNICALL Java_com_example_testhiddenpreview_DistanceCalculator_blobDetector(JNIEnv* env, jobject thiz, jint width, jint height,
		jbyteArray NV21FrameData) {

	jbyte * pNV21FrameData = env->GetByteArrayElements(NV21FrameData, 0);

	clock_t begin, end;
	double time_spent;

	// get image
	Mat gray_image(height, width, CV_8UC1, (unsigned char *) pNV21FrameData);

	begin = clock();

	//apply adaptive histogram equalization
	Mat adaptive_equalized = clahe(gray_image);

	end = clock();
	time_spent = (double) (end - begin) / CLOCKS_PER_SEC;

	LOGE("CLAHE =  %.5f . \n", time_spent * 1000);

	//find transmitter
	int centerRadius[3];

	begin = clock();

	//detector(adaptive_equalized, centerRadius);

	end = clock();
	time_spent = (double) (end - begin) / CLOCKS_PER_SEC;

	LOGE("Detector =  %.5f . \n", time_spent * 1000);

	//cout << "centerColumn: " << centerRadius[0] << " centerRow: " << centerRadius[1] << " radius: " << centerRadius[2] << std::endl;

	int centerColumn = centerRadius[0];
	int centerRow = centerRadius[1];
	int circleRadius = centerRadius[2];

	//prepare result
	jint fill[3];

	fill[0] = centerColumn;
	fill[1] = centerRow;
	fill[2] = circleRadius;


	jintArray result;
	result = env->NewIntArray(3);
	env->SetIntArrayRegion(result, 0, 3, fill);

	env->ReleaseByteArrayElements(NV21FrameData, pNV21FrameData, JNI_ABORT);

	LOGE("DONE");
	return result;
}


JNIEXPORT jintArray JNICALL Java_com_example_testhiddenpreview_CamCallback_ImageProcessing(JNIEnv* env, jobject thiz, jint width, jint height,
		jbyteArray NV21FrameData) {

	jbyte * pNV21FrameData = env->GetByteArrayElements(NV21FrameData, 0);

	Mat gray_image(height, width, CV_8UC1, (unsigned char *) pNV21FrameData);

	//blur image for smoothing

	Mat blurred_image;

	blur(gray_image, blurred_image, Size(10, 10));

	//Binary filter
	Mat thresholded_image;
	threshold(blurred_image, thresholded_image, 1, 255, THRESH_BINARY);

	unsigned char middle_column[thresholded_image.rows];

	for (int i = 0; i < thresholded_image.rows; i++) {

		middle_column[i] = thresholded_image.at<unsigned char>(i,
		thresholded_image.cols / 2 - 1);
		//printf("%d\n", middle_column[i]);

	}

	//find peaks and falls
	std::vector<int> peaks;
	std::vector<int> falls;

	for (int i = 0; i < thresholded_image.rows - 1; i++) {

		if (middle_column[i] == 0 && middle_column[i + 1] == 255) {
			peaks.push_back(i);
			//printf("On: %d \n", i);

		} else if (middle_column[i] == 255 && middle_column[i + 1] == 0
				&& peaks.size() != 0) {
			falls.push_back(i);
			//printf("Off: %d \n", i);
		}

	}

	//find preambles
	std::vector<int> preambles;
	for (int i = 0; i < peaks.size() - 2; i++) {
		//printf("%d \n", peaks[i]);

		int dif = peaks[i + 1] - peaks[i];
		int nextdif = peaks[i + 2] - peaks[i + 1];

		if (nextdif < 1.5 * dif) {

			if (i + 4 <= peaks.size() - 1) {

				int extradif = peaks[i + 3] - peaks[i + 2];
				if (extradif < dif)
					continue;

				LOGE("Preamble at %d \n", i);
				preambles.push_back(i);
			}
			i = i + 3;
		}
	}


	jint fill[preambles.size()];


	for (int i = 0; i < preambles.size(); i++) {

		int preamble = preambles[i];
		int firstBitDif = peaks[preamble + 3] - peaks[preamble + 2];
		int secondBitDif = peaks[preamble + 4] - peaks[preamble + 3];

		int onPeriodFirstBit = falls[preamble + 2] - peaks[preamble + 2];
		int onPeriodSecondBit = falls[preamble + 3] - peaks[preamble + 3];

		if (onPeriodFirstBit > firstBitDif / 2 	&& onPeriodSecondBit > secondBitDif / 2) {
			LOGE("Found 1\n");
			fill[i]= 1;
		} else if (onPeriodFirstBit > firstBitDif / 2 && onPeriodSecondBit < secondBitDif / 2) {
			LOGE("Found 2\n");
			fill[i]= 2;
		} else if (onPeriodFirstBit < firstBitDif / 2 && onPeriodSecondBit > secondBitDif / 2) {
			LOGE("Found 3\n");
			fill[i]= 3;
		} else if (onPeriodFirstBit < firstBitDif / 2 && onPeriodSecondBit < secondBitDif / 2) {
			LOGE("Found 4\n");
			fill[i]= 4;
		}

	}

	jintArray result;
	result = env->NewIntArray(preambles.size());
	env->SetIntArrayRegion(result, 0, preambles.size(), fill);


	env->ReleaseByteArrayElements(NV21FrameData, pNV21FrameData, 0);

	return result;
	LOGE("DONE");
}


JNIEXPORT void JNICALL Java_com_example_testhiddenpreview_CamCallback_fftProcessing(JNIEnv* env, jobject thiz, jint width, jint height,jbyteArray NV21FrameData) {
	jbyte * pNV21FrameData = env->GetByteArrayElements(NV21FrameData, 0);

	Mat gray_image(height, width, CV_8UC1, (unsigned char *) pNV21FrameData);
	return;
}


void getCorrectedPixelsCenterOffset(Mat input, int centerRadius[3], vector<int>& offsetBlobStart, vector<int>& correctPixels ){

	float factor = 1;
	int newRadius = ceil(centerRadius[2] * factor);

	int top = centerRadius[1] - newRadius;
	int bottom = centerRadius[1] + newRadius;

	if(top < 0){
		top = 0;
	}
	if(bottom > input.rows){
		bottom = input.rows;
	}

	for(int i = top ; i< bottom ; i++){

		if( (i >= offsetBlobStart[1] + top ) && (i <= offsetBlobStart[1] + top + offsetBlobStart[0])  ){
			correctPixels.push_back(input.at<unsigned char>(i, centerRadius[0] - offsetBlobStart[0]) );
		}
		else{
			correctPixels.push_back(input.at<unsigned char>(i, centerRadius[0]));
		}

	}

}



void avoidBlobCenterOffset(vector<int>& inputPixels, vector<int>& offsetBlobStart){


	//find peaks and falls
	std::vector<int> peaks;
	std::vector<int> falls;

	for (int i = 0; i < inputPixels.size()-1; i++){

		if(inputPixels[i] == 0 && inputPixels[i+1] == 255){
			peaks.push_back(i);
			//printf("On: %d \n", i);

		}
		else if(inputPixels[i] == 255 && inputPixels[i+1] == 0 && peaks.size() !=0){
			falls.push_back(i);
			//printf("Off: %d \n", i);
		}

	}

	int blobStart = 0;
	int blobEnd = 0;

	for (int i = 0; i < falls.size(); i++){

		int dif = falls[i] - peaks[i];

		if (dif > 56){ //freq = 3000 -> dif > 28

			if(blobStart == 0){
				blobStart = peaks[i];
			}

		}
		else if (dif < 40 && blobStart != 0 ){
			blobEnd = falls[i-1];
			break;
		}

	}

	offsetBlobStart.push_back(blobEnd - blobStart);
	offsetBlobStart.push_back(blobStart);

}

int avoidBlobOffset(Mat input, int centerRadius[3]){

	float factor = 1;
	int newRadius = ceil(centerRadius[2] * factor);

	int top = centerRadius[1] - newRadius;
	int bottom = centerRadius[1] + newRadius;

	if(top < 0){
		top = 0;
	}
	if(bottom > input.rows){
		bottom = input.rows;
	}

	int column = centerRadius[0];

	while(1){

		vector<int> pixels;

		for(int i = top ; i< bottom ; i++){

			pixels.push_back(input.at<unsigned char>(i, column));
		}


		//find peaks and falls
		std::vector<int> peaks;
		std::vector<int> falls;

		for (int i = 0; i < pixels.size()-1; i++){

			if(pixels[i] == 0 && pixels[i+1] == 255){
				peaks.push_back(i);
				//printf("On: %d \n", i);
				//fflush(stdout);

			}
			else if(pixels[i] == 255 && pixels[i+1] == 0 && peaks.size() !=0){
				falls.push_back(i);
				//printf("Off: %d \n", i);
				//fflush(stdout);
			}
		}

		int noBlob = 0;

		for (int i = 0; i < falls.size(); i++){

			int dif = falls[i] - peaks[i];

			if (dif > 3 * 16){ //freq = 3000 -> 3*8
				column = column - 1;
				noBlob = 1;
			}
		}

		if(noBlob == 0)
			break;
	}

	return centerRadius[0] - column;
}


int roundUp(int numToRound, int multiple)
{
   return ((numToRound + multiple - 1) / multiple) * multiple;
}

float pow2roundup (int x)
{
    if (x < 0)
        return 0;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x+1;
}

}


void detectBits(vector<int>& inputPixels, vector<int>& detectedBits){

	//find peaks and falls
		std::vector<int> peaks;
		std::vector<int> falls;

		int freq = 1500; // fill in preamble freq from arduino code
		int factor = ceil(24000/freq);

		int dif;

		for (unsigned int i = 0; i < inputPixels.size() - 1; i++){

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
		// preamble looks like: 01110

		for (unsigned int i = 0; i < peaks.size(); i++){

			dif = falls[i] - peaks[i];
			//cout << dif << ' ';

			if ( dif >= 2.5 *factor ){ // look for the big white bars (the 111s in 01110s)
				if( j == 1 ){
					if(i == 0)
						continue;
					firstPreamble = i; // set the first big white bar to firstPreamble
					j++;
				}
				else{
					secondPreamble = i; // set the second big white bar to firstPreamble
					break;
				}
			}
		}

		if(secondPreamble == 0)
			return;

		cout << "first " << firstPreamble << " second " << secondPreamble<< "\n" ;


		j = 1;
		int addZero = 0;


		while(true){

			// the black bars that are between the first and second preamble
			dif = peaks[firstPreamble + j] - falls[firstPreamble + j -1];

			cout << dif << ' ';

			if(dif > 1.0 * factor && dif < 1.1*factor ){ // if black bar is one periode
				if(j == 1 || firstPreamble + j == secondPreamble){ // if it is one of the 0s in 01110
					//cout << "0" << "\n";
					detectedBits.push_back(0); // detect it as a single 0
				}
				else{
					detectedBits.push_back(0);
					detectedBits.push_back(0);

					//cout << "0" << "\n";
					//cout << "/0" << "\n";
				}
			}
			else if (dif >= 1.1*factor) {
				if (dif > 2.6 * factor){ // if we get 00 after the last preamble 0
					detectedBits.push_back(0);
					detectedBits.push_back(0);
				}
				else{ // if we get only 0 after the last preamble 0
					detectedBits.push_back(0);
				}
			}

			else {
				if((j != 1 && firstPreamble + j != secondPreamble) || addZero == 1){
					//cout << "0" << "\n";
					detectedBits.push_back(0);
				}
			}


			if (firstPreamble + j == secondPreamble){
			  for (unsigned int i = 0; i < detectedBits.size(); i++)
				  cout << detectedBits[i] << ' ';
				break;
			}

			// the white bars that are between the first and second preamble
			dif = falls[firstPreamble +j] - peaks[firstPreamble + j];

			cout << dif << ' ';

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


void centerToZero(vector<int>& inputPixels, vector<int>& outputPixels){

	for(int i = 0 ; i< inputPixels.size(); i++){

		if(inputPixels[i] == 255)
			outputPixels.push_back(1);
		else
			outputPixels.push_back(0);

	}
}


double averagePixelsForLightCheck (Mat input){

	std::vector<double> averageVertical;

	for(int i = 0 ; i< input.rows ; i++){

		int sum = 0;
		for(int j = 0 ; j< input.cols ; j++){
			sum = sum + input.at<unsigned char>(i, j);
		}

		averageVertical.push_back(sum/input.cols);

	}

	int sum = 0;
	for(int i = 0 ; i< input.rows ; i++){
		sum = sum + averageVertical[i];
	}

	double average = sum / input.rows;

	LOGE("Average %d \n", average);


	return average;
}

double averageBlobPixelsForLight(Mat input, int column, int row ,int radius){

	std::vector<double> averageVertical;


	int top = row - radius;
	int bottom = row + radius;

	if(top < 0){
		top = 0;
	}
	if(bottom > input.rows){
		bottom = input.rows;
	}

	//printf("Start: %d End: %d\n", top, bottom);
	int sum = 0;

	for(int i = top ; i< bottom ; i++){

		sum = sum + input.at<unsigned char>(i, column);

	}


	double average = sum / radius;

	return average;

}



void getMiddlePixels(Mat input, int centerRadius[3], vector<int>& pixels){

	float factor = 1;
	int newRadius = ceil(centerRadius[2] * factor);
	int offset = centerRadius[2];

	int top = centerRadius[1] - newRadius;
	int bottom = centerRadius[1] + newRadius;

	if(top < 0){
		top = 0;
	}
	if(bottom > input.rows){
		bottom = input.rows;
	}

	//printf("Start: %d End: %d\n", top, bottom);

	for(int i = top ; i< bottom ; i++){

		pixels.push_back(input.at<unsigned char>(i, centerRadius[0]));

	}

}

Mat adaptiveThreshold(Mat input){

	Mat adaptive;

	adaptiveThreshold(input, adaptive, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 105, 2);

	return adaptive;

}

Mat blur(Mat input){

	//Blur image
	Mat blurred_image;
	blur(input, blurred_image, Size(3, 3));

	return blurred_image;

}

Mat histogramEqualization(Mat input){

	Mat equalized;
	equalizeHist(input, equalized);

	return equalized;

}


Mat clahe(Mat input){

	Ptr<CLAHE> clahe = createCLAHE();
	clahe->setClipLimit(10);
	clahe->setTilesGridSize(Size(8,8));

	Mat adaptive_equalized;
	clahe->apply(input,adaptive_equalized);


	return adaptive_equalized;
}


void detector(Mat input, vector< vector<int> > &returnMatrix){

	string file = "/storage/emulated/0/";

  Mat blurred_image;
  // blur strength depends on how big the black bars are
  // (else can be seen as separate blobs)
  blur(input, blurred_image, Size(150,150));

  Mat otsu;
  threshold(blurred_image, otsu, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

	imwrite(file + "otsuBlobImage.jpg", otsu);

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
	  //circle(input, center[i], radius[i], red, 3);
	  returnMatrix[i][0] = center[i].x;
	  returnMatrix[i][1] = center[i].y;
	  returnMatrix[i][2] = radius[i];
  }

	imwrite(file + "greyImage.jpg", input);
}

void segment(Mat fullImage, vector<Mat> &segmentedImages, vector< vector<int> > &returnMatrix ){

	int p1, p2;
		for (int i = 0; i < segmentedImages.size(); i++){
			int radius = returnMatrix[i][2];
			if (returnMatrix[i][0] - ceil(1.25*radius) >= 0) //check if it falls outside border
				p1 = returnMatrix[i][0] - ceil(1.25*radius); // x-coordinate of top left point
			else
				p1 = 0;

			if ( returnMatrix[i][1] - ceil(1.25*radius) >= 0) //check if it falls outside border
				p2 = returnMatrix[i][1] - ceil(1.25*radius); // y-coordinate of top left point
			else
				p2 = 0;

		if (returnMatrix[i][0] + ceil(1.25*radius) <= fullImage.cols && returnMatrix[i][1] + ceil(1.25*radius) <= fullImage.rows){
			Rect rect(p1, p2, ceil(2.5*radius), ceil(2.5*radius));
			fullImage(rect).copyTo(segmentedImages[i]);
			string file = "/storage/emulated/0/";
			imwrite(file + "segImages.jpg", segmentedImages[i]);
		}
		else //if image falls outside border, make empty matrix (TODO: make better solution)
			segmentedImages[i] = Mat::zeros(1, 1, CV_8U);
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
	//Mat filled_adaptive;
	//fillHole(adaptive, filled_adaptive);
	//imshow("ad", filled_adaptive);
	//waitKey(0);

	// Get decode line
	vector<int> correctedPixels;
	getCorrectedPixelsOffset(adaptive, correctedPixels);
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

void getCorrectedPixelsOffset(Mat input, vector<int>& correctPixels ){

	int top = 0;
	int bottom = input.rows;

	for(int i = top ; i < bottom ; i++){
		correctPixels.push_back(input.at<unsigned char>(i, 0.5*input.cols) );
	}
}

void decodeBits(vector<int>& inputPixels, vector<int> &detectedBits){
	  //find peaks and falls
		std::vector<int> peaks;
		std::vector<int> falls;

		int freq = 2500; // fill in preamble freq from arduino code
		int factor = ceil(24000/freq);

		int dif;

		for (unsigned int i = 0; i < inputPixels.size() - 1; i++){

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
		// preamble looks like: 01110

		for (unsigned int i = 0; i < peaks.size(); i++){

			dif = falls[i] - peaks[i];
			//cout << dif << ' ';

			if ( dif >= 2.5 *factor ){ // look for the big white bars (the 111s in 01110s)
				if( j == 1 ){
					//if(i == 0)
						//continue;
					firstPreamble = i; // set the first big white bar to firstPreamble
					j++;
				}
				else{
					secondPreamble = i; // set the second big white bar to firstPreamble
					break;
				}
			}
		}

		if(secondPreamble == 0)
			return;

		cout << "first " << firstPreamble << " second " << secondPreamble<< "\n" ;


		j = 1;
		int addZero = 0;


		while(true){

			// the black bars that are between the first and second preamble
			dif = peaks[firstPreamble + j] - falls[firstPreamble + j -1];

			cout << dif << ' ';

			if (dif >= 1.1*factor) {
				if (dif > 2.0 * factor){ // if we get 00 after the last preamble 0
					detectedBits.push_back(0);
					detectedBits.push_back(0);
				}
				else{ // if we get only 0 after the last preamble 0
					detectedBits.push_back(0);
				}
			}

			else {
				if((j != 1 && firstPreamble + j != secondPreamble) || addZero == 1){
					//cout << "0" << "\n";
					detectedBits.push_back(0);
				}
			}


			if (firstPreamble + j == secondPreamble){
			  for (unsigned int i = 0; i < detectedBits.size(); i++)
				  cout << detectedBits[i] << ' ';
				break;
			}

			// the white bars that are between the first and second preamble
			dif = falls[firstPreamble +j] - peaks[firstPreamble + j];

			cout << dif << ' ';

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

void angleOfArrival(double loc[], vector< vector<int> > &returnMatrix){
	double Lx1 = lampLocations[0][0];
	double Lx2 = lampLocations[1][0];
	double Lx3 = lampLocations[2][0];

	double Ly1 = lampLocations[0][1];
	double Ly2 = lampLocations[1][1];
	double Ly3 = lampLocations[2][1];

	double Lz1 = lampLocations[0][2];
	double Lz2 = lampLocations[1][2];
	double Lz3 = lampLocations[2][2];

	int a1 = returnMatrix[2][0]-960;
	int a2 = returnMatrix[0][0]-960;
	int a3 = returnMatrix[1][0]-960;
	int b1 = 540-returnMatrix[2][1];
	int b2 = 540-returnMatrix[0][1];
	int b3 = 540-returnMatrix[1][1];

	/*cout << "a1 = " << a1 << endl;
	cout << "a2 = " << a2 << endl;
	cout << "a3 = " << a3 << endl;
	cout << "b1 = " << b1 << endl;
	cout << "b2 = " << b2 << endl;
	cout << "b3 = " << b3 << endl;*/

	//cout << "Lx2 = "<< a1 << endl;

	double k1 = (Lx1*b3-Lx3*b3-Ly1*a3+Ly3*a3)/(a1*b3-a3*b1);
	double k2 = (Lx1*a3*b1+Lx2*a1*b3-Lx2*a3*b1-Lx3*a1*b3-Ly1*a1*a3+Ly3*a1*a3)/((a1*b3-a3*b1)*a2);
	double k3 = (Lx1*b1-Lx3*b1-Ly1*a1+Ly3*a1)/(a1*b3-a3*b1);

	double Xc1 = Lx1 - k1*a1;
	double Xc2 = Lx2 - k2*a2;
	double Xc3 = Lx3 - k3*a3;

	double Yc1 = Ly1 - k1*b1;
	double Yc2 = Ly2 - k2*b2;
	double Yc3 = Ly3 - k3*b3;

	//double Zf = (Lz1 - Lz3) / (k1 - k3);
	double Zf = 1604;

	double Zc1 = Lz1 + k1*Zf;
	double Zc2 = Lz2 + k2*Zf;
	double Zc3 = Lz3 + k3*Zf;

	double Z_avg = (Zc1 + Zc2 + Zc3)/3;
	double X_avg = (Xc1 + Xc2 + Xc3)/3;
	double Y_avg = (Yc1 + Yc2 + Yc3)/3;

	loc[0] = X_avg;
	loc[1] = Y_avg;
	loc[2] = Z_avg;
}

