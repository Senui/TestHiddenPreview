package com.example.testhiddenpreview;

//separate thread implementation
class LightChecker implements Runnable {
	long minPrime;

	private static final String TAG = "LightChecker Thread";

	LightChecker() {
	}

	public void run() {

		// android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_FOREGROUND);

		long startTime = System.nanoTime();

		int result = lightCheck(1920, 1080, CamCallback.data);
		
		if(result == 1)
			CamCallback.lightOn = 1;

	}
	

	static {
		System.loadLibrary("jni_part");
	}

	public native int lightCheck(int width, int height,	byte[] NV21FrameData);
}

