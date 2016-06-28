/**
    VLC Indoor Localization APP
    jni_part.cpp
    Purpose: Computes smartphone location from image of blobs

    @authors Ahmad Hesam & Ruiling Zhang
    @version 28/06/2016
*/

#include "jni_part.h"

#define  LOG_TAG    "JNI_PART"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using namespace std;
using namespace cv;

// The absolute coordinates of the lamps
vector<vector<double> > lampLocations {
		{-24.5, 168, 0},
		{2.5, 130, 8},
		{36, 166, 5}
};

// Length of id (see Arduino code)
int codeLength = 3;

// The preamble frequency (see Arduino code)
int freq = 3000;

extern "C" {

	JNIEXPORT jdoubleArray JNICALL Java_com_example_testhiddenpreview_Decoder_decode(JNIEnv* env, jobject thiz, jint width, jint height,jbyteArray NV21FrameData, jint centerRow, jint centerColumn, jint blobRadius );

	JNIEXPORT jdoubleArray JNICALL Java_com_example_testhiddenpreview_Decoder_decode(JNIEnv* env, jobject thiz, jint width, jint height,jbyteArray NV21FrameData, jint centerRow, jint centerColumn, jint blobRadius ){

		jbyte * pNV21FrameData = env->GetByteArrayElements(NV21FrameData, 0);

		// -- Create Mat file of the captured image data
		//string file = "/storage/emulated/0/";
		//Mat gray_image = imread(file + "greyImage_input.jpg", CV_LOAD_IMAGE_GRAYSCALE);
		Mat gray_image(height, width, CV_8UC1, (unsigned char *) pNV21FrameData);
		Mat gray_image_copy = gray_image.clone();

		// -- Make an empty array for failed cases
		jdouble fill[1] = {0};
		jdoubleArray empty;
		empty = env->NewDoubleArray(1);
		env->SetDoubleArrayRegion(empty, 0, 1, fill);

		// -- Check if image exists or is readable
		if (! gray_image.data)
		{
			LOGD("Could not read image...");
			return empty;
		}

		// -- Detect the blobs and return their pixel positions and radii
		vector<vector<int> > returnMatrix;
		detector(gray_image_copy, returnMatrix);

		// -- Segment image in N pieces and save each segment in segmentedImages
		vector<Mat> segmentedImages(returnMatrix.size());
		segment(gray_image, segmentedImages, returnMatrix);

		// -- Process each segment and decode the id
		if (returnMatrix.size() != 0){
			vector<vector<int> > detectedBits(returnMatrix.size(), vector<int>(codeLength));

			for (unsigned int i = 0; i < segmentedImages.size(); i ++){
				detectedBits[i].clear();
				if (countNonZero(segmentedImages[i] > 1))
					processSegment(segmentedImages[i], detectedBits[i]);
				else {
					vector<int> emptyVector(codeLength, 0);
					detectedBits[i] = emptyVector;
				}
			}

			int fill[9];
			int arrayLength = sizeof(fill)/sizeof(fill[0]);
			int p = 0;
			for (unsigned int j = 0; j < detectedBits.size(); j++){
				for (int k = 0; k < codeLength; k++)
				{
					fill[p] = detectedBits[j][k];
					p++;
				}
			}

			vector<int> fillArray (fill, fill + sizeof(fill)/sizeof(fill[0]));

			vector<vector<double> > link_loc(lampLocations.size(), vector<double>(lampLocations[0].size()));
			int successLink = link(fillArray, link_loc);
			LOGD("{1} link_loc[0][0] = %.1f", link_loc[0][0]);

			if (successLink == 1){
				double location[3];
				int dimensions = sizeof(location)/sizeof(*location);
				LOGD("{2} link_loc[0][0] = %.1f", link_loc[0][0]);
				angleOfArrival(location, returnMatrix, link_loc);

				jdouble fillCoordinates[dimensions];
				for(int i = 0; i < dimensions; i++){
					fillCoordinates[i] = location[i];
				}

				jdoubleArray coordinates;
				coordinates = env->NewDoubleArray(dimensions);
				env->SetDoubleArrayRegion(coordinates, 0, dimensions, fillCoordinates);

				env->ReleaseByteArrayElements(NV21FrameData, pNV21FrameData, JNI_ABORT);
				return coordinates;
			}
		}

		return empty;
	}
}

