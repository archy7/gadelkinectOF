#include "ofApp.h"

#include <cstdint>
#include <Kinect.h>
#include <iostream>
#include <assert.h>

//#include "Point.h"

//--------------------------------------------------------------
void ofApp::setup(){

	ofSetFrameRate(60);
	ofSetVerticalSync(false);
	ofSetBackgroundColor(0, 0, 0);
	ofEnableDepthTest();
	ofDisableArbTex();

	initKinect();
	initDepthFrameReader();
	
	{
		bool scanSuccess = false;
		while (!scanSuccess) {
			scanSuccess = scanArea();
		}
	}
	
	initPointBuffer();
	printPointInfo();

	buildLevel();
}

//--------------------------------------------------------------
void ofApp::update(){

}

//--------------------------------------------------------------
void ofApp::draw(){

	ofColor blue(0, 0, 255);
	ofColor red(255, 0, 0);

	camera.begin();

	ofSetColor(blue);
	m_flatFloor.draw();

	ofSetColor(red);
	m_floor.draw();

	camera.end();

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch (key) {
	case 'w': {
		//move forward == -z
		ofVec3f cameraPos = camera.getPosition();
		cameraPos.z = cameraPos.z - 1 * playerSpeed;
		camera.setPosition(cameraPos);
		break;
	}
	case 'a': {
		//move left == -x
		ofVec3f cameraPos = camera.getPosition();
		cameraPos.x = cameraPos.x - 1 * playerSpeed;
		camera.setPosition(cameraPos);
		break;
	}
	case 's': {
		//move back == +z
		ofVec3f cameraPos = camera.getPosition();
		cameraPos.z = cameraPos.z + 1 * playerSpeed;
		camera.setPosition(cameraPos);
		break;
	}
	case 'd': {
		//move right == +x
		ofVec3f cameraPos = camera.getPosition();
		cameraPos.x = cameraPos.x + 1 * playerSpeed;
		camera.setPosition(cameraPos);
		break;
	}
	case ' ': {
		//move up == +y
		ofVec3f cameraPos = camera.getPosition();
		cameraPos.y = cameraPos.y + 1 * playerSpeed;
		camera.setPosition(cameraPos);
		break;
	}
	case 'y': {
		//move down == -y
		ofVec3f cameraPos = camera.getPosition();
		cameraPos.y = cameraPos.y - 1 * playerSpeed;
		camera.setPosition(cameraPos);
		break;
	}
	default: {
		//nothing

	}
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
	uint32_t mousePosX = x;
	uint32_t mousePosY = y;

	moveCamera(x,y);
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}


//////////////////////////////////////////////////////////////77
////////////////////////////////////////////////////////////////77

ofApp::ofApp() :
	m_kinect(nullptr),
	m_depthFrameReader(nullptr),
	m_depthBuffer(nullptr),
	m_depthWidth(0),
	m_depthHeight(0)
{
}

ofApp::~ofApp() {
	/*
	delete m_depthFrameReader;
	delete m_kinect;
	*/
	SafeRelease(m_depthFrameReader);
	SafeRelease(m_kinect);

	delete[] m_depthBuffer;
	delete[] m_PointBuffer;
}

void ofApp::initKinect() {
	HRESULT hr = GetDefaultKinectSensor(&m_kinect);

	if (FAILED(hr)) {
		std::cerr << "Failed to connect to the Kinect sensor. Shutting down." << std::endl;
		::exit(1);
	}

	m_kinect->Open();
}

void ofApp::initDepthFrameReader() {
	IDepthFrameSource* depthFrameSource;

	HRESULT hr = m_kinect->get_DepthFrameSource(&depthFrameSource);
	if (FAILED(hr)) {
		std::cerr << "Failed to find depth frame source. Shutting down." << std::endl;
		::exit(2);
	}

	hr = depthFrameSource->OpenReader(&m_depthFrameReader);
	if (FAILED(hr)) {
		std::cerr << "Failed to open the Reader. Shutting down." << std::endl;
		::exit(2);
	}

	// Retrieved Depth Frame Size
	IFrameDescription* pDepthDescription;
	hr = depthFrameSource->get_FrameDescription(&pDepthDescription);
	if (FAILED(hr)) {
		std::cerr << "Error : IDepthFrameSource::get_FrameDescription()" << std::endl;
		return;
	}

	pDepthDescription->get_Width(&m_depthWidth); // 512
	pDepthDescription->get_Height(&m_depthHeight); // 424


	m_depthBuffer = new uint16_t[m_depthWidth * m_depthHeight];

	SafeRelease(depthFrameSource);
}

bool ofApp::scanArea() {
	std::cout << "Scanning the area." << std::endl;
	HRESULT hr;

	IDepthFrame* depthFrame;
	hr = m_depthFrameReader->AcquireLatestFrame(&depthFrame);
	if (FAILED(hr)) {
		std::cerr << "Failed to scan the Area." << std::endl;
		return false;
	}

	//move information into depthbuffer of uint16_t. the value of each uint16_t represents the distance of the scanned """pixel""" to the Kinect, therefore 0 - 65535.
	//The Kinect has problems scanning points that are closer than ~0.5 meters
	hr = depthFrame->CopyFrameDataToArray(m_depthWidth * m_depthHeight, m_depthBuffer);
	if (FAILED(hr)) {
		std::cerr << "Failed to scan the Area. (copy of pixels)" << std::endl;
		return false;
	}

	SafeRelease(depthFrame);

	//The image/scan is now in the buffer
	//The depth buffer will under almost all circumstances be of size 512 * 424 (width * height)
	//For the depth buffer not to be of this size, the physical Kinect Sensor would have to be a different one
	/*
	Taking this into consideration, we can come the the following conclusion.
	Creating a point cloud, of which each point has its own XYZ coordinates, we can access them with width|height|depth as 3D coordinates.
	If the scanned image is to be layed down as a vertical "plane" in a right handed euclidic space(look down the negative Z-axis), we would map the coordinates in such a way:
	+X = width
	-Y = depth
	-Z = height
	This would result in a 512(w) * 424(h) points with a certain depth as their unique coordinates.
	For more points to be displayed, interpolation will be required to generate points
	*/

	std::cout << "Scan complete." << std::endl;
	return true;
}


void ofApp::initPointBuffer() {

	std::cout << "init point buffer." << std::endl;
	m_PointBuffer = new ofPoint[m_depthWidth * m_depthHeight]; //there must be some optimization to this

	/*for (uint16_t heightIndex = 0u; heightIndex < m_depthHeight; ++heightIndex) {
		for (uint16_t widthIndex = 0u; heightIndex < m_depthWidth; ++widthIndex) {
			Point& point = m_PointBuffer[heightIndex * m_depthHeight + widthIndex];
			//Point* point = m_PointBuffer+(heightIndex * m_depthHeight + widthIndex);
			point.x = widthIndex;
			point.y = static_cast<float>(m_depthBuffer[heightIndex * m_depthHeight + widthIndex]) *(-1);
			point.z = heightIndex *(-1);
		}
	}*/
	int16_t heightIndex = 0u;
	int16_t widthIndex = 0u;
	const uint32_t maxIndex = m_depthWidth * m_depthHeight -1; //0 counts too
	for (uint32_t pointBufferIndex = 0u; pointBufferIndex < maxIndex; ++pointBufferIndex) {

		ofPoint& point = m_PointBuffer[pointBufferIndex];
		point.x = static_cast<float>(widthIndex);
		point.y = static_cast<float>(m_depthBuffer[pointBufferIndex]) * (-1);
		point.z = static_cast<float>(heightIndex) * (-1);

		++widthIndex;
		if (widthIndex == m_depthWidth) {
			++heightIndex;
			widthIndex = 0;
		}
	}

	std::cout << "point buffer initialized" << std::endl;

	/*
	1.
	Was ist besser?
	ein solcher Zugriff:
	... = m_PointBuffer[heightIndex][widthIndex];

	oder der verwendete?

	2.
	was ist besser?
	Point& point = m_PointBuffer[heightIndex * m_depthHeight + widthIndex]; (verwendet)
	point.x = widthIndex;
	...

	oder:

	Point* point = m_PointBuffer+(heightIndex * m_depthHeight + widthIndex);
	point->x = widthIndex;

	*/
}

void ofApp::printPointInfo() {
	std::cout << "printing point info" << std::endl;

	int16_t heightIndex = 0u;
	int16_t widthIndex = 0u;
	const uint32_t maxIndex = m_depthWidth * m_depthHeight - 1; //0 counts too
	for (uint32_t pointBufferIndex = 0u; pointBufferIndex < 10; ++pointBufferIndex) {
		ofPoint& point = m_PointBuffer[pointBufferIndex];
		std::cout << "Point at position: x[" << point.x << "],y[" << point.y << "],z[" << point.z << "]." << std::endl;


		++widthIndex;
		if (widthIndex == m_depthWidth) {
			++heightIndex;
			widthIndex = 0;
		}
	}
}

void ofApp::buildLevel() {
	m_flatFloor.setHeight(m_depthHeight);
	m_flatFloor.setWidth(m_depthWidth);

	m_flatFloor.setPosition(0,0,0);
	m_flatFloor.rotate(90, 1, 0, 0);

	buildFloor();

	camera.setPosition(0, 0, 0);
}

void ofApp::buildQuadIndices(uint32_t pointIndex) {
	/*
		+----+
		|  / |
		| /  |
		|/   |
		+----+

		The top left vertex (point) is passed to this function by Index in the pointBuffer Array.
		We call it x.
		We now have two routines to find the other 3 vertices of this "quad". (It really is 2 triangles)
		
		Routine 1: the upper left triangle
			The upper left vertex is given (x).
			The upper right vertex is gotten by calculating x + 1.
			The lower left vertex is gotten by calculating x + m_depthWidth (the size of the width of the original depthbuffer).

		Routine 2: the lower right triangle
			The upper right vertex is gotten by calculating x + 1.
			The lower left vertex is gotten by calculating x + m_depthWidth.
			The lower right vertex is gotten by calculating x + m_depthWidth + 1.
	
		It is of importance to note, that the lower and far right borders of the depth buffer shall not be handled, 
		since calculations on these points would lead to really undesired results. (They would no make sense at all).
	*/

	if (((pointIndex+1) % m_depthWidth) == 0) { //äußerer rechter Rand welcher nicht bearbeitet werden soll
		return;
	}

	if (pointIndex >= (m_depthWidth*(m_depthHeight - 1))) { //unterer Rand welcher auch nicht bearbeitet werden soll
		return;
	}

	//Routine 1
	//upper left
	m_floor.addIndex(pointIndex);

	//upper right
	m_floor.addIndex(pointIndex + 1);

	//lower left
	m_floor.addIndex(pointIndex + m_depthWidth);

	//Routine 2
	//upper lright
	m_floor.addIndex(pointIndex + 1);

	//lower left
	m_floor.addIndex(pointIndex + m_depthWidth);

	//lower right
	m_floor.addIndex(pointIndex + m_depthWidth + 1);

	//quad done
}

void ofApp::buildFloor() {

	int16_t heightIndex = 0u;
	int16_t widthIndex = 0u;
	const uint32_t maxIndex = m_depthWidth * m_depthHeight - 1; //0 counts too

	m_floor.setMode(OF_PRIMITIVE_TRIANGLES);
	for (uint32_t pointBufferIndex = 0u; pointBufferIndex < maxIndex; ++pointBufferIndex) {
		ofPoint& point = m_PointBuffer[pointBufferIndex];

		m_floor.addVertex(point);
		//addColor or do other stuff

		++widthIndex;
		if (widthIndex == m_depthWidth) {
			++heightIndex;
			widthIndex = 0;
		}

		
	}

	heightIndex = 0u;
	widthIndex = 0u;
	for (uint32_t pointBufferIndex = 0u; pointBufferIndex < maxIndex; ++pointBufferIndex) {

		buildQuadIndices(pointBufferIndex);

		++widthIndex;
		if (widthIndex == m_depthWidth) {
			++heightIndex;
			widthIndex = 0;
		}

	}
}

void ofApp::moveCamera(uint32_t mousePosX, uint32_t mousePosY) {
	int windowW = ofGetWindowWidth();
	int windowH = ofGetWindowHeight();
	double pixelPerDegreeX = windowW / (double)360; //PASST
	double pixelPerDegreeY = windowH / (double)180;

	float cameraAngleX = mousePosX / pixelPerDegreeX; //PASST
	float cameraAngleY = mousePosY / pixelPerDegreeY;

	float radiansOfDegreesX = cameraAngleX * DtoRfactor;
	float radiansOfDegreesY = cameraAngleY * DtoRfactor;

	float sinResult = sin(radiansOfDegreesX); //<- these functions expect radians and not degrees, i had to learn this the hard way
	float cosResult = cos(radiansOfDegreesX);

	//cotinue here for possible Y position changes, for now left alone

	camera.rotate(10, 0, 1, 0);

}