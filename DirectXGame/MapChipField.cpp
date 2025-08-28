#include "MapChipField.h"
#include <cassert>
#include <fstream>
#include <sstream>
#define NOMINMAX

#undef min
#undef max


static const std::unordered_map<std::string, MapChipType> kMapChipTable = {
    {"0", MapChipType::kBlank},
    {"1", MapChipType::kBlock},
};
static const std::unordered_map<std::string, EventChipType> kEventTable = {
    {"",  EventChipType::kNone      },
    {"0", EventChipType::kNone      },
    {"E", EventChipType::kEnemy     },
    {"I", EventChipType::kItem      },
    {"G", EventChipType::kBossGate  },
    {"B", EventChipType::kBossSpawn },
    {"L", EventChipType::kLanternRow},
};

void MapChipField::ResetMapChipData() { mapChipData_.data.assign(kNumBlockVirtical, std::vector<MapChipType>(kNumBlockHorizontal, MapChipType::kBlank)); }

void MapChipField::LoadMapChipCsv(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) { /* ログ出して既定の空マップにする */
		return;
	}

	std::vector<std::vector<MapChipType>> rows;
	std::string line;
	while (std::getline(file, line)) {
		std::istringstream ls(line);
		std::vector<MapChipType> row;
		std::string w;
		while (std::getline(ls, w, ',')) {
			auto it = kMapChipTable.find(w);
			row.push_back(it != kMapChipTable.end() ? it->second : MapChipType::kBlank);
		}
		if (!row.empty())
			rows.push_back(std::move(row));
	}
	file.close();

	if (rows.empty())
		return;

	// 内部バッファをフィールド既定サイズに合わせたい場合はここで再サンプル
	// あるいは MapChipField 自体を可変サイズ対応に変更
	mapChipData_.data = std::move(rows);
	// ※ この場合は kNumBlockHorizontal / Virtical をメンバに置き換える（可変化）とよい
}

// ===== イベント層 =====
void MapChipField::LoadEventCsv(const std::string& filePath) {
	eventMap_.data.assign(kNumBlockVirtical, std::vector<EventChipType>(kNumBlockHorizontal, EventChipType::kNone));
	enemyVariant_.clear();

	std::ifstream file(filePath);
	assert(file.is_open());

	std::string line;
	for (uint32_t y = 0; y < kNumBlockVirtical && std::getline(file, line); ++y) {
		std::istringstream ls(line);
		for (uint32_t x = 0; x < kNumBlockHorizontal; ++x) {
			std::string w;
			std::getline(ls, w, ',');

			if (w.empty() || w == "0") {
				eventMap_.data[y][x] = EventChipType::kNone;
				continue;
			}

			// 大文字化（任意）
			for (auto& c : w)
				c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

			if (w[0] == 'E') {
				eventMap_.data[y][x] = EventChipType::kEnemy;
				// E, E1, E2… → 数字があれば読む（なければ0）
				uint8_t v = 0;
				if (w.size() >= 2)
					v = static_cast<uint8_t>(std::clamp(std::atoi(w.c_str() + 1), 0, 255));
				enemyVariant_[(static_cast<uint64_t>(y) << 32) | x] = v;
			} else if (w == "H") { // Hopper ショートハンド
				eventMap_.data[y][x] = EventChipType::kEnemy;
				enemyVariant_[(static_cast<uint64_t>(y) << 32) | x] = 1;
			} else if (w == "S") { // Shooter ショートハンド
				eventMap_.data[y][x] = EventChipType::kEnemy;
				enemyVariant_[(static_cast<uint64_t>(y) << 32) | x] = 2;
			} else if (w == "B") {
				eventMap_.data[y][x] = EventChipType::kBossSpawn;
			} else if (w == "G") {
				eventMap_.data[y][x] = EventChipType::kBossGate;
			} else if (w == "I") {
				eventMap_.data[y][x] = EventChipType::kItem;
			} else if (w == "L") {
				eventMap_.data[y][x] = EventChipType::kLanternRow;
			} else {
				eventMap_.data[y][x] = EventChipType::kNone;
			}
		}
	}
}


