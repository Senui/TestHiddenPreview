/*
 * jni_part.h
 *
 *  Created on: 28 jun. 2016
 *  Author: Ahmad Hesam & Ruiling Zhang
 */

#ifndef JNI_JNI_PART_H_
#define JNI_JNI_PART_H_

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
#include <string>
#include <sstream>

using namespace std;
using namespace cv;

// Function declarations
Mat clahe(Mat gray_image);
Mat blur(Mat input);

void detector(Mat input, vector< vector<int> > &returnMatrix);
/**
 * @brief Detect the blobs and store their center positions and radii.
 *
 * detector - blurs the N blobs and consequently passes them through a binary filter. This results
 * in an image of N circular-like shapes. OpenCV's findContours function can then detect the
 * contours of these shapes and the function minEnclosingCircle will give us the center position
 * in pixels and radius of each blob.
 *
 * @param [input] the grayscale image of the blobs
 * @param [returnMatrix] a 2D vector that will contain the center positions and raddi of the blobs
 */

void segment(Mat fullImage, vector<Mat> &segmentedImages, vector< vector<int> > &returnMatrix );
/**
 * @brief Segments the full size image into N segmented images, where N is the number of detected blobs
 *
 * segment - For each detected blob a square image will be saved which is 1.25*radius x 1.25*radius.
 * The factor 1.25 is there to be sure all the blob information is stored.
 *
 * In order to avoid accesses that are out of the range of the full size image, it is checked if the
 * segmented image will go beyond this range. If it does, the blob is still segmented,
 * but only the part that is still in range.
 *
 * @param [fullImage] the grayscale image of the blobs
 * @param [segmentedImages] a vector of Mat images that will be filled with the segmented blob images
 * @param [returnMatrix] a 2D vector that will contain the center positions and raddi of the blobs
 */

void processSegment(Mat segment, vector<int> &detectedBits);
/**
 * @brief Processes the image to prepare it for blob decoding.
 *
 * processSegment - The following series of image processing techniques are applied for each segmented
 * image (i.e. containing a single blob):
 * 1. clahe 		: increase contrast between the dark and light bars
 * 2. blur			: blur the blob in order to reduce granularity
 * 3. mask			: remove the noise in the area surrounding the blob to increase performance of next step
 * 4. threshold	: apply binary threshold to get black and white bars for decoding
 * 5. erode &		: smoothen the bars to decode more accurately
 * 		dilate
 *
 * After the processing it calls getCorrectedPixelsOffset and decodeBits.
 *
 * @param [segment] a Mat image that contains a single blob
 * @param [detectedBits] a vector that will contain the ids of the N blobs
 */

void getCorrectedPixelsOffset(Mat input, vector<int>& correctPixels );
/**
 * @brief Extracts the middle column of an image.
 *
 * getCorrectedPixelsOffset - a for loop that takes the middle pixel of each row and stores it in a
 * vector. This vector of pixels shall further be referred to as the "decode line".
 *
 * @param [input] the processed segmented image that contains the blob in black and white bars
 * @param [correctPixels] a vector that will store the middle column of pixels of input
 */

void decodeBits(vector<int>& inputPixels, vector<int> &detectedBits);
/**
 * @brief Decodes the id of a single blob from its middle column of pixels.
 *
 * decodeBits - It reads the decode line pixel by pixel and detects at which pixel there are "peaks"
 * and "falls". A "peak" is defined as a pixel where the decode line makes a transition from black to
 * white. A "fall" is defined as a pixel where the decode line makes the opposite transition. Also a
 * "factor" is defined as the theoretical width of a black or white bar and can be calculated using
 * the encoding frequency of the Arduino boards.
 *
 * Next, an attempt will be made to detect two preambles in the decode line. A preamble is detected by
 * its characteristically long white bars (i.e. three consecutive 1s). If the preambles are
 * successfully detected, their position in the decode line is marked. Else the function returns
 * prematurely.
 *
 * Next, the bars between the position of the first and second preamble are considered for decoding.
 * There should be at least 2 black bars in this region that are part of the preamble. The remaining
 * detected bits make up the id of the blob.
 *
 * Decoding technique:
 * 1. Start decoding the id at the last pixel of the first preamble - call this position 0.
 * And a 'checkpoint' is set at position 1. The state of the bit at this first checkpoint
 * is always 0, as it is part of the first preamble.
 * 2. A "factor" number of pixels are added to the checkpoint and it is check whether a transition
 * has occurred or not.
 * 3. If a transition occurred, the next bit must be a 1 and if not, the next bit must be a 0.
 * 4. A "factor" number of pixels is added until the black bar that belongs to the second preamble
 * is reached.
 *
 * @param [inputPixels] a vector containing the decode line
 * @param [detectedBits] a vector that will contain the ids of the N blobs
 */

int link(vector<int> &ids, vector<vector<double> > &link_loc);
/**
 * @brief Links the detected ids to the absolute position of the lamps
 *
 * link - subvectors are created that contain the number of bits that make a single id.
 * A subvector is compared to the possible ids that exist in the VLC system. If a match is found
 * the lamp locations that correspond to that id are placed in a 2D vector. Else, the function
 * returns prematurely. Only when all the blobs are linked to their corresponding lamp, the function
 * returns a success value of 1.
 *
 * @param [ids] a vector that contains all the ids of the blobs
 * @param [link_loc] a 2D vector that will contain the lamp locations in the correct order for later
 * computations
 */

void angleOfArrival(double* loc, vector<vector<int> > &returnMatrix, vector<vector<double> > link_loc);
/**
 * @brief Computes the location of the smartphone through the Angle of Arrival method.
 *
 * angleOfArrival -
 *
 * @param [loc]
 * @param [returnMatrix]
 * @param [link_loc] a 2D vector that will contain the lamp locations in the correct order
 */


#endif /* JNI_JNI_PART_H_ */
