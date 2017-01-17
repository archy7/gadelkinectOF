#pragma once

#include <cstdint>
#include <assert.h>

#include "ofPoint.h"

enum borderTypes { NO_BORDER, TOP_BORDER, LEFT_BORDER, BOTTOM_BORDER, RIGHT_BORDER, 
					TOP_LEFT_CORNER, BOTTOM_LEFT_CORNER, TOP_RIGHT_CORNER, BOTTOM_RIGHT_CORNER };

class PointRefactorer {
private:

	ofPoint* m_pointArray;

	const size_t m_depthWidth;
	const size_t m_depthHeight;
	const size_t m_arraySize;

	
	
	bool m_pointsCorrected;

	inline void bilinearAdjustment(ofPoint* pointArray, uint32_t pointIndex);
	inline void linearAdjustment(ofPoint* pointArray, uint32_t pointIndex);

	inline void correctRow(size_t rowIndex);
	inline void correctCol(size_t colIndex);

	inline void extrapolateLeft(uint16_t pointIndex, uint16_t extrapolationSize, float factor);
	inline void extrapolateRight(uint16_t pointIndex, uint16_t extrapolationSize, float factor);


	inline borderTypes findBorderType(uint32_t pointIndex);
public:
	PointRefactorer(ofPoint* pointArray, size_t depthWidth, size_t depthHeight) :
		m_pointArray(pointArray),
		m_depthWidth(depthWidth),
		m_depthHeight(depthHeight),
		m_arraySize(depthWidth * m_depthHeight)
	{
		assert(pointArray != nullptr);
	}

	~PointRefactorer() {
		//delete pointArray = Everything goes to hell
		//dont do it
	}

	void rescalePoints(float scale);
	void smoothPoints();
	void correctPoints();
};