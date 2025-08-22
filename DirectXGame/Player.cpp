#include "Player.h"
#include <assert.h>
#include <numbers>

using namespace KamataEngine;

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Model* modelAttack,KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	// NULLポインタチェック
	assert(model);
	assert(modelAttack);
	assert(camera);

	// モデルの設定
	model_ = model;
	modelAttack_ = modelAttack;
	camera_ = camera;

	// ワールド変換データの初期化
	worldTransform_.Initialize();

	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f; // Y軸を90度回転
	worldTransformAttack_.Initialize();
}

void Player::Update() {
	if (behaviorRequest_ != Behavior::kUnknown) {
		// behaviorRequest_ に値が入っていたらビヘイビア遷移する
		behavior_ = behaviorRequest_; // ビヘイビア変更

		// 初期化処理（新しいビヘイビアに応じて）
		switch (behavior_) {
		case Behavior::kRoot:
		default:
			BehaviorRootInitialize();
			break;
		case Behavior::kAttack:
			BehaviorAttackInitialize();
			break;
		}

		behaviorRequest_ = Behavior::kUnknown; // リクエストリセット
	}

	// ビヘイビアごとの更新処理を呼ぶ
	switch (behavior_) {
	case Behavior::kRoot:
	default:
		BehaviorRootUpdate();
		break;
	case Behavior::kAttack:
		BehaviorAttackUpdate();
		break;
	}
}


void Player::BehaviorRootUpdate() {
	MoveInput();

	if (Input::GetInstance()->PushKey(DIK_SPACE)) {
		behaviorRequest_ = Behavior::kAttack;
	}

	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = velocity_;
	CheckCollisionMap(collisionMapInfo);
	HandleCeilingCollision(collisionMapInfo);
	HandleWallCollision(collisionMapInfo);
	ApplyCollisionResult(collisionMapInfo);
	SwitchLandingState(collisionMapInfo);

	if (turnTimer_ > 0.0f) {
		turnTimer_ -= 1.0f / 60.0f;
		float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		float t = 1.0f - (turnTimer_ / kTimeRun);
		worldTransform_.rotation_.y = std::lerp(turnFirstRotationY_, destinationRotationY, t);
	} else {
		float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		worldTransform_.rotation_.y = destinationRotationY;
	}

	UpdateWorldTransform(worldTransform_);
	UpdateWorldTransform(worldTransformAttack_);
}

void Player::BehaviorAttackUpdate() {
	// 前方向ベクトル（Y軸回転からZ-方向へ）
	Vector3 direction = {std::sin(worldTransform_.rotation_.y), 0.0f, std::cos(worldTransform_.rotation_.y)};

	// 攻撃フェーズに応じた処理
	switch (attackPhase_) {
	case AttackPhase::Tame:
	default: {
		float t = static_cast<float>(attackParameter_) / 20.0f;
		worldTransform_.scale_.z = EaseOut(1.0f, 0.3f, t);
		worldTransform_.scale_.y = EaseOut(1.0f, 1.6f, t);

		// 前進動作へ移行
		if (attackParameter_ >= 20.0f) {
			attackPhase_ = AttackPhase::Dash;
			attackParameter_ = 0; // カウンターをリセット
		}

		velocity_ = {0.0f, 0.0f, 0.0f}; // 止まる
		break;
	}

	case AttackPhase::Dash: {
		float t = static_cast<float>(attackParameter_) / 20.0f;
		worldTransform_.scale_.z = EaseOut(0.3f, 1.3f, t);
		worldTransform_.scale_.y = EaseIn(1.6f, 0.7f, t);

		// 前方向に移動
		velocity_ = direction * 0.2f;

		// 余韻動作へ移行
		if (attackParameter_ >= 20.0f) {
			attackPhase_ = AttackPhase::Recoil;
			attackParameter_ = 0;
		}
		break;
	}

	case AttackPhase::Recoil: {
		float t = static_cast<float>(attackParameter_) / 20.0f;
		worldTransform_.scale_.z = EaseOut(1.3f, 1.0f, t);
		worldTransform_.scale_.y = EaseOut(0.7f, 1.0f, t);

		velocity_ = {0.0f, 0.0f, 0.0f}; // 止まる
		break;
	}
	}

	worldTransformAttack_.translation_ = worldTransform_.translation_;
	worldTransformAttack_.rotation_ = worldTransform_.rotation_;

	// 衝突判定と移動適用は共通処理
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = velocity_;
	CheckCollisionMap(collisionMapInfo);
	ApplyCollisionResult(collisionMapInfo);

	// 行列更新
	UpdateWorldTransform(worldTransform_);
	UpdateWorldTransform(worldTransformAttack_);

	// カウント進める（共通）
	attackParameter_++;

	// 攻撃行動終了
	if (attackPhase_ == AttackPhase::Recoil && attackParameter_ >= 20) {
		behaviorRequest_ = Behavior::kRoot;
		attackPhase_ = AttackPhase::Tame;
		attackParameter_ = 0;
	}
}


