package com.example.testhiddenpreview;

import android.util.Log;

//separate thread implementation
class DistanceCalculator implements Runnable {
	long minPrime;

	private static final String TAG = "DistanceCalculator Thread";

	DistanceCalculator() {
	}

	public void run() {

		// android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_FOREGROUND);

		long startTime = System.nanoTime();

		int[] result = blobDetector(1920, 1080, CamCallback.data);

		CamCallback.centerColumn = result[0];
		CamCallback.centerRow = result[1];
		CamCallback.BlobRadius = result[2];
		
		//CamCallback.BlobRadius = 175;
		
		System.out.println("column " +  result[0] + " row " + result[1] + " radius "+ result[2]);
		
		//CamCallback.distance = calculateDistance(CamCallback.BlobRadius);

		CamCallback.distance = calculateDistance(75);
	}
	
	
	public long calculateDistance(int blobRadius){
		
		int blobSize = 2 * blobRadius;
		double x = 0;
		long distance = 0;
		
		double a1 = -10.5;
		double b1 = 885;
		double a2 = -2.5;
		double b2 = 405;
		
		
		if(blobSize > 253){
			
			x = (blobSize - b1) / a1;
			
		}
		else{
			
			x = (blobSize - b2) / a2;

		}
		
		//round to closest 10 multiple
		distance = (long) (Math.floor(x/10) * 10);
		
		
		return distance;
		
	}

	static {
		System.loadLibrary("jni_part");
	}

	public native int[] blobDetector(int width, int height, byte[] NV21FrameData);
}
