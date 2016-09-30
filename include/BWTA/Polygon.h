#pragma once
#include <BWAPI.h>
#include <vector>

namespace BWTA
{
	class Polygon : public std::vector < BWAPI::Position >
	{
	public:
		Polygon(); // TODO remove after fixing load_data
		Polygon(const BoostPolygon& boostPol, const int& scale = 1);
		Polygon(const Polygon& b);

		double getArea() const;
		double getPerimeter() const;
		BWAPI::Position getCenter() const;
		BWAPI::Position getNearestPoint(BWAPI::Position p) const;
		const std::vector<Polygon>& getHoles() const { return holes; };

		std::vector<Polygon> holes;
	};
}