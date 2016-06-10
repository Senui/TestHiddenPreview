package com.example.testhiddenpreview;

import java.io.IOException;
import java.util.List;
import java.util.ListIterator;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.Parameters;
import android.util.Log;
import android.view.Gravity;
import android.view.TextureView;
import android.view.TextureView.SurfaceTextureListener;
import android.widget.FrameLayout;

public class CamPreview extends TextureView implements SurfaceTextureListener {

  public Camera mCamera;
  
  private static final String TAG = "CameraView";
  Context context;

  Parameters parameters;

  public CamPreview(Context context, Camera camera) {
    super(context);
    this.context = context;
    mCamera = camera;
   }

  @Override
  public void onSurfaceTextureAvailable(SurfaceTexture surface, int width, int height) {
    Camera.Size previewSize = mCamera.getParameters().getPreviewSize();
    //setLayoutParams(new FrameLayout.LayoutParams(1920, 1080, Gravity.CENTER));

	
    parameters = mCamera.getParameters();
	// Set the camera preview size
	parameters.setPreviewSize(1920, 1080);
	
  	mCamera.setParameters(parameters);
  	
  	mCamera.setDisplayOrientation(180);


    try{
      mCamera.setPreviewTexture(surface);
     } catch (IOException t) {}

    mCamera.startPreview();
    //this.setVisibility(INVISIBLE); // Make the surface invisible as soon as it is created
    this.setVisibility(VISIBLE);
  }
  
  
  
  public void takePicture(){
  	Camera.Parameters params = mCamera.getParameters();
  	
  	System.out.println(params.flatten());

  	params.setPictureSize(1600, 1200);

  	mCamera.setParameters(params);
  	
  	mCamera.takePicture(null, null, new PhotoHandler(context));
  	mCamera.startPreview();
  }

  @Override
  public void onSurfaceTextureSizeChanged(SurfaceTexture surface, int width, int height) {
      // Put code here to handle texture size change if you want to
  }

  @Override
  public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
    return true;
  }

  @Override
  public void onSurfaceTextureUpdated(SurfaceTexture surface) {
      // Update your view here!
  }
  
  
  
  public void setPreviewFPS(double min, double max){
      Camera.Parameters params = mCamera.getParameters();
      
      //find minimum fps 
      List<int[]> fpsRanges = params.getSupportedPreviewFpsRange();
      ListIterator<int[]> fpsItr = fpsRanges.listIterator();
      while(fpsItr.hasNext()) {
          int[] element = fpsItr.next();
          System.out.println("min="+ element[0] + " max="+ element[1]);
      }       
      
      params.setPreviewFpsRange((int)(min*1000), (int)(max*1000));
      mCamera.setParameters(params);
  }
  
  public void TurnOFFAutoFocus(){
  	    	
  	//Cancels any auto-focus function in progress. Whether or not auto-focus is currently in progress, 
  	//this function will return the focus position to the default.
  	mCamera.cancelAutoFocus();
  }
  
  public void setIsoValue(int value){
  	Camera.Parameters params = mCamera.getParameters();
  	
  	String isoValue = params.get("iso");
      Log.e(TAG, isoValue);
  	
  	params.set("iso", "800");
  	mCamera.setParameters(params);
  	
  	params = mCamera.getParameters();
  	isoValue = params.get("iso");
      Log.e(TAG, isoValue);

  }
  
  public void printCameraParameters(){
  	Camera.Parameters params = mCamera.getParameters();

  	/*int min = params.getMinExposureCompensation();
      int max = params.getMaxExposureCompensation();
      float exposureStep = params.getExposureCompensationStep();
      float focalLength = params.getFocalLength();
      int exposureCompensation = params.getExposureCompensation();
      System.out.println("ExposureComp min=" + min +" max=" + max + " step="+exposureStep);
      System.out.println("Focal Length="+focalLength);
      System.out.println("EC= "+exposureCompensation);
      
      //Log.e(TAG, params.flatten());
      Log.e(TAG, params.getAntibanding());*/
  	System.out.println("Jpeg Quality " + params.getJpegQuality());
      
  }
  
  public void setExposureCompensation(int value){
  	Camera.Parameters params = mCamera.getParameters();
      params.setExposureCompensation(value);
      mCamera.setParameters(params);     

  }
  
  public void lockExposure(){
  	Camera.Parameters params = mCamera.getParameters();
  	if(params.isAutoExposureLockSupported()){
      	params.setAutoExposureLock(true);
      }    	
      mCamera.setParameters(params);

  }
  
  public void disableAntibanding(){
  	Camera.Parameters params = mCamera.getParameters();
  	params.setAntibanding(Parameters.ANTIBANDING_OFF);
      mCamera.setParameters(params);
  }
  
  public void disableVideoStabilization(){
  	
  	Camera.Parameters params = mCamera.getParameters();
  	if(params.isVideoStabilizationSupported()){
      	params.setVideoStabilization(false);;
      }    	
      mCamera.setParameters(params);
  	
  }
  
  public void lockWhiteBalance(){
  	Camera.Parameters params = mCamera.getParameters();
  	if(params.isAutoWhiteBalanceLockSupported()){
      	params.setAutoWhiteBalanceLock(true);
      }    	
      mCamera.setParameters(params);

  }
  
  
  
  
}