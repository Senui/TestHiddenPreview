package com.example.testhiddenpreview;

import java.util.Locale;

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




		//android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_DISPLAY);
		
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
				
				/*
				try {
					Thread.sleep(2000);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				*/
				
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
					System.out.print(String.format(Locale.US, "---%.2f---", result[i]));
					bitstring.append(String.format(Locale.US, "%.2f, ", result[i]));
				}
				
				bitstring.append("Z");
				binaryString = bitstring.toString();
				
				System.out.print("\n");
				System.out.print("**Binary String = " + binaryString + "**\n");

				
				/*
				//transform to binary
				for (int i = 0; i < result.length - 1; i=i+2) {
					
					if(result[i] == 0 && result[i+1] == 1)
						resultStr = resultStr + "0" ;
					else if (result[i] == 1 && result[i+1] == 0)
						resultStr = resultStr + "1" ;
				}*/
				
				System.out.println("Frame "+ frame.number+": " + resultStr + "// " + frame.timestamp);

				/*if(resultStr.equals("0000")){
					
					System.out.println("Message Start!!!");
					binaryString = "";
					started = 0;
					if(!message.equals(""))
						message = message + "//";
					continue;
					
				}
				
				resultStr = "";
				*/
				
				
				if(binaryString.length() != 0) {
					
					System.out.println("---------------------------------");
					System.out.println(binaryString);
					
					
					if(started == 1 ){
						//System.out.println(binaryString);
						
						//int charCode = Integer.parseInt(binaryString, 2);
						//final String str = new Character((char)charCode).toString();
						//System.out.println("Character: "+ str);
						final String binaryString2 = binaryString;
						//message = message + str;
						
						//System.out.println(str);
						handler.post(new Runnable(){
							public void run() {
								//MainActivity.debugging.setText("ID: " + binaryString2);
								MainActivity.debugging.setText("Location:" + binaryString2);
							}
						});
						/*
						MainActivity.runOnUiThread(new Runnable() {
				            @Override
				            public void run() {
				                // This code will always run on the UI thread, therefore is safe to modify UI elements.
				                MainActivity.message.setText("my text");
				            }
				        });
				        */
						
						/*MainActivity.handler.post(new Runnable() {
							public void run() {
								MainActivity.message.setText(Decoder.message);
							}
						});*/
						
						
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

