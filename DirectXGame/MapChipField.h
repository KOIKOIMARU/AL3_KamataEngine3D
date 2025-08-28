#pragma once
#include "KamataEngine.h"
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>  

using namespace KamataEngine;

enum class MapChipType {
	kBlank, // 空白
	kBlock, // ブロック
};

enum class EventChipType : uint8_t {
	kNone,
	kEnemy,     // E: 雑魚スポーン
	kItem,      // I: 固定アイテム(テスト用)
	kBossGate,  // G: ゲート列
	kBossSpawn, // B: ボス出現
	kLanternRow // L: 提灯列(演出)
};

struct MapChipData {
	std::vector<std::vector<MapChipType>> data;
};
struct EventMapData {
	std::vector<std::vector<EventChipType>> data;
};

class MapChipField {
public:
	struct IndexSet {
		uint32_t xIndex;
		uint32_t yIndex;
	};
	struct Rect {
		float left, right, bottom, top;
	};

	// --- 衝突レイヤ ---
	void ResetMapChipData();
	void LoadMapChipCsv(const std::string& filePath);
	MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) const;


	// --- イベントレイヤ（追加） ---
	void LoadEventCsv(const std::string& filePath);
	EventChipType GetEventByIndex(int32_t xIndex, int32_t yIndex) const;
	std::vector<std::pair<uint32_t, uint32_t>> GetEventsInColumn(uint32_t xIndex, EventChipType type) const;
	void ConsumeEvent(uint32_t xIndex, uint32_t yIndex);
	std::optional<uint32_t> FindFirstColumn(EventChipType type) const;

	// ユーティリティ
	Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) const;
	IndexSet GetMapChipIndexSetByPosition(const Vector3& position) const;
	Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex) const;

	inline uint32_t GetNumVirtical() const { return kNumBlockVirtical; }
	inline uint32_t GetNumHorizontal() const { return kNumBlockHorizontal; }

	// MapChipField.h に追記
	MapChipType SafeGet(uint32_t xIndex, uint32_t yIndex) const;

	  uint8_t GetEnemyVariant(uint32_t x, uint32_t y) const;


private:
	MapChipData mapChipData_; // 衝突
	EventMapData eventMap_;   // イベント（追加）

	// 1ブロックのサイズ
	static inline const float kBlockWidth = 1.0f;
	static inline const float kBlockHeight = 1.0f;
	// ブロック数
	static inline const uint32_t kNumBlockVirtical = 20;
	static inline const uint32_t kNumBlockHorizontal = 100;

	  std::unordered_map<uint64_t, uint8_t> enemyVariant_; // key=(y<<32)|x

};
