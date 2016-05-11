package com.example.testhiddenpreview;

public class FrameData {
	
	public int number;
    public byte[] data;
    public long timestamp;
    
    public FrameData(byte[] data, int number, long timestamp){
    	
    	this.data = data;
    	this.number = number;
    	this.timestamp = timestamp;
    	
    }

}
