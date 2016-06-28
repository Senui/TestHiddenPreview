package com.example.testhiddenpreview;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.LinkedBlockingDeque;

import android.content.Context;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.media.MediaScannerConnection;
import android.os.AsyncTask;
import android.os.Environment;
import android.provider.MediaStore;
import android.util.Log;
import android.widget.Toast;

public class CamCallback implements Camera.PreviewCallback{
	
    private static final String TAG = "CamCallback";
    public static boolean enableProcess = false;
       
    static byte[] data;        
    static long distance = -1;    
    static int centerColumn = 995;
    static int centerRow = 540;    
    static int BlobRadius = 0;    
    static int lightOn = 0;    
    static int bits = 2;
    
    public static LinkedBlockingDeque<FrameData> frameQueue; 

    int frameNumber = 0; 
    int i = 0;
    
    static Mode mode = Mode.init;
    Camera camera;
    
    long prevCaptureTime = 0;
    long captureTime = 0;
    
    FileOutputStream stream;
    
    static int expectedValue = 7;
    static int sleep = 1000 - expectedValue;
    
    CamCallback(){

		frameQueue = new LinkedBlockingDeque<FrameData>();
		startDecoder();	    		
    }

	
	@Override
	public void onPreviewFrame(byte[] data, Camera camera){
	     // Process the camera data here
		 		
		this.data = data;
    	this.camera = camera;
		
		if (enableProcess) {
			
			prevCaptureTime = captureTime;
			captureTime = System.nanoTime();		
						
			long dif = captureTime - prevCaptureTime;
			int delay = Math.round ((float)dif/1000000);
			
			Log.e(TAG,"Delay:" + delay );
			
			mode = Mode.decode;

			if(mode == Mode.check_light){
				
				//MainActivity.debugging.setText("Waiting for Light");
				
				int light = checkForLight();
				
				if(light == 1){
					mode= Mode.calculate_distance;
					System.out.println("LightON");
				}			
			}
			else if (mode== Mode.capture_frame){
				
				captureFrame();

				try {
					Thread.sleep(1000);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				
				i++;
				
				
				if(i == 51){
					
					System.out.println("Finished");
					enableProcess = false;
					i=0;
				}
			}
			/*else if(mode== Mode.calculate_distance){
				
				calculateDistance();
				System.out.println("DistanceCalculated");
				
				bits = calculateBits();
				
				System.out.println("Number of bits: "+bits);
				
				enableProcess = false;
				
				if(MainActivity.initiator == 1){
					MainActivity.sendBitsAndData(bits);	
				}
				

				
			}*/
			else if(mode == Mode.decode){
				
				//captureFrame();
				frameNumber++;
				
				/*
				if(frameNumber == 1){
					
	            	MainActivity.message.setText(Long.toString(System.currentTimeMillis()));

				}
				*/
				
				try {
					
					if(frameQueue.size() < 80)
						frameQueue.put(new FrameData(data, frameNumber, captureTime));
					System.out.println("Frame " + frameNumber +" inserted in queue");
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				};
				
				String delayString = delay + " \n";
				
				if(frameNumber == 201){
					//startDecoder();	

					enableProcess = false;
					frameNumber = 0;
				}
				
				try {
					Thread.sleep(sleep); //sleep 1 sec
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
			
			else if(mode == Mode.waiting){
				
				//MainActivity.debugging.setText("Waiting..");
				
				frameNumber++;
				
				String delayString = delay + " \n";
				
			    try {
					stream.write(delayString.getBytes());
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			    
		        try {
					Thread.sleep(1000);
				} catch (InterruptedException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
		        
		        
		        if(frameNumber == 201){
					//startDecoder();	

					enableProcess = false;
					frameNumber = 0;
					
					
					try {
						stream.close();
					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
			}			
		}
	}
	
	public int checkForLight(){
		
		LightChecker p = new LightChecker();
		Thread thread = new Thread(p);
		thread.start();
		
		try {
			thread.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		System.out.println("FOUND "+lightOn);
		
		
		//return lightOn;
		return 1;
		
	}
	
	public void calculateDistance(){
		
		System.out.println(this.distance);
		DistanceCalculator p = new DistanceCalculator();
		Thread thread = new Thread(p);
		thread.start();
		
		try {
			thread.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		System.out.println(distance);
		
		
	}
	
	public void startDecoder(){
		
		Decoder p = new Decoder();
		Thread thread = new Thread(p);
		thread.start();		
		
	}
	
	public int calculateBits(){
		
		int dBlob = 2 * 200;
		
		int width = 8;
		
		int dBit = 2 * width;
		
		float dPreamble = (float) (2.5 * dBit);
		
		float bits = (dBlob - 3 * dPreamble) / (2 * dBit);
		
		
		//System.out.println("Bits: " + bits);

		//System.out.println("FloorBits: " + Math.floor(bits));
		
		return (int) Math.floor(bits);
	}
	
	public void captureFrame(){
		
		//save frame to jpeg
		try {
	        Camera.Parameters parameters = camera.getParameters();
	        
			System.out.println(ImageFormat.getBitsPerPixel(parameters.getPreviewFormat()));

	        
	        Size size = parameters.getPreviewSize();
	        
	        System.out.println(size.height);
	        YuvImage image = new YuvImage(data, parameters.getPreviewFormat(), size.width, size.height, null);
	        
	        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyymmddhhmmss");
	        String date = dateFormat.format(new Date());
	        String photoFile = "Picture_" + date + ".jpg";

	        String filename = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES).getPath() + File.separator + photoFile;
	        //String filename = File.separator + photoFile;
	        //String sdCardRoot = Environment.getExternalStorageDirectory().getAbsolutePath();
	        //File file = new File(sdCardRoot + filename);
	        System.out.println(filename);
	        File file = new File(filename);
	        
	        FileOutputStream filecon = new FileOutputStream(file);
	        image.compressToJpeg(new Rect(0, 0, image.getWidth(), image.getHeight()), 100,filecon);
	        try {
				filecon.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
	        
			MediaScannerConnection.scanFile(MainActivity.context, new String[] { file.getAbsolutePath() }, null, null);
	        
	    } catch (FileNotFoundException e) {
	        System.out.println("File NOT found");
	    }	
	}
}