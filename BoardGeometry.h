#pragma once

#include "MahjongGame.h"
#include <algorithm>

struct BoardRect
{
	float left, top, right, bottom;
};

class BoardGeometry
{
public:
	// The supplied PNGs contain a face plus left and bottom sides.
	static constexpr float FaceLeft = 78.0f / 618.0f;
	static constexpr float FaceTop = 8.0f / 859.0f;
	static constexpr float FaceRight = 612.0f / 618.0f;
	static constexpr float FaceBottom = 780.0f / 859.0f;

	BoardGeometry(float width, float height)
		: width_(width), height_(height),
		scale_((std::min)(width / 780.0f, height / 580.0f))
	{
	}

	float Scale() const { return scale_; }

	BoardRect TileRect(const Slot& slot) const
	{
		const float tileWidth = 50.0f * scale_;
		const float tileHeight = 69.5f * scale_;
		const float left = (width_ - 15.0f * tileWidth) / 2.0f +
			slot.x * tileWidth / 2.0f + slot.z * 8.0f * scale_;
		const float top = (height_ - 8.0f * tileHeight) / 2.0f +
			slot.y * tileHeight / 2.0f - slot.z * 8.0f * scale_;
		return { left, top, left + tileWidth, top + tileHeight };
	}

	BoardRect TileImageRect(const Slot& slot) const
	{
		const BoardRect face = TileRect(slot);
		const float width = (face.right - face.left) / (FaceRight - FaceLeft);
		const float height = (face.bottom - face.top) / (FaceBottom - FaceTop);
		const float left = face.left - width * FaceLeft;
		const float top = face.top - height * FaceTop;
		return { left, top, left + width, top + height };
	}

	int HitTest(const std::array<Tile, MahjongGame::TileCount>& tiles, float x, float y) const
	{
		// Faces are painted over every side in their layer.
		for (int z = 4; z >= 0; --z)
			for (int part = 0; part < 2; ++part)
				for (int i = MahjongGame::TileCount - 1; i >= 0; --i)
				{
					const Tile& tile = tiles[i];
					if (!tile.present || tile.slot.z != z)
						continue;
					const BoardRect rect = part == 0 ? TileRect(tile.slot) : TileImageRect(tile.slot);
					if (x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom)
						return i;
				}
		return -1;
	}

private:
	float width_, height_, scale_;
};
