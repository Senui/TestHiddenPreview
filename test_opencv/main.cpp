#include <opencv2/core/core.hpp>
#include "opencv2/opencv.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

using namespace cv;
using namespace std;

vector<vector<double> > lampLocations {
	{-24.5, 168, 0},
	{2.5, 130, 0},
	{36, 166, 0}
};

int lineNr = 0;

void detector(Mat input, vector< vector<int> > &returnMatrix )
{
	imwrite("original.jpg", input);

  Mat blurred_image;
  blur(input, blurred_image, Size(50, 50));
  imwrite("blurred_image.jpg", blurred_image);

  Mat otsu;
  threshold(blurred_image, otsu, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
  imwrite("otsu_detected.jpg", otsu);

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
      //cout << radius[i];
  }

  cout << "Lamp X: x, y, r" << endl;
	for ( std::vector<std::vector<int> >::size_type i = 0; i < returnMatrix.size(); i++ )
	{
		 cout << "Lamp " << i << ": ";
	   for ( std::vector<int>::size_type j = 0; j < returnMatrix[i].size(); j++ )
	   {
	  	 std::cout << returnMatrix[i][j] << ' ';
	   }
	   std::cout << std::endl;
	}
	cout << endl;

  //namedWindow( "Display window (1)", WINDOW_AUTOSIZE );   // Create a window for display.
  //imshow( "Display window (1)", blurred_image);       	  // Show our image inside it.

  //namedWindow( "Display window", WINDOW_AUTOSIZE );   	// Create a window for display.
  imwrite( "detected_circles.jpg", input);       						  // Show our image inside it.

  waitKey(0);                                          	// Wait for a keystroke in the window
}

void segment(Mat fullImage, vector<Mat> &segmentedImages, vector< vector<int> > &returnMatrix){

	int p1, p2;
	for (unsigned int i = 0; i < segmentedImages.size(); i++){
		int radius = returnMatrix[i][2];
		if (returnMatrix[i][0] - ceil(1.25*radius) >= 0)
			p1 = returnMatrix[i][0] - ceil(1.25*radius); // x-coordinate of top left point
		else
			p1 = 0;

		if ( returnMatrix[i][1] - ceil(1.25*radius) >= 0)
			p2 = returnMatrix[i][1] - ceil(1.25*radius); // y-coordinate of top left point
		else
			p2 = 0;

		if (returnMatrix[i][0] + ceil(1.25*radius) <= fullImage.cols && returnMatrix[i][1] + ceil(1.25*radius) <= fullImage.rows){
			Rect rect(p1, p2, ceil(2.5*radius), ceil(2.5*radius));
			fullImage(rect).copyTo(segmentedImages[i]);
			//imshow("roi_img", segmentedImages[i]);
			//waitKey(0);
		}
		else {
			segmentedImages[i] = Mat::zeros(1, 1, CV_8U);
		}
	}
}
void decodeBits(vector<int>& inputPixels, vector<int> &detectedBits){
	//find peaks and falls
	std::vector<int> peaks;
	std::vector<int> falls;

	int freq = 3000; // fill in preamble freq from arduino code
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

	if(secondPreamble == 0){
		cout << "\n";
		return;
	}

	cout << "first " << firstPreamble << " second " << secondPreamble<< "\n" ;


	j = 1;
//	int addZero = 0;

	int check_point;
	//int distance_preamble;
	//distance_preamble = peaks[secondPreamble] - falls[firstPreamble];
	//set the start point
	check_point = falls[firstPreamble] + 4;
	cout << "initial check_point"<< check_point << "\n";
	//cout << "distance between two preambles "<< distance_preamble << "\n";
	while(true){

		// the black bars that are between the first and second preamble
		//dif = peaks[firstPreamble + j] - falls[firstPreamble + j -1];

		// cout << dif << ' ';

		check_point = check_point + 8;
		if (check_point < peaks[secondPreamble] - 4){
			if (j%2 == 1)
			{
				if (check_point < peaks[firstPreamble + j])
					detectedBits.push_back(0);
				else{
					detectedBits.push_back(1);
					j++;
				}
			}
			else{
				if (check_point < falls[firstPreamble + j-1])
					detectedBits.push_back(1);
				else{
					detectedBits.push_back(0);
					j++;
				}

			}
		}
		else{
			break;
		}



/*		if (dif >= 1.1*factor) {

			if (dif > 2.0 * factor){ // if we get 00 after the last preamble 0
				detectedBits.push_back(0);
				detectedBits.push_back(0);
			}
			else { // if we get only 0 after the last preamble 0
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
			//for (unsigned int i = 0; i < detectedBits.size(); i++)
				//cout << detectedBits[i] << ' ';
			break;
		}

		// the white bars that are between the first and second preamble
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

		j++;*/
	}

}

