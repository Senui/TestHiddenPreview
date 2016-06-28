package com.example.testhiddenpreview;

import java.io.File;
//import java.io.FileDescriptor;
//import java.io.FileInputStream;
//import java.io.FileOutputStream;
import java.io.IOException;

import com.android.future.usb.UsbAccessory;
import com.android.future.usb.UsbManager;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.media.MediaScannerConnection;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.util.TypedValue;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup.LayoutParams;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

  public class MainActivity extends Activity implements SensorEventListener {
  private SurfaceView preview=null;
  private SurfaceHolder previewHolder=null;
  private Camera camera=null;
  private boolean inPreview=false;
  private boolean cameraConfigured=false;
  
  private int PreviewSizeWidth = 1920;
  private int PreviewSizeHeight = 1080;

  private Button processButton;
  private Button exposureCompButton;
  private Button lockCameraParams;
  //private Button turnLightOn;
  private Button beginTransmission;
  
  private SensorManager mSensorManager;
  //private Sensor mLight;
  
  //int checkLight = 0;
  private static final float MAX_EXPOSURE_COMPENSATION = 0.5f;
  private static final float MIN_EXPOSURE_COMPENSATION = -20.0f;


  CamCallback camCallback;
  public static CamPreview camPreview;

	private static final String TAG = "MainActivity-ArduinoAccessory";
	
	private static final String ACTION_USB_PERMISSION = "com.google.android.DemoKit.action.USB_PERMISSION";
	  
	private UsbManager mUsbManager;
	private PendingIntent mPermissionIntent;
	private boolean mPermissionRequestPending;
	private ToggleButton buttonLED;
	
	//UsbAccessory mAccessory;
	//ParcelFileDescriptor mFileDescriptor;
	//FileInputStream mInputStream;
	//static FileOutputStream mOutputStream;
  
	//static File delayFile;
	
    static Context context;
    
    public static TextView xPos, yPos, zPos;
    public static TextView debugging;
    public static TextView message;
    
    static Handler handler;
    Parameters parameters;
    
    static int initiator = 0;

  
  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    
    setContentView(R.layout.activity_main);
    
    mSensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
	//mLight = mSensorManager.getDefaultSensor(Sensor.TYPE_LIGHT);
    
    // Setup the camera and the preview object
    Camera mCamera = Camera.open(0);
    camPreview = new CamPreview(this,mCamera);
    camPreview.setSurfaceTextureListener(camPreview);
    
    
    parameters = mCamera.getParameters();
    //System.out.println(parameters.flatten());

    // Connect the preview object to a FrameLayout in your UI
    // You'll have to create a FrameLayout object in your UI to place this preview in
    RelativeLayout preview = (RelativeLayout) findViewById(R.id.cameraView); 
    preview.addView(camPreview);
    

    //create buttons
   
    processButton = new Button(this);
    processButton.setText("Process");
    processButton.setId(1);
    processButton.setPadding(50,50,50,50);
    RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
    lp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);  
    preview.addView(processButton, lp);
    
    exposureCompButton = new Button(this);
    exposureCompButton.setText("BestExp");
    exposureCompButton.setId(2);
    exposureCompButton.setPadding(50,50,50,50);
    lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
    lp.addRule(RelativeLayout.ALIGN_PARENT_LEFT);  
    preview.addView(exposureCompButton, lp);
    
    lockCameraParams = new Button(this);
    lockCameraParams.setText("LockExp");
    lockCameraParams.setId(3);
    lockCameraParams.setPadding(50,50,50,50);
    lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
    lp.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);  
    preview.addView(lockCameraParams, lp);
    
    
    //create TextView
    
    debugging = new TextView(this);
    lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
    lp.addRule(RelativeLayout.ABOVE, 3);
    debugging.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);   
    preview.addView(debugging, lp);
    
    
    message = new TextView(this);
    lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);  
    lp.addRule(RelativeLayout.BELOW, 2);
    
    message.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
    
    
    preview.addView(message, lp);
    
    
    addListenerOnButton();
    
    context = getApplicationContext();

    // Attach a callback for preview
    camCallback = new CamCallback();
    mCamera.setPreviewCallback(camCallback);
        
    
