#include "../MahjongGame.h"
#include "../BoardGeometry.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <set>
#include <sstream>
#include <tuple>

static int Find(const MahjongGame& game, int x, int y, int z)
{
	for (int i = 0; i < MahjongGame::TileCount; ++i)
	{
		const Slot& slot = game.Tiles()[i].slot;
		if (slot.x == x && slot.y == y && slot.z == z)
			return i;
	}
	return -1;
}

static void VerifySolution(MahjongGame game)
{
	const auto order = game.DealOrder();
	assert(order.size() * 2 == static_cast<size_t>(game.Remaining()));
	for (const auto& pair : order)
	{
		for (int i = 0; i < MahjongGame::TileCount; ++i)
		{
			const auto& tile = game.Tiles()[i];
			bool above = false, left = false, right = false;
			for (int j = 0; j < MahjongGame::TileCount; ++j)
			{
				const auto& other = game.Tiles()[j];
				if (i == j || !other.present) continue;
				const int dx = other.slot.x - tile.slot.x;
				const int dy = other.slot.y - tile.slot.y;
				if (other.slot.z > tile.slot.z && dx > -2 && dx < 2 && dy > -2 && dy < 2)
					above = true;
				if (other.slot.z == tile.slot.z && dy > -2 && dy < 2)
				{
					left |= dx == -2;
					right |= dx == 2;
				}
			}
			assert(game.IsFree(i) == (tile.present && !above && (!left || !right)));
		}
		assert(game.IsFree(pair.first));
		assert(game.IsFree(pair.second));
		assert(MahjongGame::Matches(game.Tiles()[pair.first].face, game.Tiles()[pair.second].face));
		assert(game.Remove(pair.first, pair.second));
	}
	assert(game.Remaining() == 0);
}

