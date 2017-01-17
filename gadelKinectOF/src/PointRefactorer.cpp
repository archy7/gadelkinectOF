#include "PointRefactorer.h"

#include <cmath> //std::abs
#include <vector> 

/*
	correct depth values of 0 --and adjusts too hight values ("Ausreiﬂer"), which are values that are 50% bigger than the current average 
*/
void PointRefactorer::correctPoints() {

	m_pointsCorrected = false;

	while (m_pointsCorrected == false) {
		m_pointsCorrected = true;
		for (size_t heightIndex = 0u; heightIndex < m_depthHeight; ++heightIndex) { //each row
			correctRow(heightIndex);
		}

		for (size_t widthIndex = 0u; widthIndex < m_depthWidth; ++widthIndex) { //each col
			correctCol(widthIndex);
		}
	}
	
}


/*
	smooths out Points so a 
*/
void PointRefactorer::smoothPoints() {

}

/*
	resource heavy function that takes the given pointArray and rescales in by a certain scale
*/
void PointRefactorer::rescalePoints(float scale) {

}

inline void PointRefactorer::correctRow(size_t rowIndex) {
	const uint16_t startIndex = rowIndex * m_depthWidth;
	const uint16_t endIndex = ((rowIndex+1) * m_depthWidth) -1;
	
	bool doLeft = false;
	bool doRight = false;
	//check if the left border is incorrect
	if (m_pointArray[startIndex].y == -0) {
		doLeft = true;
	}

	//try to interpolate every point between the left and right border
	for (uint16_t pointIndex = startIndex+1; pointIndex < endIndex; ++pointIndex) {
		
		bool leftInterpolationPointFound = false;
		bool rightInterpolationPointFound = false;
		if (m_pointArray[pointIndex].y == -0) {//point must be fixed
			//find last previous usable point
			uint16_t stepsToPreviousPoint = 1u; //under almost all logical circumstances, this should be 1, considering how we iterate over the array "from left to right"
			uint16_t stepsToNextPoint = 1u;
			
			for (; (pointIndex - stepsToPreviousPoint) > startIndex; ++stepsToPreviousPoint) {
				if (m_pointArray[pointIndex - stepsToPreviousPoint].y == -0) {  //interpolation value not found yet
					continue;
				}
				else {
					leftInterpolationPointFound = true;
					break;
				}

			}
			if (!leftInterpolationPointFound) { //skip this point if there is no chance of interpolating
				m_pointsCorrected = false;
				continue;
			}

			//find the next usable point
			for (; (pointIndex + stepsToNextPoint) < endIndex; ++stepsToNextPoint) {
				if (m_pointArray[pointIndex + stepsToPreviousPoint].y == -0) { //interpolation value not found yet
					continue;
				}
				else {
					rightInterpolationPointFound = true;
					break;
				}

			}
			if (!rightInterpolationPointFound) { //skip this point if there is no chance of interpolating
				m_pointsCorrected = false;
				continue;
			}

			//if the code reaches this point, interpolation must be possible
			//it is also important to mention, that the -0 values, which were skipped earlier are now also corrected.
			//which means that some coming -0 values are now also fixed
			//under almost all logical circumstances, previous points should not be touched
			uint16_t discreteValues = stepsToPreviousPoint + stepsToNextPoint;

			float leftInterpolationValue = m_pointArray[pointIndex - stepsToPreviousPoint].y;
			float rightInterpolationValue = m_pointArray[pointIndex + stepsToNextPoint].y;

			float difference = leftInterpolationValue - rightInterpolationValue;

			float interpolationSteps = difference / static_cast<float>(discreteValues);

			uint16_t factor = 1;

			for (size_t correctedIndex = pointIndex - stepsToPreviousPoint + 1; correctedIndex < pointIndex + stepsToNextPoint; ++correctedIndex) {
				m_pointArray[correctedIndex].y = interpolationSteps * factor;
				factor++;
			}

		}
	}

	//check if the right border is incorrect
	if (m_pointArray[endIndex].y == -0) {
		doRight = true;
	}

	//try to: extrapolate left, extrapolate right
	if (doLeft) {
		uint16_t pointIndex = startIndex;
		extrapolateLeft(pointIndex, 5, 0.5);
	}
	if (doRight) {
		uint16_t pointIndex = endIndex;
		extrapolateRight(pointIndex, 5, 0.5);
	}

	//finally done
}