/*    CharSequence text = "Hello toast!";
    int duration = Toast.LENGTH_LONG;

    Toast toast = Toast.makeText(context, text, duration);
    toast.show();*/
    
    handler = new Handler();
  }
  
  
 /* @Override
	public Object onRetainNonConfigurationInstance() {
		if (mAccessory != null) {
			return mAccessory;
		} else {
			return super.onRetainNonConfigurationInstance();
		}
	}*/
  
  @Override
	public void onResume() {
		super.onResume();
		
		/*mSensorManager.registerListener(this, mLight, SensorManager.SENSOR_DELAY_UI);

		if (mInputStream != null && mOutputStream != null) {
			return;
		}

		UsbAccessory[] accessories = mUsbManager.getAccessoryList();
		UsbAccessory accessory = (accessories == null ? null : accessories[0]);
		if (accessory != null) {
			if (mUsbManager.hasPermission(accessory)) {
				openAccessory(accessory);
			} else {
				synchronized (mUsbReceiver) {
					if (!mPermissionRequestPending) {
						mUsbManager.requestPermission(accessory,mPermissionIntent);
						mPermissionRequestPending = true;
					}
				}
			}
		} else {
			Log.d(TAG, "mAccessory is null");
		}*/
	}
    
  @Override
	public void onPause() {
		super.onPause();
		/*mSensorManager.unregisterListener(this);
		closeAccessory();*/
	}
  
  @Override
	public void onDestroy() {
		/*unregisterReceiver(mUsbReceiver);*/
		super.onDestroy();
	}
  
  
  
  public void addListenerOnButton() {
	   	 
  	processButton = (Button) findViewById(1);

  	processButton.setOnClickListener(new OnClickListener() {

		@Override
		public void onClick(View arg0) {
			
			if(!camCallback.enableProcess)
				camCallback.enableProcess = true;
			else
				camCallback.enableProcess = false;			            				 
		}
	});
  	
  	exposureCompButton = (Button) findViewById(2);
  	 
  	exposureCompButton.setOnClickListener(new OnClickListener() {

		@Override
		public void onClick(View arg0) {
			int expComp = -4;
			String isoValue = "100";
						
			/*parameters.set("min-exposure-compensation", expComp);
			parameters.setAutoExposureLock(true);
			parameters.setExposureCompensation(expComp);
			System.out.println("Exposure Compensation set to " + Integer.toString(expComp));
			*/
			//parameters.set("iso", isoValue);
			//System.out.println("Exposure Compensation set to " + isoValue);
			
			camPreview.mCamera.setParameters(parameters);
		}
	});

  	lockCameraParams = (Button) findViewById(3);
 	 
  	lockCameraParams.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View arg0) {
				
				camPreview.disableAntibanding();
				camPreview.disableVideoStabilization();
				camPreview.lockExposure();
				camPreview.lockWhiteBalance(); 
				camPreview.TurnOFFAutoFocus();
				
				//camPreview.setPreviewFPS(15, 15);
				camPreview.setPreviewFPS(24, 30);

				
				camPreview.printCameraParameters();
				
				//checkLight = 1;
				
				try {
        			Thread.sleep(2000);
        		} catch (InterruptedException e1) {
        			// TODO Auto-generated catch block
        			e1.printStackTrace();
        		} 
			}
		});
  }
  	

@Override
public final void onAccuracyChanged(Sensor sensor, int accuracy) {
	// Do something here if sensor accuracy changes.
}

@Override
public final void onSensorChanged(SensorEvent event) {
	// The light sensor returns a single value.
	// Many sensors return 3 values, one for each axis.
		
		System.out.println(event.values[0]);
		
		if(event.values[0] > 65){
			
			camCallback.mode = Mode.calculate_distance;
			camCallback.enableProcess = true;
			
			//checkLight = 0;
			
			if(initiator == 0){
				new java.util.Timer().schedule( 
				        new java.util.TimerTask() {
				            @Override
				            public void run() {
				            	//System.out.println("TurnON LED");
				            	//blinkLED();
				            	
				            	try {
				        			Thread.sleep(1500);
				        		} catch (InterruptedException e1) {
				        			// TODO Auto-generated catch block
				        			e1.printStackTrace();
				        		} 
				            	
				            	camCallback.mode = Mode.decode;
				    			camCallback.enableProcess = true;
				            	
				            	
				            }
				        }, 
				        1000
				);
			
			}
			else{
				new java.util.Timer().schedule( 
				        new java.util.TimerTask() {
				            @Override
				            public void run() {
				            	
				            	final String time = Long.toString(System.currentTimeMillis());
				            	MainActivity.handler.post(new Runnable() {
									public void run() {
										MainActivity.message.setText(time);
									}
								});

				            	//beginTransmission();
				            	
				            }
				        }, 
				        1000 
				);
				
				
			}
		}
		
		//sensorArray.add(new SensorValues(event.values[0], System.currentTimeMillis()));
	
	// Do something with this sensor value.
	//System.out.println(event.values[0]);
}
   
  
  
}