void getCorrectedPixelsOffset(Mat input, vector<int>& correctPixels ){

	int top = 0;
	int bottom = input.rows;

	for(int i = top ; i < bottom ; i++){
		correctPixels.push_back(input.at<unsigned char>(i, 0.5*input.cols) );
	}
}

void fillHole(const Mat srcBw, Mat &dstBw)
{
	Size m_Size = srcBw.size();
	Mat Temp = Mat::zeros(m_Size.height+2,m_Size.width+2,srcBw.type());
	srcBw.copyTo(Temp(Range(1, m_Size.height + 1), Range(1, m_Size.width + 1)));

	floodFill(Temp, Point(0, 0), Scalar(255));

	Mat cutImg;
	Temp(Range(1, m_Size.height + 1), Range(1, m_Size.width + 1)).copyTo(cutImg);

	dstBw = srcBw | (~cutImg);
}

void processSegment(Mat segment, vector<int> &detectedBits){

	// Enhance image contrast
	Mat adaptive_equalized;
	Ptr<CLAHE> clahe = createCLAHE();
	clahe->setClipLimit(10);
	clahe->setTilesGridSize(Size(8,8));
	clahe->apply(segment, adaptive_equalized);

	//imshow("segment", segment);
	//waitKey(0);
	//imshow("ad_eq", adaptive_equalized);
	//waitKey(0);

	// Blur image
	Mat blurred_segment;
	blur(adaptive_equalized, blurred_segment, Size(3, 3));

	//OTSU image (converts gray to binary)
	Mat adaptive, adaptive0;
	//adaptiveThreshold(blurred_segment, adaptive0, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 105, 2);
	threshold(blurred_segment, adaptive, 0, 255, CV_THRESH_BINARY | CV_THRESH_OTSU);
//	namedWindow("ad", WINDOW_NORMAL);
//	imshow("ad", adaptive);
//	waitKey(0);
	Mat element = getStructuringElement(MORPH_RECT, Size(5, 5));
	Mat dilate_img;
	Mat erode_img;
	erode(adaptive, erode_img, element);
	dilate(erode_img,dilate_img,element);
	//imshow("erode",dilate_img);
	//waitKey(0);
	// Remove noise
	Mat filled_adaptive;
	fillHole(dilate_img, filled_adaptive);
	//namedWindow("ad_filled", WINDOW_NORMAL);
	//imshow("ad_filled", filled_adaptive);
	//waitKey(0);

	// Get decode line
	vector<int> correctedPixels;
	getCorrectedPixelsOffset(filled_adaptive, correctedPixels);
	//vector<string> line = {"line1.jpg", "line2.jpg", "line3.jpg"};
	//imwrite(line[lineNr], correctedPixels);
	lineNr++;

	decodeBits(correctedPixels, detectedBits);
}