/*
	recursive function that tries to extrapolate as many points as possible
	starting at @param pointIndex, then incremenenting by 1 (aka going "right") by @param extrapolationSize number of times
	@param Factor determines how much weight the closer points have
	The function fails if the point does not have to extrapolated or if there is nothing to extrapolate with
*/
inline void PointRefactorer::extrapolateLeft(uint16_t pointIndex, uint16_t extrapolationSize, float factor) {

	assert("PointRefactorer::extrapolateLeft()::Factor must be in interval ]0,1]" && factor > 0 && factor <= 1);

	const uint16_t extrapolationEndIndex = pointIndex + extrapolationSize;
	//try to extrapolate as many points as possible
	//therefore, find out how far <direction of search> one has to go, until there is a correct value
	uint16_t recursionDepth = 0u;
	for (uint16_t i = pointIndex; i < extrapolationEndIndex; ++i) {
		if (m_pointArray[i].y != -0) {
			break;
		}
		else {
			++recursionDepth;
		}
	}

	if (recursionDepth == 0u) {	//the point which shall be extrapolated is not to be extrapolated OK
		return;
	}

	if (recursionDepth == extrapolationEndIndex) {	//there is absolutely nothing to extrapolate with FAIL
		m_pointsCorrected = false;
		return;
	}

	extrapolateLeft(pointIndex+recursionDepth, extrapolationSize, factor);
	//find out if and how many points are actually usable for extrapolation
	//just if someone wishes to extrapolate with 20 points does not mean that that is possible too
	//however, under most circumstances, this will be the desired size, since points were previously interpolated
	//also start gathering the values for calculation
	std::vector<float> depthValues;
	uint16_t possibleExtrapolationSize = 0u;
	for (uint16_t i = pointIndex; i < extrapolationEndIndex; ++i) {
		if (m_pointArray[i].y != -0) {
			++possibleExtrapolationSize;
			depthValues.push_back(m_pointArray[i].y);
		}
		else {
			break;
		}
	}
	
	//usable points are now determined
	float result = 0;
	for (float depthValue : depthValues) {
		result += depthValue * factor;
		factor *= factor;
	}

	m_pointArray[pointIndex].y = result;
}

/*
recursive function that tries to extrapolate as many points as possible
starting at @param pointIndex, then decremenenting by 1 (aka going "left") by @param extrapolationSize number of times
@param Factor determines how much weight the closer points have
The function fails if the point does not have to extrapolated or if there is nothing to extrapolate with
*/
inline void PointRefactorer::extrapolateRight(uint16_t pointIndex, uint16_t extrapolationSize, float factor) {

	assert("PointRefactorer::extrapolateRight()::Factor must be in interval ]0,1]" && factor > 0 && factor <= 1);

	const uint16_t extrapolationEndIndex = pointIndex - extrapolationSize;
	//try to extrapolate as many points as possible
	//therefore, find out how far <direction of search> one has to go, until there is a correct value
	uint16_t recursionDepth = 0u;
	for (uint16_t i = pointIndex; i > extrapolationEndIndex; --i) {
		if (m_pointArray[i].y != -0) {
			break;
		}
		else {
			++recursionDepth;
		}
	}

	if (recursionDepth == 0u) {	//the point which shall be extrapolated is not to be extrapolated OK
		return;
	}

	if (recursionDepth == extrapolationEndIndex) {	//there is absolutely nothing to extrapolate with FAIL
		m_pointsCorrected = false;
		return;
	}

	extrapolateRight(pointIndex - recursionDepth, extrapolationSize, factor);
	//find out if and how many points are actually usable for extrapolation
	//just if someone wishes to extrapolate with 20 points does not mean that that is possible too
	//however, under most circumstances, this will be the desired size, since points were previously interpolated
	//also start gathering the values for calculation
	
	std::vector<float> depthValues;
	uint16_t possibleExtrapolationSize = 0u;
	for (uint16_t i = pointIndex; i > extrapolationEndIndex; --i) {
		if (m_pointArray[i].y != -0) {
			++possibleExtrapolationSize;
			depthValues.push_back(m_pointArray[i].y);
		}
		else {
			break;
		}
	}

	//usable points are now determined
	float result = 0;
	for (float depthValue : depthValues) {
		result += depthValue * factor;
		factor *= factor;
	}

	m_pointArray[pointIndex].y = result;
}

