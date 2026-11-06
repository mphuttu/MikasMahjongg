#include "MahjongGame.h"

#include <algorithm>
#include <cmath>
#include <istream>
#include <numeric>
#include <ostream>
#include <string>

MahjongGame::MahjongGame(unsigned int seed) : random_(seed)
{
	int index = 0;
	const int starts[] = { 2, 6, 4, 2, 2, 4, 6, 2 };
	const int ends[] = { 24, 20, 22, 24, 24, 22, 20, 24 };
	for (int row = 0; row < 8; ++row)
		for (int x = starts[row]; x <= ends[row]; x += 2)
			tiles_[index++] = { { x, row * 2, 0 }, 0, true };
	tiles_[index++] = { { 0, 7, 0 }, 0, true };
	tiles_[index++] = { { 26, 7, 0 }, 0, true };
	tiles_[index++] = { { 28, 7, 0 }, 0, true };
	for (int z = 1; z <= 3; ++z)
	{
		const int startX = 6 + 2 * z, endX = 20 - 2 * z;
		const int startY = 2 * z, endY = 14 - 2 * z;
		for (int y = startY; y <= endY; y += 2)
			for (int x = startX; x <= endX; x += 2)
				tiles_[index++] = { { x, y, z }, 0, true };
	}
	tiles_[index++] = { { 13, 7, 4 }, 0, true };
}

bool MahjongGame::Matches(int first, int second)
{
	return first == second ||
		(first >= 34 && first < 38 && second >= 34 && second < 38) ||
		(first >= 38 && first < 42 && second >= 38 && second < 42);
}

bool MahjongGame::IsFree(int index) const
{
	if (index < 0 || index >= TileCount || !tiles_[index].present)
		return false;
	const Slot& slot = tiles_[index].slot;
	bool left = false, right = false;
	for (int i = 0; i < TileCount; ++i)
	{
		if (i == index || !tiles_[i].present)
			continue;
		const Slot& other = tiles_[i].slot;
		const int dx = other.x - slot.x, dy = other.y - slot.y;
		if (other.z > slot.z && std::abs(dx) < 2 && std::abs(dy) < 2)
			return false;
		if (other.z == slot.z && std::abs(dy) < 2)
		{
			if (dx == -2) left = true;
			if (dx == 2) right = true;
		}
	}
	return !left || !right;
}

bool MahjongGame::NewGame()
{
	const auto oldTiles = tiles_;
	for (auto& tile : tiles_)
		tile.present = true;
	std::vector<int> faces;
	for (int face = 0; face < 34; ++face)
		for (int i = 0; i < 4; ++i)
			faces.push_back(face);
	for (int face = 34; face < FaceCount; ++face)
		faces.push_back(face);
	if (!Deal(faces))
	{
		tiles_ = oldTiles;
		return false;
	}
	history_.clear();
	for (int i = 0; i < TileCount; ++i)
		initialFaces_[i] = tiles_[i].face;
	initialOrder_ = dealOrder_;
	return true;
}

void MahjongGame::Restart()
{
	for (int i = 0; i < TileCount; ++i)
	{
		tiles_[i].face = initialFaces_[i];
		tiles_[i].present = true;
	}
	history_.clear();
	dealOrder_ = initialOrder_;
}

bool MahjongGame::Reshuffle()
{
	std::vector<int> faces;
	for (const auto& tile : tiles_)
		if (tile.present)
			faces.push_back(tile.face);
	if (!Deal(faces))
		return false;
	history_.clear();
	return true;
}

bool MahjongGame::Deal(const std::vector<int>& faces)
{
	std::vector<std::pair<int, int>> facePairs;
	std::vector<int> remaining = faces;
	std::shuffle(remaining.begin(), remaining.end(), random_);
	while (!remaining.empty())
	{
		const int face = remaining.back();
		remaining.pop_back();
		const auto match = std::find_if(remaining.begin(), remaining.end(),
			[face](int other) { return Matches(face, other); });
		if (match == remaining.end())
			return false;
		facePairs.emplace_back(face, *match);
		remaining.erase(match);
	}

	const auto original = tiles_;
	std::vector<Pair> order;
	for (int attempt = 0; attempt < 2000; ++attempt)
	{
		tiles_ = original;
		order.clear();
		while (order.size() < facePairs.size())
		{
			std::vector<int> free;
			for (int i = 0; i < TileCount; ++i)
				if (IsFree(i))
					free.push_back(i);
			if (free.size() < 2)
				break;
			std::shuffle(free.begin(), free.end(), random_);
			order.emplace_back(free[0], free[1]);
			tiles_[free[0]].present = false;
			tiles_[free[1]].present = false;
		}
		if (order.size() == facePairs.size())
		{
			tiles_ = original;
			std::shuffle(facePairs.begin(), facePairs.end(), random_);
			for (size_t i = 0; i < order.size(); ++i)
			{
				tiles_[order[i].first].face = facePairs[i].first;
				tiles_[order[i].second].face = facePairs[i].second;
			}
			dealOrder_ = order;
			return true;
		}
	}
	tiles_ = original;
	return false;
}