Mat blur(Mat input){

	//Blur image
	Mat blurred_image;
	blur(input, blurred_image, Size(3, 3));

	return blurred_image;
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
	imwrite(file + "original.jpg", input);

  Mat blurred_image;
  // blur strength depends on how big the black bars are
  // (else can be seen as separate blobs)
  blur(input, blurred_image, Size(50,50));

  Mat otsu;
  threshold(blurred_image, otsu, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

	imwrite(file + "otsu_detected.jpg", otsu);

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
  for (int i = 0; i < count2; ++i)
  	returnMatrix[i].resize(3); // 3 because: a, b, and radius

  for( int i = 0; i < count2; i++)
  {
	  //circle(input, center[i], radius[i], red, 3);
	  returnMatrix[i][0] = center[i].x;
	  returnMatrix[i][1] = center[i].y;
	  returnMatrix[i][2] = radius[i];
  }

	imwrite(file + "greyImage.jpg", input);
}

void segment(Mat fullImage, vector<Mat> &segmentedImages, vector< vector<int> > &returnMatrix){

	int cols_top, cols_bottom, rows_left, rows_right, col_length, row_length;
	for (unsigned int i = 0; i < segmentedImages.size(); i++){
		int radius = returnMatrix[i][2];
		// x-coordinate of top point
		if(returnMatrix[i][0] - ceil(1.25*radius) >= 0){
			cols_top = returnMatrix[i][0] - ceil(1.25*radius);
		}
		else{
			cols_top = 0;
		}
		// x-coordinate of bottom point
		if(returnMatrix[i][0] + ceil(1.25*radius) <= fullImage.cols){
			cols_bottom = returnMatrix[i][0] + ceil(1.25*radius);
		}
		else{
			cols_bottom = fullImage.cols;
		}
		// y-coordinate of left point
		if(returnMatrix[i][1] - ceil(1.25*radius) >= 0){
			rows_left = returnMatrix[i][1] - ceil(1.25*radius);
		}
		else{
			rows_left = 0;
		}
		// y-coordinate of right point
		if(returnMatrix[i][1] + ceil(1.25*radius) <= fullImage.rows ){
			rows_right = returnMatrix[i][1] + ceil(1.25*radius);
		}
		else{
			rows_right = fullImage.rows;
		}
		col_length = cols_bottom - cols_top;
		row_length = rows_right - rows_left;
		Rect rect(cols_top, rows_left, col_length, row_length);
		fullImage(rect).copyTo(segmentedImages[i]);


		int top, bottom, left, right;
		Mat dst;
		dst = segmentedImages[i];
		top = (int) cols_top - (returnMatrix[i][0] - ceil(1.25*radius));
		bottom = (int) returnMatrix[i][0] + ceil(1.25*radius) - cols_bottom;
		left = (int) rows_left - (returnMatrix[i][1] - ceil(1.25*radius));
		right = (int) returnMatrix[i][1] + ceil(1.25*radius) - rows_right;

		copyMakeBorder(dst, segmentedImages[i],top, bottom, left, right, BORDER_CONSTANT,0);
	}
}

void processSegment(Mat segment, vector<int> &detectedBits){

	Mat blurMask;
	blur(segment, blurMask, Size(50, 50));

	Mat otsu;
	threshold(blurMask, otsu, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

	vector<vector<Point> > contours;
	vector<Point2i> center;
	vector<int> radius;

	findContours(otsu.clone(), contours, CV_RETR_TREE, CV_CHAIN_APPROX_NONE);

	Point2f c;
	float r;
	minEnclosingCircle( contours[0], c, r);

	if (r >= 50)
	{
			center.push_back(c);
			radius.push_back(r);
	}

	Mat circleMask = Mat::zeros(segment.rows, segment.cols, CV_8UC1);
	circle(circleMask, center[0], 0.95*radius[0], Scalar(255,255,255), CV_FILLED, 3);
	Mat invertMask; bitwise_not(circleMask, invertMask);
	imwrite( "invertMask.jpg", invertMask);
	imwrite(" circleSegment.jpg", segment);

	// Enhance image contrast
	Mat adaptive_equalized;
	Ptr<CLAHE> clahe = createCLAHE();
	clahe->setClipLimit(10);
	clahe->setTilesGridSize(Size(9,9));
	clahe->apply(segment, adaptive_equalized);

	// Blur image
	Mat blurred_segment;
	blur(adaptive_equalized, blurred_segment, Size(1, 1));
	blurred_segment = blurred_segment-invertMask;

	//OTSU image (converts gray to binary)
	Mat adaptive, adaptive0;
	threshold(blurred_segment, adaptive, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);

	Mat element = getStructuringElement(MORPH_RECT, Size(5, 5));
	Mat dilate_img;
	Mat erode_img;
	erode(adaptive, erode_img, element);
	dilate(erode_img,dilate_img,element);

	// Get decode line
	vector<int> correctedPixels;
	getCorrectedPixelsOffset(dilate_img, correctedPixels);

	decodeBits(correctedPixels, detectedBits);
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
	int factor = ceil(24000/freq);
	cout << "factor" << factor << "\n";

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
		cout << dif << " falls" << falls[i] << "peaks" << peaks[i]<<"\n";

		if ( dif >= 2.5*factor ){ // look for the big white bars (the 111s in 01110s)
			if( j == 1 ){
				//if(i == 0)
					//continue;
				firstPreamble = i; // set the first big white bar to firstPreamble
//				preambleSize1 = dif;
				j++;
			}
			else{
				secondPreamble = i; // set the second big white bar to firstPreamble
				break;
			}
		}
	}

	if(secondPreamble == 0){
		cout << "\n";
		return;
	}

	cout << "first " << firstPreamble << " second " << secondPreamble<< "\n" ;


	j = 1;

	int check_point, check_id;
	//int distance_preamble;
	//distance_preamble = peaks[secondPreamble] - falls[firstPreamble];
	//set the start point
	check_point = falls[firstPreamble] + 1;
	cout << "initial check_point"<< check_point << "\n";
	//cout << "distance between two preambles "<< distance_preamble << "\n";
	while(true){

		// the black bars that are between the first and second preamble
		//dif = peaks[firstPreamble + j] - falls[firstPreamble + j -1];

		check_point = check_point + 8;
		if (check_point < peaks[secondPreamble] - 4){
			if (j%2 == 1)
			{
				check_id = (j+1)/2 ;
				if (check_point <= peaks[firstPreamble + check_id]){
					cout<< "error10: " << j <<", " <<peaks[firstPreamble + j] << "\n";
					detectedBits.push_back(0);}
				else{
					detectedBits.push_back(1);
					cout<< "error11: " << j <<", " <<peaks[firstPreamble + j] << "\n";
					j++;
				}
			}
			else{
				check_id = j/2 ;
				if (check_point <= falls[firstPreamble + check_id]){
					detectedBits.push_back(1);
					cout<< "error21: " << j <<", " <<falls[firstPreamble + j] << "\n";
				}
				else{
					detectedBits.push_back(0);
					cout<< "error20: " << j <<", " <<falls[firstPreamble + j] << "\n";
					j++;
				}

			}
		}
		else{
			break;
		}
	}
}

int link(vector<int> &ids, vector<vector<double> > &link_loc){
	int num = 0, num_1 = 0, num_2 = 0, num_3 = 0;
	vector< vector<int> > correctIds = {{0,0,1},{1,0,0},{0,1,1}};

	for(int i = 0; i < 3; i++){

		vector<int> id (ids.begin() + 3*i, ids.begin() + 3*i + 3);
		for (std::vector<int>::const_iterator j = id.begin(); j != id.end(); ++j)
		    std::cout << *j << ' ';
		if (id == correctIds[0]){
			link_loc[i][0] = lampLocations[0][0];
			link_loc[i][1] = lampLocations[0][1];
			link_loc[i][2] = lampLocations[0][2];
			num_1 = 1;
		}
		else if (id == correctIds[2]){
			link_loc[i][0] = lampLocations[1][0];
			link_loc[i][1] = lampLocations[1][1];
			link_loc[i][2] = lampLocations[1][2];
			num_2 = 1;
		}
		else if (id == correctIds[1]){
			link_loc[i][0] = lampLocations[2][0];
			link_loc[i][1] = lampLocations[2][1];
			link_loc[i][2] = lampLocations[2][2];
			num_3 = 1;
		}
		else
			return 0;
	}

	num = num_1 + num_2 + num_3;
	if (num == 3)
		return 1;
	else
		return 0;
}

void angleOfArrival(double* loc, vector<vector<int> > &returnMatrix, vector<vector<double> > link_loc){

	LOGD("{3} link_loc[0][0] = %.1f", link_loc[0][0]);
	double Lx1 = link_loc[2][0];
	double Lx2 = link_loc[0][0];
	double Lx3 = link_loc[1][0];
	double Ly1 = link_loc[2][1];
	double Ly2 = link_loc[0][1];
	double Ly3 = link_loc[1][1];
	double Lz1 = link_loc[2][2];
	double Lz2 = link_loc[0][2];
	double Lz3 = link_loc[1][2];

	LOGD("Lx1 = %.1f", Lx1);
	LOGD("Ly2 = %.1f", Ly2);
	LOGD("Lz3 = %.1f", Lz3);

	int a1,a2,a3,b1,b2,b3;
	a1 = returnMatrix[2][0]-960;
	a2 = returnMatrix[0][0]-960;
	a3 = returnMatrix[1][0]-960;
	b1 = 540-returnMatrix[2][1];
	b2 = 540-returnMatrix[0][1];
	b3 = 540-returnMatrix[1][1];

	LOGD("a1 = %d", a1);
	LOGD("a2 = %d", a2);
	LOGD("b1 = %d", b1);
	LOGD("b3 = %d", b3);


	double k1 = (Lx1*b3-Lx3*b3-Ly1*a3+Ly3*a3)/(a1*b3-a3*b1);
	double k2 = (Lx1*a3*b1+Lx2*a1*b3-Lx2*a3*b1-Lx3*a1*b3-Ly1*a1*a3+Ly3*a1*a3)/((a1*b3-a3*b1)*a2);
	double k3 = (Lx1*b1-Lx3*b1-Ly1*a1+Ly3*a1)/(a1*b3-a3*b1);

	LOGD("%k1 = %.5f", k1);
	LOGD("%k2 = %.5f", k2);
	LOGD("%k3 = %.5f", k3);


	double Xc1 = Lx1 - k1*a1;
	double Xc2 = Lx2 - k2*a2;
	double Xc3 = Lx3 - k3*a3;

	double Yc1 = Ly1 - k1*b1;
	double Yc2 = Ly2 - k2*b2;
	double Yc3 = Ly3 - k3*b3;

	double Zf = 1700;

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

