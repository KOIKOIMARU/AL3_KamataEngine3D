#include "Enemy.h"
#include <assert.h>
#include <numbers>

void Enemy::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	// NULLポインタチェック
	assert(model);
	assert(camera);

	// モデルの設定
	model_ = model;
	camera_ = camera;

	// ワールド変換データの初期化
	worldTransform_.Initialize();

	worldTransform_.translation_ = position;

	worldTransform_.rotation_.y = std::numbers::pi_v<float> / -2.0f; // Y軸を90度回転

	// 速度を設定する
	velocity_ = Vector3(-kWalkSpeed, 0.0f, 0.0f); // 初期速度を設定

	// 歩行モーションのタイマーを初期化
	walkTimer_ = 0.0f; // 初期化
}

void Enemy::Update() {
	// 歩行モーションのタイマーを更新
	walkTimer_ += 1.0f / 60.0f;

	// サインで角度変化
	float param = std::sin(2.0f * std::numbers::pi_v<float> * walkTimer_ / kWalkMotionTime);
	float t = (param + 1.0f) / 2.0f; // 0.0〜1.0に変換
	float degree = std::lerp(kWalkMotionAngleStart, kWalkMotionAngleEnd, t);
	worldTransform_.rotation_.x = degree * (std::numbers::pi_v<float> / 180.0f); // ラジアンに変換

	worldTransform_.translation_.x += velocity_.x; // X座標に速度を加算

	// 行列を定数バッファに転送
	UpdateWorldTransform(worldTransform_);
}

void Enemy::Draw() { model_->Draw(worldTransform_, *camera_); }

// デストラクタ
Enemy::~Enemy() {};