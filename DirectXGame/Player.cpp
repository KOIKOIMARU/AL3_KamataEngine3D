#include "Player.h"
#include <assert.h>
#include <numbers>

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
	MoveInput();

    // 衝突情報構造体を初期化
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = velocity_; // 現在の速度をコピーして渡す

    // マップとの衝突チェック
	CheckCollisionMap(collisionMapInfo);

	// 天井などの衝突結果に応じた処理（速度など）
	HandleCeilingCollision(collisionMapInfo);

	// 判定結果を反映して座標を更新
	ApplyCollisionResult(collisionMapInfo);

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

void Player::MoveInput() {
	if (onGround_) {
		if (Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_LEFT)) {
			Vector3 acceleration = {};
			if (Input::GetInstance()->PushKey(DIK_RIGHT)) {
				if (velocity_.x < 0.0f) {
					velocity_.x *= (1.0f - kAttenuation);
				}
				acceleration.x += kAcceleration;
				if (lrDirection_ != LRDirection::kRight) {
					lrDirection_ = LRDirection::kRight;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = 0.0f;
				}
			} else if (Input::GetInstance()->PushKey(DIK_LEFT)) {
				if (velocity_.x > 0.0f) {
					velocity_.x *= (1.0f - kAttenuation);
				}
				acceleration.x -= kAcceleration;
				if (lrDirection_ != LRDirection::kLeft) {
					lrDirection_ = LRDirection::kLeft;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = 0.0f;
				}
			}
			velocity_ += acceleration;
			velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
		} else {
			velocity_.x *= (1.0f - kAttenuation);
		}

		if (Input::GetInstance()->PushKey(DIK_UP)) {
			velocity_ += Vector3(0, kJumpAcceleration, 0);
		}
	} else {
		velocity_ += Vector3(0, -kGravityAcceleration, 0);
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}
}

void Player::CheckCollisionMap(CollisionMapInfo& info) {
	// 天井との衝突判定（上方向）
	CheckCollisionTop(info);

	// 地面との衝突判定（下方向）
	//CheckCollisionBottom(info);

	// 壁との衝突判定（右方向）
	//CheckCollisionRight(info);

	// 壁との衝突判定（左方向）
	//CheckCollisionLeft(info);
}

Vector3 Player::CornerPosition(const Vector3& center, Corner corner) {
	Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0}, // 右下
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0}, // 左下
	    {+kWidth / 2.0f, +kHeight / 2.0f, 0}, // 右上
	    {-kWidth / 2.0f, +kHeight / 2.0f, 0}, // 左上
	};

	return center + offsetTable[static_cast<uint32_t>(corner)];
}


void Player::CheckCollisionTop(CollisionMapInfo& info) {
	// 上昇中でなければ判定しない（早期リターン）
	if (info.move.y <= 0) {
		return;
	}

	std::array<Vector3, 4> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	// 真上の当たり判定を行う
	bool hit = false;

	// 左上の判定
	MapChipField::IndexSet indexSet;
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		hit = true;
	}

	// 右上の判定
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock) {
		hit = true;
	}

	if (hit) {
		// めり込みを排除する方向に移動量を制限
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]); // 左右どちらでも可
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);

		// 移動量を制限（上端と自分の下端との差分）
		info.move.y = std::max(0.0f, rect.bottom - (worldTransform_.translation_.y + kHeight / 2.0f));

		// フラグを立てる（天井に当たった）
		info.isHitCeiling = true;
	}
}

void Player::ApplyCollisionResult(const CollisionMapInfo& info) {
	// 判定によって変化した移動量分だけ、実装に座標に加算して移動させる
	worldTransform_.translation_ += info.move;
}

void Player::HandleCeilingCollision(const CollisionMapInfo& info) {
	// 天井に当たった？
	if (info.isHitCeiling) {
		DebugText::GetInstance()->ConsolePrintf("hit ceiling\n");

		// 上方向の速度を止める
		velocity_.y = 0;
	}
}

// デストラクタ
Player::~Player() {
}