bool MahjongGame::Remove(int first, int second)
{
	if (first == second || !IsFree(first) || !IsFree(second) ||
		!Matches(tiles_[first].face, tiles_[second].face))
		return false;
	tiles_[first].present = tiles_[second].present = false;
	history_.emplace_back(first, second);
	return true;
}

bool MahjongGame::Undo()
{
	if (history_.empty())
		return false;
	const Pair pair = history_.back();
	history_.pop_back();
	tiles_[pair.first].present = tiles_[pair.second].present = true;
	return true;
}

bool MahjongGame::Hint(Pair& pair) const
{
	for (int i = 0; i < TileCount; ++i)
		if (IsFree(i))
			for (int j = i + 1; j < TileCount; ++j)
				if (IsFree(j) && Matches(tiles_[i].face, tiles_[j].face))
				{
					pair = { i, j };
					return true;
				}
	return false;
}

int MahjongGame::Remaining() const
{
	return static_cast<int>(std::count_if(tiles_.begin(), tiles_.end(),
		[](const Tile& tile) { return tile.present; }));
}

bool MahjongGame::Save(std::ostream& out, const GameStatistics& statistics) const
{
	out << "MIKAS_MAHJONGG 1\n" << statistics.elapsedMilliseconds << ' '
		<< statistics.hints << ' ' << statistics.undos << ' ' << statistics.reshuffles << ' '
		<< (statistics.scoreRecorded ? 1 : 0) << '\n';
	for (int face : initialFaces_) out << face << ' ';
	out << '\n';
	for (const auto& pair : initialOrder_) out << pair.first << ' ' << pair.second << ' ';
	out << '\n';
	for (const auto& tile : tiles_) out << tile.face << ' ' << (tile.present ? 1 : 0) << ' ';
	out << '\n' << history_.size() << '\n';
	for (const auto& pair : history_) out << pair.first << ' ' << pair.second << ' ';
	out << '\n';
	return static_cast<bool>(out);
}

bool MahjongGame::Load(std::istream& in, GameStatistics& statistics)
{
	std::string magic;
	int version = 0;
	int scoreRecorded = 0;
	GameStatistics loadedStats;
	MahjongGame loaded(0);
	if (!(in >> magic >> version) || magic != "MIKAS_MAHJONGG" || version != 1 ||
		!(in >> loadedStats.elapsedMilliseconds >> loadedStats.hints >> loadedStats.undos >> loadedStats.reshuffles >> scoreRecorded) ||
		loadedStats.elapsedMilliseconds > 3155760000000ULL ||
		loadedStats.hints > 1000000 || loadedStats.undos > 1000000 || loadedStats.reshuffles > 1000000 ||
		(scoreRecorded != 0 && scoreRecorded != 1))
		return false;
	loadedStats.scoreRecorded = scoreRecorded != 0;
	std::array<int, FaceCount> initialCounts = {}, currentCounts = {};
	for (int& face : loaded.initialFaces_)
	{
		if (!(in >> face) || face < 0 || face >= FaceCount) return false;
		++initialCounts[face];
	}
	for (int i = 0; i < FaceCount; ++i)
		if (initialCounts[i] != (i < 34 ? 4 : 1)) return false;
	for (int i = 0; i < TileCount / 2; ++i)
	{
		int first, second;
		if (!(in >> first >> second) || first < 0 || first >= TileCount ||
			second < 0 || second >= TileCount || first == second) return false;
		loaded.initialOrder_.emplace_back(first, second);
	}
	for (auto& tile : loaded.tiles_)
	{
		int present;
		if (!(in >> tile.face >> present) || tile.face < 0 || tile.face >= FaceCount ||
			(present != 0 && present != 1)) return false;
		tile.present = present != 0;
		++currentCounts[tile.face];
	}
	if (currentCounts != initialCounts || loaded.Remaining() % 2 != 0) return false;
	unsigned int count;
	if (!(in >> count) || count > TileCount / 2) return false;
	for (unsigned int i = 0; i < count; ++i)
	{
		int first, second;
		if (!(in >> first >> second) || first < 0 || first >= TileCount ||
			second < 0 || second >= TileCount || first == second) return false;
		loaded.history_.emplace_back(first, second);
	}
	in >> std::ws;
	if (!in.eof()) return false;

	MahjongGame original = loaded;
	original.Restart();
	for (const auto& pair : loaded.initialOrder_)
		if (!original.Remove(pair.first, pair.second)) return false;
	if (original.Remaining() != 0) return false;

	MahjongGame replay = loaded;
	for (auto it = loaded.history_.rbegin(); it != loaded.history_.rend(); ++it)
	{
		if (replay.tiles_[it->first].present || replay.tiles_[it->second].present ||
			!Matches(replay.tiles_[it->first].face, replay.tiles_[it->second].face))
			return false;
		replay.tiles_[it->first].present = replay.tiles_[it->second].present = true;
	}
	replay.history_.clear();
	for (const auto& pair : loaded.history_)
		if (!replay.Remove(pair.first, pair.second)) return false;
	for (int i = 0; i < TileCount; ++i)
		if (replay.tiles_[i].present != loaded.tiles_[i].present) return false;
	loaded.dealOrder_.clear();
	*this = loaded;
	statistics = loadedStats;
	return true;
}