void Player::Draw() { 
	model_->Draw(worldTransform_, *camera_); 
	if (attackPhase_ == AttackPhase::Dash || attackPhase_ == AttackPhase::Recoil)
	modelAttack_->Draw(worldTransformAttack_, *camera_);
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
	CheckCollisionBottom(info);

	// 壁との衝突判定（右方向）
	CheckCollisionRight(info);

	// 壁との衝突判定（左方向）
	CheckCollisionLeft(info);
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
	if (info.move.y <= 0) {
		return;
	}

	std::array<Vector3, 4> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType, mapChipTypeNext;
	bool hit = false;

	// 左上
	MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1); // ↓下
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		Vector3 prePos = worldTransform_.translation_;
		Vector3 preTop = CornerPosition(prePos, kLeftTop);
		MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(preTop);
		if (indexSetNow.yIndex != indexSet.yIndex) {
			hit = true;
		}
	}

	// 右上
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex + 1); // ↓下
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		Vector3 prePos = worldTransform_.translation_;
		Vector3 preTop = CornerPosition(prePos, kRightTop);
		MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(preTop);
		if (indexSetNow.yIndex != indexSet.yIndex) {
			hit = true;
		}
	}

	if (hit) {
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		info.move.y = std::max(0.0f, rect.bottom - (worldTransform_.translation_.y + kHeight / 2.0f));
		info.isHitCeiling = true;
	}
}


void Player::CheckCollisionBottom(CollisionMapInfo& info) {
	if (info.move.y >= 0) {
		return;
	}

	std::array<Vector3, 4> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType, mapChipTypeNext;
	bool hit = false;

	// 左下
	MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1); // 一つ上

	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		// セル境界をまたいだかどうかチェック
		Vector3 prePos = worldTransform_.translation_;
		Vector3 preBottom = CornerPosition(prePos, kLeftBottom);
		MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(preBottom);

		if (indexSetNow.yIndex != indexSet.yIndex) {
			hit = true;
		}
	}

	// 右下も同様
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1); // 一つ上

	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		Vector3 prePos = worldTransform_.translation_;
		Vector3 preBottom = CornerPosition(prePos, kRightBottom);
		MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(preBottom);

		if (indexSetNow.yIndex != indexSet.yIndex) {
			hit = true;
		}
	}

	if (hit) {
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);

		info.move.y = std::min(0.0f, rect.top - (worldTransform_.translation_.y - kHeight / 2.0f));
		info.isHitGround = true;
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

void Player::SwitchLandingState(const CollisionMapInfo& info) {
	if (onGround_) {
		// Y速度が上向き → ジャンプ開始
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {
			// 落下判定（床がなくなったら）
			bool hit = false;

			// 左右下端のマップチップを確認
			Vector3 leftBottom = CornerPosition(worldTransform_.translation_, kLeftBottom);
			Vector3 rightBottom = CornerPosition(worldTransform_.translation_, kRightBottom);

			MapChipField::IndexSet indexSet;
			MapChipType type;

			// 左下
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(leftBottom);
			type = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (type == MapChipType::kBlock) {
				hit = true;
			}

			// 右下
			indexSet = mapChipField_->GetMapChipIndexSetByPosition(rightBottom);
			type = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			if (type == MapChipType::kBlock) {
				hit = true;
			}

			if (!hit) {
				onGround_ = false; // 落下開始
			}
		}
	} else {
		// 空中 → 接地処理
		if (info.isHitGround) {
			onGround_ = true;
			// velocity_.x *= (1.0f - kAttenuationLanding); // 減速
			velocity_.y = 0.0f; // Y速度ゼロで止める
		}
	}
}

