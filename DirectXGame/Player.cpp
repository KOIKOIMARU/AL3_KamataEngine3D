#include "Player.h"
#include <assert.h>
#include <numbers>
#define NOMINMAX
#include <algorithm>

using namespace KamataEngine;

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	// NULLポインタチェック
	assert(model);
	assert(camera);

	// モデルの設定
	model_ = model;
	camera_ = camera;

	// ワールド変換データの初期化
	worldTransform_.Initialize();

	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f; // Y軸を90度回転
}

void Player::Update() {
	// 移動入力
	// 接地状態
	if (onGround_) {
		if (Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_LEFT)) {
			// 左右加速
			Vector3 acceleration = {};
			if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
				// 左移動中の右入力
				if (velocity_.x < 0.0f) {
					// 速度と逆方向に入力中は急ブレーキ
					velocity_.x *= (1.0f - kAttenuation); // 速度を反転して減速
				}
				acceleration.x += kAcceleration;
				if (lrDirection_ != LRDirection::kRight) {
					lrDirection_ = LRDirection::kRight;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = 0.0f;
				}

			} else if (Input::GetInstance()->PushKey(DIK_LEFT)) {
				// 左移動中の右入力
				if (velocity_.x > 0.0f) {
					// 速度と逆方向に入力中は急ブレーキ
					velocity_.x *= (1.0f - kAttenuation); // 速度を反転して減速
				}
				acceleration.x -= kAcceleration;
				if (lrDirection_ != LRDirection::kLeft) {
					lrDirection_ = LRDirection::kLeft;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = 0.0f;
				}
			}
			// 加速/減速
			velocity_ += acceleration;

			// 最大速度制限
			velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed); // 最大速度を0.1に制限
		} else {
			// 減速
			velocity_.x *= (1.0f - kAttenuation);
		}
		if (Input::GetInstance()->PushKey(DIK_UP)) {
		   // ジャンプ加速
			velocity_ += Vector3(0,kJumpAcceleration,0); // ジャンプ速度を設定
		}
	} else {
		// 落下速度
		velocity_ += Vector3(0,-kGravityAcceleration,0); // 重力加速度を適用
		// 最大落下速度制限
		velocity_.y = max(velocity_.y, -kLimitFallSpeed); // 最大落下速度を0.1に制限
	}

	// 移動
	worldTransform_.translation_ += velocity_;

	bool landing = false;
	if (velocity_.y < 0) {
		if (worldTransform_.translation_.y <= 1.0f) {
			landing = true;
		}
	}

	if (onGround_) {
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		}
	} else {
		if (landing) {
			worldTransform_.translation_.y = 1.0f;
			velocity_.x *= (1.0f - kAttenuation);
			velocity_.y = 0.0f;
			onGround_ = true;
		}
	}

	// 旋回制御
	if (turnTimer_ > 0.0f) {
		// 旋回タイマーを1/60秒だけカウントダウンする
		turnTimer_ -= 1.0f / 60.0f;

		// 左右の自キャラ角度テーブル
		float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};

		// 状態に応じた目標角度を取得する
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

		// 自キャラの角度を設定する（線形補間 or 補間関数）
		float t = 1.0f - (turnTimer_ / kTimeRun);
		worldTransform_.rotation_.y = std::lerp(turnFirstRotationY_, destinationRotationY, t);
	} else {
		// 左右の自キャラ角度テーブル
		float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};

		// 状態に応じた角度を即設定する
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		worldTransform_.rotation_.y = destinationRotationY;
	}



	// 行列を定数バッファに転送
	UpdateWorldTransform(worldTransform_);

}

void Player::Draw() {  
   model_->Draw(worldTransform_, *camera_);  
}

// デストラクタ
Player::~Player() {
}