inline void PointRefactorer::correctCol(size_t colIndex) {
	//TODO
	const uint16_t startIndex = rowIndex * m_depthWidth;
	const uint16_t endIndex = ((rowIndex + 1) * m_depthWidth) - 1;

	bool doLeft = false;
	bool doRight = false;
	//check if the left border is incorrect
	if (m_pointArray[startIndex].y == -0) {
		doLeft = true;
	}

	//try to interpolate every point between the left and right border
	for (uint16_t pointIndex = startIndex + 1; pointIndex < endIndex; ++pointIndex) {

		bool leftInterpolationPointFound = false;
		bool rightInterpolationPointFound = false;
		if (m_pointArray[pointIndex].y == -0) {//point must be fixed
											   //find last previous usable point
			uint16_t stepsToPreviousPoint = 1u; //under almost all logical circumstances, this should be 1, considering how we iterate over the array "from left to right"
			uint16_t stepsToNextPoint = 1u;

			for (; (pointIndex - stepsToPreviousPoint) > startIndex; ++stepsToPreviousPoint) {
				if (m_pointArray[pointIndex - stepsToPreviousPoint].y == -0) {  //interpolation value not found yet
					continue;
				}
				else {
					leftInterpolationPointFound = true;
					break;
				}

			}
			if (!leftInterpolationPointFound) { //skip this point if there is no chance of interpolating
				m_pointsCorrected = false;
				continue;
			}

			//find the next usable point
			for (; (pointIndex + stepsToNextPoint) < endIndex; ++stepsToNextPoint) {
				if (m_pointArray[pointIndex + stepsToPreviousPoint].y == -0) { //interpolation value not found yet
					continue;
				}
				else {
					rightInterpolationPointFound = true;
					break;
				}

			}
			if (!rightInterpolationPointFound) { //skip this point if there is no chance of interpolating
				m_pointsCorrected = false;
				continue;
			}

			//if the code reaches this point, interpolation must be possible
			//it is also important to mention, that the -0 values, which were skipped earlier are now also corrected.
			//which means that some coming -0 values are now also fixed
			//under almost all logical circumstances, previous points should not be touched
			uint16_t discreteValues = stepsToPreviousPoint + stepsToNextPoint;

			float leftInterpolationValue = m_pointArray[pointIndex - stepsToPreviousPoint].y;
			float rightInterpolationValue = m_pointArray[pointIndex + stepsToNextPoint].y;

			float difference = leftInterpolationValue - rightInterpolationValue;

			float interpolationSteps = difference / static_cast<float>(discreteValues);

			uint16_t factor = 1;

			for (size_t correctedIndex = pointIndex - stepsToPreviousPoint + 1; correctedIndex < pointIndex + stepsToNextPoint; ++correctedIndex) {
				m_pointArray[correctedIndex].y = interpolationSteps * factor;
				factor++;
			}

		}
	}

	//check if the right border is incorrect
	if (m_pointArray[endIndex].y == -0) {
		doRight = true;
	}

	//try to: extrapolate left, extrapolate right
	if (doLeft) {
		uint16_t pointIndex = startIndex;
		extrapolateLeft(pointIndex, 5, 0.5);
	}
	if (doRight) {
		uint16_t pointIndex = endIndex;
		extrapolateRight(pointIndex, 5, 0.5);
	}

	//finally done
}








































inline borderTypes PointRefactorer::findBorderType(uint32_t pointIndex) {

	//Left edge
	if ((pointIndex % m_depthWidth)==0) {
		return borderTypes::LEFT_BORDER;
	}
	if ((pointIndex % m_depthWidth) == (pointIndex - 1)) {
		return borderTypes::RIGHT_BORDER;
	}
	if (pointIndex < m_depthWidth) {
		return borderTypes::TOP_BORDER;
	}
	if (pointIndex >= (m_depthWidth +(m_depthHeight-1))) {
		return borderTypes::BOTTOM_BORDER;
	}
	if () {

	}
	
	return borderTypes::NO_BORDER;
}

inline void PointRefactorer::bilinearAdjustment(ofPoint* pointArray, uint32_t pointIndex) {

}
/*
	Vertical Borders are adjusted by linear interpolation of points on the vertical axis
*/
inline void PointRefactorer::linearAdjustment(ofPoint* pointArray, uint32_t pointIndex) {
	borderTypes typeofthispoint = findBorderType(pointIndex);
	switch (typeofthispoint) {
	case borderTypes::LEFT:
		//cascade		
	case borderTypes::RIGHT: {
		//care for edge cases (corners)
		if()
		uint16_t offSetNextNonZeroDepth = 0u;

		break;
	}
	case borderTypes::NONE: 

		//cascade
	default:

		break;
	}
}