int main()
{
	assert(!MahjongGame::Matches(-1, -1));
	assert(!MahjongGame::Matches(MahjongGame::FaceCount, MahjongGame::FaceCount));
	assert(!MahjongGame::Matches(0, -1));
	assert(!MahjongGame::Matches(MahjongGame::FaceCount, 0));
	assert(MahjongGame::Matches(0, 0));
	assert(MahjongGame::Matches(MahjongGame::FaceCount - 1, MahjongGame::FaceCount - 1));
	{
		MahjongGame game(42);
		assert(game.NewGame());
		const auto originalTiles = game.Tiles();
		const auto originalOrder = game.DealOrder();
		for (int i = 0; i < 10; ++i)
			assert(game.Remove(originalOrder[i].first, originalOrder[i].second));
		assert(game.Reshuffle());
		const auto reshuffledOrder = game.DealOrder();
		for (int i = 0; i < 5; ++i)
			assert(game.Remove(reshuffledOrder[i].first, reshuffledOrder[i].second));
		GameStatistics stats;
		stats.elapsedMilliseconds = 123456;
		stats.hints = 3; stats.undos = 2; stats.reshuffles = 1; stats.scoreRecorded = true;
		std::ostringstream output;
		assert(game.Save(output, stats));
		std::ostringstream failedOutput;
		failedOutput.setstate(std::ios::badbit);
		assert(!game.Save(failedOutput, stats));
		const std::string saved = output.str();
		MahjongGame loaded(99);
		GameStatistics restored;
		std::istringstream input(saved);
		assert(loaded.Load(input, restored));
		assert(restored.elapsedMilliseconds == stats.elapsedMilliseconds);
		assert(restored.hints == 3 && restored.undos == 2 && restored.reshuffles == 1);
		assert(restored.scoreRecorded);
		for (int i = 0; i < MahjongGame::TileCount; ++i)
		{
			assert(loaded.Tiles()[i].face == game.Tiles()[i].face);
			assert(loaded.Tiles()[i].present == game.Tiles()[i].present);
		}
		for (int i = 0; i < 5; ++i)
			assert(loaded.Undo());
		assert(!loaded.Undo());
		loaded.Restart();
		for (int i = 0; i < MahjongGame::TileCount; ++i)
			assert(loaded.Tiles()[i].present && loaded.Tiles()[i].face == originalTiles[i].face);
		VerifySolution(loaded);
		std::vector<std::string> invalidFiles = {
			std::string(), saved.substr(0, saved.size() / 2), std::string("WRONG") + saved,
			saved + "unexpected"
		};
		std::istringstream tokensStream(saved);
		std::vector<std::string> tokens;
		std::string token;
		while (tokensStream >> token) tokens.push_back(token);
		assert(tokens.size() > 585);
		for (int corruption = 0; corruption < 3; ++corruption)
		{
			auto altered = tokens;
			if (corruption == 0) altered[295] = "999";
			if (corruption == 1) altered[152] = altered[151];
			if (corruption == 2) altered[585] = altered[584];
			std::ostringstream broken;
			for (const auto& value : altered) broken << value << ' ';
			invalidFiles.push_back(broken.str());
		}
		for (const std::string& invalid : invalidFiles)
		{
			const auto before = loaded.Tiles();
			GameStatistics unchanged = stats;
			std::istringstream broken(invalid);
			assert(!loaded.Load(broken, unchanged));
			assert(unchanged.elapsedMilliseconds == stats.elapsedMilliseconds);
			for (int i = 0; i < MahjongGame::TileCount; ++i)
				assert(loaded.Tiles()[i].face == before[i].face && loaded.Tiles()[i].present == before[i].present);
		}
	}
	for (const auto& size : { std::pair<float, float>{780.0f, 580.0f},
		std::pair<float, float>{400.0f, 900.0f}, std::pair<float, float>{1600.0f, 400.0f} })
	{
		const BoardGeometry geometry(size.first, size.second);
		const BoardRect base = geometry.TileRect({ 13, 7, 0 });
		const BoardRect top = geometry.TileRect({ 13, 7, 4 });
		assert(std::abs((top.left - base.left) - 32.0f * geometry.Scale()) < 0.001f);
		assert(std::abs((top.top - base.top) + 32.0f * geometry.Scale()) < 0.001f);
		assert(std::abs((top.right - top.left) - 50.0f * geometry.Scale()) < 0.001f);
		MahjongGame layout(0);
		for (const auto& tile : layout.Tiles())
		{
			const BoardRect rect = geometry.TileRect(tile.slot);
			assert(rect.left >= 0 && rect.top >= 0);
			assert(rect.right <= size.first && rect.bottom <= size.second);
			const BoardRect image = geometry.TileImageRect(tile.slot);
			assert(image.left >= 0 && image.top >= 0);
			assert(image.right <= size.first && image.bottom <= size.second);
		}
		std::array<Tile, MahjongGame::TileCount> tiles = {};
		tiles[0] = { { 8, 4, 0 }, 0, true };
		tiles[1] = { { 10, 4, 0 }, 1, true };
		tiles[2] = { { 8, 6, 0 }, 2, true };
		const BoardRect first = geometry.TileRect(tiles[0].slot);
		const BoardRect second = geometry.TileRect(tiles[1].slot);
		const BoardRect below = geometry.TileRect(tiles[2].slot);
		assert(std::abs(first.right - second.left) < 0.001f);
		assert(std::abs(first.bottom - below.top) < 0.001f);
		const float epsilon = geometry.Scale();
		assert(geometry.HitTest(tiles, first.right - epsilon, (first.top + first.bottom) / 2) == 0);
		assert(geometry.HitTest(tiles, first.left - epsilon, (first.top + first.bottom) / 2) == 0);
		assert(geometry.HitTest(tiles, (first.left + first.right) / 2, first.bottom - epsilon / 2) == 0);
		tiles[0].present = false;
		assert(geometry.HitTest(tiles, second.left - epsilon, (second.top + second.bottom) / 2) == 1);
		tiles[0].present = true;
		tiles[3] = { { 8, 4, 1 }, 3, true };
		const BoardRect upper = geometry.TileRect(tiles[3].slot);
		assert(geometry.HitTest(tiles, upper.left - epsilon, (upper.top + upper.bottom) / 2) == 3);
		assert(geometry.HitTest(tiles, -1.0f, -1.0f) == -1);
		std::array<Tile, MahjongGame::TileCount> offsetTiles = {};
		offsetTiles[0] = { { 0, 7, 0 }, 0, true };
		offsetTiles[1] = { { 2, 6, 0 }, 1, true };
		const BoardRect offset = geometry.TileRect(offsetTiles[0].slot);
		const BoardRect neighbor = geometry.TileRect(offsetTiles[1].slot);
		assert(std::abs(offset.right - neighbor.left) < 0.001f);
		assert(geometry.HitTest(offsetTiles, offset.right - epsilon, offset.top + epsilon) == 0);
		assert(geometry.HitTest(offsetTiles, neighbor.left + epsilon, offset.top + epsilon) == 1);
	}
	for (unsigned int seed = 0; seed < 100; ++seed)
	{
		MahjongGame game(seed);
		assert(game.NewGame());
		std::array<int, 5> layers = {};
		std::array<int, MahjongGame::FaceCount> faces = {};
		std::set<std::tuple<int, int, int>> slots;
		for (const auto& tile : game.Tiles())
		{
			++layers[tile.slot.z];
			++faces[tile.face];
			slots.emplace(tile.slot.x, tile.slot.y, tile.slot.z);
		}
		assert((layers == std::array<int, 5>{87, 36, 16, 4, 1}));
		assert(slots.size() == MahjongGame::TileCount);
		const int starts[] = {2, 6, 4, 2, 2, 4, 6, 2};
		const int ends[] = {24, 20, 22, 24, 24, 22, 20, 24};
		for (int row = 0; row < 8; ++row)
			for (int x = starts[row]; x <= ends[row]; x += 2)
				assert(slots.count(std::make_tuple(x, row * 2, 0)) == 1);
		assert(slots.count(std::make_tuple(0, 7, 0)) == 1);
		assert(slots.count(std::make_tuple(26, 7, 0)) == 1);
		assert(slots.count(std::make_tuple(28, 7, 0)) == 1);
		for (int z = 1; z <= 3; ++z)
			for (int y = z * 2; y <= 14 - z * 2; y += 2)
				for (int x = 6 + z * 2; x <= 20 - z * 2; x += 2)
					assert(slots.count(std::make_tuple(x, y, z)) == 1);
		assert(slots.count(std::make_tuple(13, 7, 4)) == 1);
		for (int i = 0; i < 34; ++i) assert(faces[i] == 4);
		for (int i = 34; i < 42; ++i) assert(faces[i] == 1);
		assert(game.IsFree(Find(game, 13, 7, 4)));
		assert(!game.IsFree(Find(game, 12, 6, 3)));
		assert(!game.IsFree(Find(game, 2, 6, 0)));
		assert(game.IsFree(Find(game, 0, 7, 0)));
		assert(game.IsFree(Find(game, 28, 7, 0)));
		assert(!game.IsFree(-1));
		assert(!game.Remove(-1, MahjongGame::TileCount));
		assert(!game.Remove(0, 0));
		assert(MahjongGame::Matches(34, 37));
		assert(MahjongGame::Matches(38, 41));
		assert(!MahjongGame::Matches(37, 38));
		VerifySolution(game);

		MahjongGame::Pair hint;
		assert(game.Hint(hint));
		assert(game.Remove(hint.first, hint.second));
		assert(game.Remaining() == 142);
		assert(game.Undo());
		assert(game.Remaining() == 144);
		assert(!game.Undo());

		const auto order = game.DealOrder();
		for (int i = 0; i < 20; ++i)
			assert(game.Remove(order[i].first, order[i].second));
		std::array<int, MahjongGame::FaceCount> before = {};
		for (const auto& tile : game.Tiles())
			if (tile.present) ++before[tile.face];
		assert(game.Reshuffle());
		std::array<int, MahjongGame::FaceCount> after = {};
		for (const auto& tile : game.Tiles())
			if (tile.present) ++after[tile.face];
		assert(before == after);
		assert(!game.Undo());
		VerifySolution(game);
	}
	std::cout << "100 generated games and reshuffles passed\n";
}
