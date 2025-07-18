#pragma once
#include "KamataEngine.h"
#include "Transform.h"
#define NOMINMAX
#include <algorithm>

#undef min
#undef max

// 前方宣言
class Player;

class CameraController {
private:
	// 構造体
	struct Rect {
		float left = 0.0f;
		float right = 1.0f;
		float bottom = 0.0f;
		float top = 1.0f;
	};

	KamataEngine::Camera* camera_ = nullptr;
	Player* target_ = nullptr;
	KamataEngine::Vector3 targetOffset_ = {0.0f, 0.0f, -15.0f};
	// カメラ移動制限
	Rect movableArea_ = {0.0f, 100.0f, 0.0f, 100.0f}; // 左、右、下、上の順
	                                                  // カメラの目標座標
	KamataEngine::Vector3 targetPosition_;
	// 座標補間の割合（0.1fなどがオススメ）
	static inline const float kInterpolationRate = 0.1f;
	static inline const float kVelocityBias = 5.0f; 
	// カメラ追従マージン
	static inline const Rect kFollowMargin = {
	    -3.0f, 
	    3.0f, 
	    -2.0f, 
	    2.0f   
	};



public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	void SetTarget(Player* target);

	void SetCamera(KamataEngine::Camera* camera);

   void SetMovableArea(const Rect& area) { movableArea_ = area; }

	void Reset();
};

