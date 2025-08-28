#pragma once
#include "KamataEngine.h"
#include "Transform.h"
#define NOMINMAX
#include <algorithm>
#include <random> // ★ 追加

#undef min
#undef max

class Player;

class CameraController {
public:
	struct Rect {
		float left = 0.0f;
		float right = 1.0f;
		float bottom = 0.0f;
		float top = 1.0f;
	};

	void Initialize();
	void Update();

	void SetTarget(Player* target);
	void SetCamera(KamataEngine::Camera* camera);

	void SetMovableArea(const Rect& area) { movableArea_ = area; }
	void Reset();

	void SetXMax(float x) { movableArea_.right = x; }
	void SetXMin(float x) { movableArea_.left = x; }
	void LockX(float x) { movableArea_.left = movableArea_.right = x; }
	CameraController::Rect GetMovableArea() const { return movableArea_; }

	// 演出API
	void AddHitstop(float sec) { hitstop_ = std::max(hitstop_, sec); }
	void AddShake(float amp, float time) {
		shakeAmp_ = std::max(shakeAmp_, amp);
		shakeTime_ = std::max(shakeTime_, time); // ★ 統一
		// 衝撃毎に位相をランダム微調整
		shakePhaseX_ += unit_(rng_) * 0.7f;
		shakePhaseY_ += unit_(rng_) * 0.7f;
	}

	void SetFollowMargin(const Rect& r) { followMargin_ = r; } // ★追加
	Rect GetFollowMargin() const { return followMargin_; }     // 既存の Getter を差し替え

	// Xが固定されているか？
	bool IsXLocked() const { return std::abs(movableArea_.right - movableArea_.left) < 1e-6f; }

	    // ★ 追加：簡単に調整できるセッター
	void SetYOffset(float y) { targetOffset_.y = y; }
	void SetZoomDistance(float d) { targetOffset_.z = -std::fabs(d); } // 近づけるほど小さく
	void ForceSnap();                                                  // 初期位置を即同期

	// ★ 追加：Xロック解除（使っているなら）
	void UnlockX() { /* 可動域を広めに戻す */
		// マップに合わせて後でSetMovableAreaを呼ぶのが理想。暫定で広域。
		movableArea_.left = -1e6f;
		movableArea_.right = +1e6f;
	}

private:
	KamataEngine::Camera* camera_ = nullptr;
	Player* target_ = nullptr;

   // 近め＆下寄りに見える既定
	KamataEngine::Vector3 targetOffset_ = {0.0f, +1.2f, -8.0f}; // ← 元 -15 を -8 へ、Yを+1.2



	Rect movableArea_ = {0.0f, 100.0f, 0.0f, 100.0f};
	KamataEngine::Vector3 targetPosition_ = {0.0f, 0.0f, 0.0f};

	static inline const float kInterpolationRate = 0.1f;
	static inline const float kVelocityBias = 2.0f;
	// CameraController.h の定数
	// 旧: { -1.2f, +2.6f, -1.2f, +1.6f }
	static inline const Rect kFollowMargin = {-3.0f, 3.0f, -2.0f, 2.0f};
	Rect followMargin_ = kFollowMargin; // ★追加：実際に使う可変マージン

	// ヒットストップ
	float hitstop_ = 0.0f;

	// シェイク（位相ベース）
	float shakeTime_ = 0.0f;       // ★ 残り時間（秒）
	float shakeAmp_ = 0.0f;        // 振幅（ワールド座標オフセット量）
	float shakeFreq_ = 22.0f;      // 基本周波数（Hz相当）
	float shakePhaseX_ = 0.0f;     // 位相X
	float shakePhaseY_ = 1.57f;    // 位相Y（ずらす）
	float shakeRotScale_ = 0.035f; // ロール係数（必要なら調整）

	// 乱数
	std::mt19937 rng_{12345};
	std::uniform_real_distribution<float> unit_{-1.0f, 1.0f};
};
