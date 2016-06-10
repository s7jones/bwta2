#pragma once

#include "functions.h"

namespace BWTA
{
	class ConnectedComponent
	{
	public:
		ConnectedComponent(int id, bool walkable = true)
			: _id(id)
			, _walkability(walkable)
			, _topLeftTile(10000, 10000)
		{};

		const int getID() const { return _id; };

		const bool isWalkable() const { return _walkability; };
		void setWalkability(bool walkability) { _walkability = walkability; };
		
		const Point top_left() const { return _topLeftTile; };
		void set_top_left(Point topLeftTile) { _topLeftTile = topLeftTile; };

	private:
		int _id;
		bool _walkability;
		Point _topLeftTile;
	};
}