void angleOfArrival(double loc[], vector< vector<int> > &returnMatrix){
	double Lx1 = lampLocations[0][0];
	double Lx2 = lampLocations[1][0];
	double Lx3 = lampLocations[2][0];
	double Ly1 = lampLocations[0][1];
	double Ly2 = lampLocations[1][1];
	double Ly3 = lampLocations[2][1];

	int a1 = returnMatrix[2][0]-960;
	int a2 = returnMatrix[0][0]-960;
	int a3 = returnMatrix[1][0]-960;
	int b1 = 540-returnMatrix[2][1];
	int b2 = 540-returnMatrix[0][1];
	int b3 = 540-returnMatrix[1][1];
	  cout << "a1 " << ": " << a1 <<"\n";
	  cout << "a2 " << ": " << a2 <<"\n";
	  cout << "a3 " << ": " << a3 <<"\n";
	  cout << "b1 " << ": " << b1 <<"\n";
	  cout << "b2 " << ": " << b2 <<"\n";
	  cout << "b3 " << ": " << b3 <<"\n";

	double k1 = (Lx1*b3-Lx3*b3-Ly1*a3+Ly3*a3)/(a1*b3-a3*b1);
	double k2 = (Lx1*a3*b1+Lx2*a1*b3-Lx2*a3*b1-Lx3*a1*b3-Ly1*a1*a3+Ly3*a1*a3)/((a1*b3-a3*b1)*a2);
	double k3 = (Lx1*b1-Lx3*b1-Ly1*a1+Ly3*a1)/(a1*b3-a3*b1);

	double Xc1 = Lx1 - k1*a1;
	double Xc2 = Lx2 - k2*a2;
	double Xc3 = Lx3 - k3*a3;

	double Yc1 = Ly1 - k1*b1;
	double Yc2 = Ly2 - k2*b2;
	double Yc3 = Ly3 - k3*b3;

	double X_avg = (Xc1 + Xc2 + Xc3)/3;
	double Y_avg = (Yc1 + Yc2 + Yc3)/3;

	loc[0] = X_avg;
	loc[1] = Y_avg;
}


int main(void){
  // -- Import image --
  Mat image, imageCopy;
  image = imread("X_106h.jpg", CV_LOAD_IMAGE_GRAYSCALE);   	// Read the file
  imageCopy = imread("X_106h.jpg", CV_LOAD_IMAGE_GRAYSCALE);  // Read the file

  if(! image.data ) // Check for invalid input
  {
		cout <<  "Could not open or find the image" << std::endl ;
		return -1;
  }

  // -- Detect N blobs and return positions + radii in returnMatrix --
  vector<vector<int> > returnMatrix(5, vector<int>(3));
  detector(imageCopy, returnMatrix);

  // -- Segment image in N pieces and put each segment in segmentedImages --
  vector<Mat> segmentedImages(returnMatrix.size());
  segment(image, segmentedImages, returnMatrix);

  // -- Process each segment and decode id --
  int codeLength = 3; // Length of id (see Arduino code)
  if (returnMatrix.size() != 0){
  	vector<vector<int> > detectedBits(returnMatrix.size(), vector<int>(codeLength));

		for (unsigned int i = 0; i < segmentedImages.size(); i ++){
			detectedBits[i].clear();
			if (countNonZero(segmentedImages[i] > 1))
				processSegment(segmentedImages[i], detectedBits[i]);
			else{
				vector<int> emptyVector(codeLength, 0);
				detectedBits[i] = emptyVector;
			}
		}

		//cout << "detected bit size = " << detectedBits[0].size() << endl;

		// -- Output decoded id --
		for (unsigned int j = 0; j < detectedBits.size(); j++){
			cout << "Lamp " << j << ": ";
			for (int k = 0; k < codeLength; k++)
			{
				cout << detectedBits[j][k] << ' ' ;
			}
			cout << "\n";
		}
  }

  else
  	cout << "No blob found!" << endl ;
//lamp localization
  double location[2];
  angleOfArrival(location, returnMatrix);

  cout << "X = " << location[0] << ", Y = " << location[1] << endl;
  return 0;

}

