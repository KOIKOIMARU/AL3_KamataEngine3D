#pragma once
#include "KamataEngine.h"
#include "MapChipField.h"
#include "Transform.h"
#include <cmath>
#define NOMINMAX
#include <algorithm>
#include <array>

#undef min
#undef max

class Enemy;

class DeathParticles {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	void Update();

	void Draw();

	Matrix4x4 MakeRotateZ(float angle);

	Vector3 Transform(const Vector3& v, const Matrix4x4& m);

	// デスフラグのゲッター
	bool IsFinished() const { return isFinished_; }
private:
	// モデル
	KamataEngine::Model* model_ = nullptr;
	// カメラ
	KamataEngine::Camera* camera_ = nullptr;
	// パーティクルの個数
	static inline const uint32_t kNumParticles = 8;

	std::array<WorldTransform, kNumParticles> worldTransforms_; // パーティクルの配列

	static inline const float kDuration = 1.0f; // 生存時間（秒）
	static inline const float kSpeed = 0.1f;    // 移動速度
	static inline const float kAngleUnit = 2.0f * 3.14159265f / kNumParticles;

	bool isFinished_ = false; // 終了フラグ
	float counter_ = 0.0f;    // 経過時間カウント

	// 色変更オブジェクト
	KamataEngine::ObjectColor objectColor_;
	// 色の数値（RGBA）
	KamataEngine::Vector4 color_;
};
