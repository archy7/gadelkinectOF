#pragma once

class Point {
public:
	float x;
	float y;
	float z;

	Point() :
		x(0),
		y(0),
		z(0)
	{

	};

	Point(float x, float y, float z) :
		x(x),
		y(y),
		z(z)
	{

	};

	Point(int x, float y, int z) :
		x(static_cast<float>(x)),
		y(y),
		z(static_cast<float>(z))
	{

	};

	Point(int x, int y, int z) :
		x(static_cast<float>(x)),
		y(static_cast<float>(y)),
		z(static_cast<float>(z))
	{

	};
};