void Player::CheckCollisionRight(CollisionMapInfo& info) {
	if (info.move.x <= 0) {
		return;
	}

	std::array<Vector3, 4> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType, mapChipTypeNext;
	bool hit = false;

	// 右上
	MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex); // ←左
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		Vector3 prePos = worldTransform_.translation_;
		Vector3 preRight = CornerPosition(prePos, kRightTop);
		MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(preRight);

		if (indexSetNow.xIndex != indexSet.xIndex) {
			hit = true;
		}
	}

	// 右下
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex - 1, indexSet.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		Vector3 prePos = worldTransform_.translation_;
		Vector3 preRight = CornerPosition(prePos, kRightBottom);
		MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(preRight);

		if (indexSetNow.xIndex != indexSet.xIndex) {
			hit = true;
		}
	}

	if (hit) {
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kRightBottom]);
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		info.move.x = std::min(0.0f, rect.left - (worldTransform_.translation_.x + kWidth / 2.0f));
		info.isHitWall = true;
	}
}


void Player::CheckCollisionLeft(CollisionMapInfo& info) {
	if (info.move.x >= 0) {
		return;
	}

	std::array<Vector3, 4> positionsNew;
	for (uint32_t i = 0; i < positionsNew.size(); ++i) {
		positionsNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType, mapChipTypeNext;
	bool hit = false;

	// 左上
	MapChipField::IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftTop]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex); // →右
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		Vector3 prePos = worldTransform_.translation_;
		Vector3 preLeft = CornerPosition(prePos, kLeftTop);
		MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(preLeft);
		if (indexSetNow.xIndex != indexSet.xIndex) {
			hit = true;
		}
	}

	// 左下
	indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex + 1, indexSet.yIndex); // →右
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
		Vector3 prePos = worldTransform_.translation_;
		Vector3 preLeft = CornerPosition(prePos, kLeftBottom);
		MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(preLeft);
		if (indexSetNow.xIndex != indexSet.xIndex) {
			hit = true;
		}
	}

	if (hit) {
		indexSet = mapChipField_->GetMapChipIndexSetByPosition(positionsNew[kLeftBottom]);
		MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		info.move.x = std::max(0.0f, rect.right - (worldTransform_.translation_.x - kWidth / 2.0f));
		info.isHitWall = true;
	}
}


void Player::HandleWallCollision(const CollisionMapInfo& info) {
	if (info.isHitWall) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

Vector3 Player::GetWorldPosition() {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0]; // Tx
	worldPos.y = worldTransform_.matWorld_.m[3][1]; // Ty
	worldPos.z = worldTransform_.matWorld_.m[3][2]; // Tz
	return worldPos;
}

AABB Player::GetAABB() {
	Vector3 worldPos = GetWorldPosition();

	AABB aabb;
	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};

	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

	return aabb;
}

void Player::OnCollision(const Enemy* enemy) {
	(void)enemy; 
	isDead_ = true;
}


void Player::BehaviorRootInitialize() {
	
}

void Player::BehaviorAttackInitialize() {
	// カウンター初期化
	attackParameter_ = 0;
}

float Player::EaseIn(float start, float end, float t) {
	// 指数イージング（InQuad: 最初ゆっくり）
	t = t * t;
	return start + (end - start) * t;
}


float Player::EaseOut(float start, float end, float t) {
	// 指数イージング（OutQuad: 加速して減速する）
	t = 1.0f - (1.0f - t) * (1.0f - t); // t を EaseOut に変換
	return start + (end - start) * t;
}

// デストラクタ
Player::~Player() {}