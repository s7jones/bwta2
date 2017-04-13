#pragma once

struct ChkSection
{
	uint32_t name;
	uint32_t size;
};

struct ChkUnit
{
	uint32_t classInstance; // sort of a "serial number"
	uint16_t x; // coordinates are the center of the sprite of the unit (in pixels)
	uint16_t y;
	uint16_t ID;
	uint16_t relationType;
	// Bit 9 - Nydus Link
	// Bit 10 - Addon Link
	uint16_t specialFlags;
	// Bit 0 - Cloak is valid
	// Bit 1 - Burrow is valid
	// Bit 2 - In transit is valid
	// Bit 3 - Hallucinated is valid
	// Bit 4 - Invincible is valid
	// Bit 5-15 - Unused
	uint16_t propertyFlags;
	// Bit 0 - Owner player is valid (the unit is not a critter, start location, etc.; not a neutral unit)
	// Bit 1 - HP is valid
	// Bit 2 - Shields is valid
	// Bit 3 - Energy is valid (unit is a wraith, etc.)
	// Bit 4 - Resource amount is valid (unit is a mineral patch, vespene geyser, etc.)
	// Bit 5 - Amount in hangar is valid (unit is a reaver, carrier, etc.)
	// Bit 6-15 - Unused
	uint8_t playerID;  // Player number of owner (0-based)
	uint8_t HP; 
	uint8_t shield;
	uint8_t energy;
	uint32_t resourceAmount;
	uint16_t unitsInHangar;
	uint16_t stateFlags;
	// Bit 0 - Unit is cloaked
	// Bit 1 - Unit is burrowed
	// Bit 2 - Building is in transit
	// Bit 3 - Unit is hallucinated
	// Bit 4 - Unit is invincible
	// Bit 5-15 - Unused
	uint32_t unused;
	uint32_t relatedUnit; // Class instance of the unit to which this unit is related to (i.e. via an add-on, nydus link, etc.)
};

struct ChkDoodad
{
	uint16_t ID; // Unit/Sprite number of the sprite
	uint16_t x;
	uint16_t y;
	uint8_t playerID; // Player number that owns the doodad
	uint8_t unused;
	uint16_t flags;
	// Bit 0-11 - Unused
	// Bit 12 - Draw as sprite (Determines if it is a sprite or a unit sprite)
	// Bit 13 - Unused
	// Bit 14 - Unused
	// Bit 15 - Disabled (Only valid if Draw as sprite is unchecked, disables the unit)
};

// Standard Dimensions are 64, 96, 128, 192, and 256
struct ChkDim
{
	uint16_t width;
	uint16_t height;
};