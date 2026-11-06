#pragma once

#include <array>
#include <cstdint>
#include <iosfwd>
#include <random>
#include <utility>
#include <vector>

struct Slot
{
	int x, y, z;
};

struct Tile
{
	Slot slot;
	int face;
	bool present;
};

struct GameStatistics
{
	std::uint64_t elapsedMilliseconds = 0;
	unsigned int hints = 0;
	unsigned int undos = 0;
	unsigned int reshuffles = 0;
	bool scoreRecorded = false;
};

class MahjongGame
{
public:
	static constexpr int TileCount = 144;
	static constexpr int FaceCount = 42;
	using Pair = std::pair<int, int>;

	explicit MahjongGame(unsigned int seed = std::random_device{}());
	bool NewGame();
	void Restart();
	bool Reshuffle();
	bool IsFree(int index) const;
	bool Remove(int first, int second);
	bool Undo();
	bool Hint(Pair& pair) const;
	int Remaining() const;
	const std::array<Tile, TileCount>& Tiles() const { return tiles_; }
	const std::vector<Pair>& DealOrder() const { return dealOrder_; }
	static bool Matches(int first, int second);
	bool Save(std::ostream& out, const GameStatistics& statistics) const;
	bool Load(std::istream& in, GameStatistics& statistics);

private:
	bool Deal(const std::vector<int>& faces);
	std::array<Tile, TileCount> tiles_;
	std::vector<Pair> history_;
	std::vector<Pair> dealOrder_;
	std::array<int, TileCount> initialFaces_ = {};
	std::vector<Pair> initialOrder_;
	std::mt19937 random_;
};