uint8_t MapChipField::GetEnemyVariant(uint32_t x, uint32_t y) const {
	uint64_t key = (static_cast<uint64_t>(y) << 32) | x;
	auto it = enemyVariant_.find(key);
	return (it == enemyVariant_.end()) ? 0 : it->second; // 既定0=Walker
}


MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) const {
	return SafeGet(xIndex, yIndex); // ← これでOK（最小修正）
}



EventChipType MapChipField::GetEventByIndex(int32_t xIndex, int32_t yIndex) const {
	if (xIndex < 0 || yIndex < 0)
		return EventChipType::kNone;
	if (xIndex >= (int32_t)kNumBlockHorizontal || yIndex >= (int32_t)kNumBlockVirtical)
		return EventChipType::kNone;
	return eventMap_.data[(uint32_t)yIndex][(uint32_t)xIndex];
}

std::vector<std::pair<uint32_t, uint32_t>> MapChipField::GetEventsInColumn(uint32_t xIndex, EventChipType type) const {
	std::vector<std::pair<uint32_t, uint32_t>> out;
	if (xIndex >= kNumBlockHorizontal)
		return out;
	for (uint32_t y = 0; y < kNumBlockVirtical; ++y) {
		if (eventMap_.data[y][xIndex] == type)
			out.emplace_back(xIndex, y);
	}
	return out;
}

void MapChipField::ConsumeEvent(uint32_t xIndex, uint32_t yIndex) {
	if (xIndex < kNumBlockHorizontal && yIndex < kNumBlockVirtical) {
		eventMap_.data[yIndex][xIndex] = EventChipType::kNone;
		uint64_t key = (static_cast<uint64_t>(yIndex) << 32) | xIndex;
		enemyVariant_.erase(key); // ← 追加
	}
}


std::optional<uint32_t> MapChipField::FindFirstColumn(EventChipType type) const {
	for (uint32_t x = 0; x < kNumBlockHorizontal; ++x) {
		if (!GetEventsInColumn(x, type).empty())
			return x;
	}
	return std::nullopt;
}

// ===== 位置⇔インデックス =====
Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) const { return {kBlockWidth * xIndex, kBlockHeight * (kNumBlockVirtical - 1 - yIndex), 0.0f}; }

// MapChipField.cpp
MapChipField::IndexSet MapChipField::GetMapChipIndexSetByPosition(const Vector3& position) const {
	IndexSet is{};
	int32_t xi = (int32_t)((position.x + kBlockWidth * 0.5f) / kBlockWidth);
	int32_t yiR = (int32_t)((position.y + kBlockHeight * 0.5f) / kBlockHeight);
	int32_t yi = (int32_t)kNumBlockVirtical - 1 - yiR;

	xi = std::max(0, (int32_t)std::min<uint32_t>(kNumBlockHorizontal - 1, xi));
	yi = std::max(0, (int32_t)std::min<uint32_t>(kNumBlockVirtical - 1, yi));

	is.xIndex = (uint32_t)xi;
	is.yIndex = (uint32_t)yi;
	return is;
}


MapChipField::Rect MapChipField::GetRectByIndex(uint32_t xIndex, uint32_t yIndex) const {
	Vector3 c = GetMapChipPositionByIndex(xIndex, yIndex);
	Rect r;
	r.left = c.x - kBlockWidth * 0.5f;
	r.right = c.x + kBlockWidth * 0.5f;
	r.bottom = c.y - kBlockHeight * 0.5f;
	r.top = c.y + kBlockHeight * 0.5f;
	return r;
}

// MapChipField.cpp に実装
MapChipType MapChipField::SafeGet(uint32_t xIndex, uint32_t yIndex) const {
	if (yIndex >= mapChipData_.data.size())
		return MapChipType::kBlank;
	const auto& row = mapChipData_.data[yIndex];
	if (xIndex >= row.size())
		return MapChipType::kBlank;
	return row[xIndex];
}
