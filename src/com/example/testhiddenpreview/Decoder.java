package com.example.testhiddenpreview;

import java.util.Locale;
import java.lang.Math;

import android.os.Handler;
import android.util.Log;

//separate thread implementation
class Decoder implements Runnable {
	long minPrime;
	
	static String message = "";
	Handler handler = new Handler();
	private static final String TAG = "Decoder Thread";
	
	double[] result;

	Decoder() {
	}

	public void run() {
		
		String binaryString = "";
		String resultStr = "";
		int started = 0;
		
        int tid=android.os.Process.myTid();
        
        Log.d(TAG,"priority before change = " + android.os.Process.getThreadPriority(tid));
		
        Log.d(TAG,"priority after change = " + android.os.Process.getThreadPriority(tid));

		
		while(true){
			
			try {
				FrameData frame = CamCallback.frameQueue.take();
				
				
				//System.out.println("Queue Size: " + CamCallback.frameQueue.size());
				
				System.out.println("Processing:" + frame.number);
				
				
				if(started == 0){
					started = 1;
					resultStr = "";
					continue;
				}
				
				//System.out.println("processed frame " + frame.number);

				long startTime = System.nanoTime();
				CamCallback.BlobRadius = 160;
				
				//System.gc();
				//result = decode(1920, 1080, frame.data, CamCallback.centerRow, CamCallback.centerColumn, CamCallback.BlobRadius);
				result = decode(1920, 1080, frame.data, CamCallback.centerRow, CamCallback.centerColumn, CamCallback.BlobRadius);
				
				//result = new int[0];
				
				long stopTime = System.nanoTime();
			    Log.e(TAG,"SerialTime Millis:"+(float)(stopTime - startTime)/1000000);
				
				if (result.length == 0) {
					System.out.println("Empty");
					continue;
				}
				else if(result.length == 1){
					
					if(result[0] == 0){
						
						System.out.println("NO LIGHT");
						continue;
					}
				}
								
				StringBuilder bitstring = new StringBuilder();
				for (int i = 0; i < result.length; i++) {
					System.out.print(String.format(Locale.US, "---%.2f---", Math.abs(result[i])));
					bitstring.append(String.format(Locale.US, " %.2f,", Math.abs(result[i])));
				}
				
				binaryString = bitstring.toString();
				
				System.out.print("\n");
				System.out.print("**Binary String = " + binaryString + "**\n");
				
				System.out.println("Frame "+ frame.number+": " + resultStr + "// " + frame.timestamp);
				
				if(binaryString.length() != 0) {
					
					System.out.println("---------------------------------");
					System.out.println(binaryString);
					
					
					if(started == 1 ){
						
						final String binaryString2 = binaryString;
						//message = message + str;
						
						//System.out.println(str);
						handler.post(new Runnable(){
							public void run() {
								//MainActivity.debugging.setText("ID: " + binaryString2);
								MainActivity.debugging.setText("Location:" + binaryString2);
							}
						});						
					}
					
					binaryString = "";
					
				}
				
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	

	static {
		System.loadLibrary("jni_part");
	}

	//public native int[] decode(int width, int height, byte[] NV21FrameData, int centerRow, int centerColumn, int blobRadius);
	public native double[] decode(int width, int height, byte[] NV21FrameData, int centerRow, int centerColumn, int blobRadius);
}

