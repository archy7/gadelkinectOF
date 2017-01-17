#pragma once

#include <cstdint>

#include <Kinect.h>

//#include "Point.h"

#include "ofMain.h"


template<typename T>
void SafeRelease(T& ptr) {
	if (ptr) {
		ptr->Release();
		ptr = nullptr;
	}
}


class ofApp : public ofBaseApp{

	public:
		ofApp();
		~ofApp();

		/*
			life time methods
		*/
		void setup();
		void update();
		void draw();

		/*
			openframework input methods
		*/
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		

		/*
			Kinect specific stuff
		*/
		bool scanArea();
		//void initPointBuffer(uint16_t width, uint16_t height); //for later
		void initPointBuffer();
		void printPointInfo();

		/*
			level specific methods
		*/
		void buildLevel();
		void buildFloor();
		void buildQuadIndices(uint32_t pointIndex);


	private:
		//Kinect Member
		IKinectSensor* m_kinect;
		IDepthFrameReader* m_depthFrameReader;

		uint16_t* m_depthBuffer;

		ofPoint* m_PointBuffer;

		int m_depthWidth = 0;
		int m_depthHeight = 0;


		//openframeworks members
		ofCamera camera;
		ofPlanePrimitive m_flatFloor;
		ofMesh m_floor;

		float playerSpeed = 10;

		const float DtoRfactor = 0.0174533;

		//Kinect Methods
		void initKinect();
		void initDepthFrameReader();

		//openframeworks Methods
		void moveCamera(uint32_t mousePosX, uint32_t mousePosY);
};