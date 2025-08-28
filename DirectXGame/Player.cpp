#include "Player.h"
#include <assert.h>
#include <numbers>
#include <cmath>

using namespace KamataEngine;

// === 1) Initialize ===
void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	assert(model);
	assert(camera);

	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f; // 右向き

	rhythm_.secPerBeat = 60.0f / rhythm_.bpm;

}

// === 2) Update（通常処理を集約） ===
void Player::Update() {
	fwCooldown_ = std::max(0.0f, fwCooldown_ - 1.0f / 60.0f);

	const float dt = 1.0f / 60.0f;

	rhythm_.timer += dt;
	while (rhythm_.timer >= rhythm_.secPerBeat) {
		rhythm_.timer -= rhythm_.secPerBeat;
		if (rhythm_.pending) {
			if (fwCooldown_ <= 0.0f) {
				FireByJudge(rhythm_.pendingType, Judge::Perfect); // 予約はPERFECT扱い
				fwCooldown_ = kCD_Rocket;                         // ← CDを消費
				rhythm_.pending = false;
			}
			// CD中なら予約は維持（何もしない）
		}
	}

	    // ★ 兵装切替（Eキー）
	if (Input::GetInstance()->TriggerKey(DIK_E)) {
		CycleFormNext();
	}

	// 入力・移動
	MoveInput();

	// --- 長押し／離し判定 ---
	bool pushX = Input::GetInstance()->PushKey(DIK_K);

	// 押し始め
	if (pushX && !prevPushAttack_ && fwCooldown_ <= 0.0f) {
		attackHolding_ = true;
		holdingType_ = currentForm_;
		rhythm_.holdTimer = 0.0f; // サークルを0から開始
	}

	// ホールド中はタイマー進める
	if (attackHolding_) {
		rhythm_.holdTimer += dt;
	}

// 離した瞬間：まず先に判定→発射
	if (!pushX && prevPushAttack_ && attackHolding_) {
		auto vis = GetRhythmVis();
		float err = std::fabs(vis.ringR - vis.targetR);

		Judge j = (err <= rhythm_.tolPerfectPx) ? Judge::Perfect : (err <= rhythm_.tolGoodPx) ? Judge::Good : Judge::Offbeat;

		FireByJudge(holdingType_, j);
		fwCooldown_ = kCD_Rocket;
		attackHolding_ = false;

		// ★ 角度付きイベントはこの1回だけにする（右 0rad / 左 πrad）
		float angle = (lrDirection_ == LRDirection::kRight) ? 0.0f : std::numbers::pi_v<float>;
		EmitUIEvent_(j, /*hasAnchor=*/true, /*angle=*/angle);
	}



	// ★ここで「少し通り過ぎたら」リセット（まだホールド中なら）
	// Update() 内、holdTimerを進めた後あたり
	if (attackHolding_) {
		const float targetT = rhythm_.secPerBeat * rhythm_.targetOffsetBeats; // 目標時刻
		if (rhythm_.holdTimer > targetT + rhythm_.resetSlackSec) {
			rhythm_.holdTimer = 0.0f; // ループ再開
		}
	}


	prevPushAttack_ = pushX;




	// マップ衝突＆反映
	CollisionMapInfo info;
	info.move = velocity_;
	CheckCollisionMap(info);
	HandleCeilingCollision(info);
	HandleWallCollision(info);
	ApplyCollisionResult(info);
	SwitchLandingState(info);

	// 旋回見た目
	if (turnTimer_ > 0.0f) {
		turnTimer_ -= 1.0f / 60.0f;
		float dstTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
		float dst = dstTable[static_cast<uint32_t>(lrDirection_)];
		float t = 1.0f - (turnTimer_ / kTimeRun);
		worldTransform_.rotation_.y = std::lerp(turnFirstRotationY_, dst, t);
	} else {
		float dstTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};
		worldTransform_.rotation_.y = dstTable[static_cast<uint32_t>(lrDirection_)];
	}

	// 行列更新（攻撃用WTは無し）
	UpdateWorldTransform(worldTransform_);

	invTimer_ = std::max(0.0f, invTimer_ - (1.0f / 60.0f));

// --- 花火の環境衝突 & 画面外処理つき更新 ---
	for (auto& fw : fireworks_) {
		if (fw.life <= 0.0f)
			continue;

		Vector3 nextPos = fw.pos;
		nextPos.x += fw.vel.x * dt;
		nextPos.y += fw.vel.y * dt;

		// 1) ブロック衝突
		if (IsBlocked_(nextPos)) {
			if (fw.type == FireworkType::Burst) {
				// ★ 仕込んでおいたパラメータで予約
				pendingBursts_.push_back({fw.pos, fw.burstCount, fw.burstSpd, fw.burstLife, fw.burstRad});
			}
			fw.life = 0.0f;
			continue;
		}
		// 2) マップ外
		if (IsOutOfWorld_(nextPos, 1.0f)) {
			fw.life = 0.0f;
			continue;
		}

		fw.pos = nextPos;
		fw.pos.z = 0.0f;
		fw.life -= dt;
	}

	// 予約をまとめて爆散
	if (!pendingBursts_.empty()) {
		for (const auto& r : pendingBursts_) {
			BurstExplode_(r.pos, r.num, r.spd, r.life, r.rad);
		}
		pendingBursts_.clear();
	}


	// 寿命切れを削除
	fireworks_.erase(std::remove_if(fireworks_.begin(), fireworks_.end(), [](const Firework& f) { return f.life <= 0.0f; }), fireworks_.end());
}

void Player::Draw() {
	// 無敵中は点滅（20Hz相当）
	if (invTimer_ > 0.0f) {
		int blink = static_cast<int>(invTimer_ * 20.0f) & 1;
		if (blink == 0) {
			return;
		} // 1フレームおきに非表示
	}
	model_->Draw(worldTransform_, *camera_);
}

void Player::MoveInput() {
	const float dt = 1.0f / 60.0f;

	// 先行入力
	bool jumpTrig = Input::GetInstance()->TriggerKey(DIK_UP) || Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->TriggerKey(DIK_Z);
	if (jumpTrig)
		jumpBuffer_ = kJumpBufferTime;
	else
		jumpBuffer_ = std::max(0.0f, jumpBuffer_ - dt);

	// コヨーテ
	if (onGround_)
		coyoteTimer_ = kCoyoteTime;
	else
		coyoteTimer_ = std::max(0.0f, coyoteTimer_ - dt);

	// ジャンプ開始
	if ((onGround_ || coyoteTimer_ > 0.0f) && jumpBuffer_ > 0.0f) {
		velocity_.y = kJumpAcceleration;
		onGround_ = false;
		coyoteTimer_ = 0.0f;
		jumpBuffer_ = 0.0f;
	}

	// --- 横移動 ---
	if (onGround_) {
		if (Input::GetInstance()->PushKey(DIK_D) || Input::GetInstance()->PushKey(DIK_A)) {
			Vector3 acc{};
			if (Input::GetInstance()->PushKey(DIK_D)) {
				if (velocity_.x < 0.0f)
					velocity_.x *= (1.0f - kAttenuation);
				acc.x += kAcceleration;
				if (lrDirection_ != LRDirection::kRight) {
					lrDirection_ = LRDirection::kRight;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTimeRun;     
				}
			} else if (Input::GetInstance()->PushKey(DIK_A)) {
				if (velocity_.x > 0.0f)
					velocity_.x *= (1.0f - kAttenuation);
				acc.x -= kAcceleration;
				if (lrDirection_ != LRDirection::kLeft) {
					lrDirection_ = LRDirection::kLeft;
					turnFirstRotationY_ = worldTransform_.rotation_.y;
					turnTimer_ = kTimeRun;     
				}
			}
			velocity_ += acc;
			velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
		} else {
			velocity_.x *= (1.0f - kAttenuation);
		}
	} else {
		// 空中
		velocity_ += Vector3(0, -kGravityAcceleration, 0);
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
	}

	// ★ 可変ジャンプ：キー離しで上昇をカット（←分岐の外に置く）
	bool jumpHeld = Input::GetInstance()->PushKey(DIK_W) || Input::GetInstance()->PushKey(DIK_SPACE) || Input::GetInstance()->PushKey(DIK_UP);
	if (!jumpHeld && !onGround_ && velocity_.y > 0.0f) {
		velocity_.y *= 0.55f; // 好みに応じて 0.45〜0.65
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
	{
		const uint32_t leftX = (indexSet.xIndex == 0) ? 0u : (indexSet.xIndex - 1);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(leftX, indexSet.yIndex); // ← 左隣を安全に参照
	}
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
	{
		const uint32_t leftX = (indexSet.xIndex == 0) ? 0u : (indexSet.xIndex - 1);
		mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(leftX, indexSet.yIndex);
	}
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
	if (!enemy)
		return;
	AABB ea = enemy->GetAABB();
	Vector3 ep{(ea.min.x + ea.max.x) * 0.5f, (ea.min.y + ea.max.y) * 0.5f, (ea.min.z + ea.max.z) * 0.5f};
	ApplyDamage(1, &ep); // ← こちらに統一
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

// Player::SpawnRocket_() を差し替え（中身だけでOK）
void Player::SpawnRocket_() {
	// 向きベクトル（左右）
	KamataEngine::Vector3 dir = {std::sin(worldTransform_.rotation_.y), 0.0f, 0.0f};
	if (std::abs(dir.x) < 0.1f) {
		dir.x = (lrDirection_ == LRDirection::kRight) ? +1.0f : -1.0f;
	}

	// プレイヤーのサイズ基準で発射位置を作る
	const float muzzleX = ((lrDirection_ == LRDirection::kRight) ? +1.0f : -1.0f) * kMuzzleForward;
	const float muzzleY = kMuzzleYOffset; // 手の高さっぽく

	Firework fw;
	fw.pos = {worldTransform_.translation_.x + muzzleX, worldTransform_.translation_.y + muzzleY, 0.0f};
	fw.vel = dir * kFW_Speed;
	fw.life = kFW_Life;

	// 見た目と判定の「体感」を合わせる
	fw.radius = 0.65f; // ← 当たり半径（見た目と揃える）
	fw.type = FireworkType::Rocket;

	fireworks_.push_back(fw);
}

void Player::FireByJudge(FireworkType type, Judge j) {
	switch (type) {
	case FireworkType::Rocket:
	default:
		SpawnRocketRated_(j);
		break;
	case FireworkType::Burst:
		SpawnBurstRated_(j);
		break;
	case FireworkType::Pierce:
		SpawnPierceRated_(j);
		break;
	}
}



void Player::SpawnRocketRated_(Judge j) {
	Vector3 dir = {std::sin(worldTransform_.rotation_.y), 0.0f, 0.0f};
	if (std::abs(dir.x) < 0.1f)
		dir.x = (lrDirection_ == LRDirection::kRight) ? +1.0f : -1.0f;

	const float muzzleX = ((lrDirection_ == LRDirection::kRight) ? +1.0f : -1.0f) * kMuzzleForward;
	const float muzzleY = kMuzzleYOffset;

	float spd = kFW_Speed;
	float life = kFW_Life;
	float rad = 0.65f;

	// ★威力を判定で上げる
	int dmg = 1;
	switch (j) {
	case Judge::Perfect:
		spd *= 1.15f;
		life *= 1.10f;
		rad += 0.10f;
		dmg = 3;
		break;
	case Judge::Good:
		spd *= 1.05f;
		rad += 0.05f;
		dmg = 2;
		break;
	case Judge::Offbeat:
		spd *= 0.90f;
		life *= 0.90f;
		dmg = 1;
		break;
	}

	Firework fw;
	fw.pos = {worldTransform_.translation_.x + muzzleX, worldTransform_.translation_.y + muzzleY, 0.0f};
	fw.vel = dir * spd;
	fw.life = life;
	fw.radius = rad;
	fw.type = FireworkType::Rocket;
	fw.damage = dmg; // ←ココ
	fw.tier = 1;

	fireworks_.push_back(fw);
}



Player::RhythmVis Player::GetRhythmVis() const {
	// 1拍ぶんの時間で rStart→rEnd に到達（=伸び切る）
	float dur = rhythm_.secPerBeat;
	float t01 = std::clamp(rhythm_.holdTimer / dur, 0.0f, 1.0f);

	// 伸び方は気持ち良い EaseOut
	float ease = 1.0f - (1.0f - t01) * (1.0f - t01);
	float ringR = rhythm_.rStart + (rhythm_.rEnd - rhythm_.rStart) * ease;

	// 目標半径：targetOffsetBeats（通常1拍後）に相当する位置
	float tgt01 = std::clamp(rhythm_.targetOffsetBeats, 0.0f, 1.0f);
	float targetR = rhythm_.rStart + (rhythm_.rEnd - rhythm_.rStart) * tgt01;

	return {ringR, targetR, t01};
}

bool Player::IsBlocked_(const KamataEngine::Vector3& p) const {
	if (!mapChipField_)
		return false;

	auto idx = mapChipField_->GetMapChipIndexSetByPosition(p);
	if (mapChipField_->GetMapChipTypeByIndex(idx.xIndex, idx.yIndex) != MapChipType::kBlock) {
		return false;
	}

	// セル矩形を取得して “縁” をスルー（境界触れただけでヒットにしない）
	auto rc = mapChipField_->GetRectByIndex(idx.xIndex, idx.yIndex);
	const float margin = 0.05f; // 好みで 0.03〜0.10

	bool insideX = (p.x > rc.left + margin) && (p.x < rc.right - margin);
	bool insideY = (p.y > rc.bottom + margin) && (p.y < rc.top - margin);

	return insideX && insideY; // 完全に内側に入った時だけ衝突扱い
}


bool Player::IsOutOfWorld_(const KamataEngine::Vector3& p, float margin) const {
	if (!mapChipField_)
		return false;
	KamataEngine::Vector3 bl = mapChipField_->GetMapChipPositionByIndex(0, mapChipField_->GetNumVirtical() - 1);
	KamataEngine::Vector3 tr = mapChipField_->GetMapChipPositionByIndex(mapChipField_->GetNumHorizontal() - 1, 0);
	return (p.x < bl.x - margin) || (p.x > tr.x + margin) || (p.y < bl.y - margin) || (p.y > tr.y + margin);
}

void Player::TakeHit_(const KamataEngine::Vector3& fromPos) {
	if (invTimer_ > 0.0f || isDead_)
		return;

	// HPを減らす
	if (--hp_ <= 0) {
		isDead_ = true;
		return;
	}

	// 方向判定（相手の位置→自分の位置の向きにノックバック）
	float dir = (worldTransform_.translation_.x >= fromPos.x) ? +1.0f : -1.0f;
	velocity_.x = kKnockX * dir;
	velocity_.y = kKnockY;
	onGround_ = false;

	// 無敵時間
	invTimer_ = kIFrameSec;
}

void Player::SyncTransformOnly() { UpdateWorldTransform(worldTransform_); }

void Player::ApplyDamage(int dmg, const KamataEngine::Vector3* fromWorldPos) {
	if (isDead_ || invTimer_ > 0.0f)
		return; // 無敵中/死亡中は無効

	hp_ -= std::max(1, dmg);
	invTimer_ = kIFrameSec; // 無敵付与

	// ノックバック（被弾元の方向へ押し返す）
	if (fromWorldPos) {
		KamataEngine::Vector3 me = GetWorldPosition();
		float dx = me.x - fromWorldPos->x;
		float dir = (dx >= 0.0f) ? +1.0f : -1.0f; // 右向き=+1, 左向き=-1（押し返す）
		velocity_.x += dir * kKnockX;
		velocity_.y = std::max(velocity_.y, kKnockY);
	} else {
		// 方向情報がないときの最小ノックバック
		velocity_.y = std::max(velocity_.y, kKnockY * 0.7f);
	}

	if (hp_ <= 0) {
		isDead_ = true;
		// 必要ならここで死亡演出トリガなど
	}
}

void Player::CycleFormNext() {
	switch (currentForm_) {
	case FireworkType::Rocket:
		currentForm_ = FireworkType::Burst;
		break;
	case FireworkType::Burst:
		currentForm_ = FireworkType::Pierce;
		break;
	case FireworkType::Pierce:
		currentForm_ = FireworkType::Rocket;
		break;
	}
	// 簡易表示（任意）
	// DebugText::GetInstance()->ConsolePrintf("Form: %d\n", (int)currentForm_);
}

void Player::SpawnBurstRated_(Judge j) {
	Vector3 dir = {std::sin(worldTransform_.rotation_.y), 0.0f, 0.0f};
	if (std::abs(dir.x) < 0.1f)
		dir.x = (lrDirection_ == LRDirection::kRight) ? +1.0f : -1.0f;
	const float muzzleX = ((lrDirection_ == LRDirection::kRight) ? +1.0f : -1.0f) * kMuzzleForward;
	const float muzzleY = kMuzzleYOffset;

	float spd = kFW_Speed * 0.90f; // 本体はやや遅め
	float life = kFW_Life * 0.95f;
	float rad = 0.60f;

	// ★爆散パラメータ
	int count = 12;
	float bspd = 12.0f;
	float blife = 0.45f;
	float brad = 0.50f;
	uint8_t tier = 1;

	switch (j) {
	case Judge::Perfect:
		spd *= 1.10f;
		life *= 1.10f;
		rad += 0.10f;
		count = 26;
		bspd = 14.0f;
		blife = 0.52f;
		brad = 0.55f;
		tier = 2;
		break;
	case Judge::Good:
		spd *= 1.05f;
		rad += 0.05f;
		count = 18;
		bspd = 12.5f;
		blife = 0.48f;
		brad = 0.52f;
		tier = 1;
		break;
	case Judge::Offbeat:
		count = 10;
		bspd = 11.0f;
		blife = 0.40f;
		brad = 0.45f;
		tier = 0;
		break;
	}

	Firework fw;
	fw.pos = {worldTransform_.translation_.x + muzzleX, worldTransform_.translation_.y + muzzleY, 0.0f};
	fw.vel = dir * spd;
	fw.life = life;
	fw.radius = rad;
	fw.type = FireworkType::Burst;
	fw.damage = 1;
	fw.tier = tier;
	fw.burstCount = count;
	fw.burstSpd = bspd;
	fw.burstLife = blife;
	fw.burstRad = brad;

	fireworks_.push_back(fw);
}


void Player::SpawnPierceRated_(Judge j) {
	Vector3 dir = {std::sin(worldTransform_.rotation_.y), 0.0f, 0.0f};
	if (std::abs(dir.x) < 0.1f)
		dir.x = (lrDirection_ == LRDirection::kRight) ? +1.0f : -1.0f;
	const float muzzleX = ((lrDirection_ == LRDirection::kRight) ? +1.0f : -1.0f) * kMuzzleForward;
	const float muzzleY = kMuzzleYOffset;

	float spd = kFW_Speed * 1.10f;
	float life = kFW_Life * 1.35f; // 貫通距離も伸ばす
	float rad = 0.55f;

	switch (j) {
	case Judge::Perfect:
		spd *= 1.50f;
		life *= 1.10f;
		break; // 速度ドン！
	case Judge::Good:
		spd *= 1.25f;
		break;
	case Judge::Offbeat:
		spd *= 0.95f;
		life *= 0.90f;
		break;
	}

	Firework fw;
	fw.pos = {worldTransform_.translation_.x + muzzleX, worldTransform_.translation_.y + muzzleY, 0.0f};
	fw.vel = dir * spd;
	fw.life = life;
	fw.radius = rad;
	fw.type = FireworkType::Pierce;
	fw.damage = 1; // 貫通は固定1でOK（必要なら調整）
	fw.tier = 1;

	fireworks_.push_back(fw);
}


void Player::BurstExplode_(const Vector3& center, int num, float spd, float life, float rad) {
	const float twoPi = 6.28318530718f;
	for (int i = 0; i < num; ++i) {
		float ang = twoPi * (float)i / (float)num;
		Firework s;
		s.pos = center;
		s.vel = {std::cos(ang) * spd, std::sin(ang) * spd, 0.0f};
		s.life = life;
		s.radius = rad;
		s.type = FireworkType::Rocket; // 小弾は通常弾の挙動（敵ヒットで消える）
		s.damage = 1;                  // 子弾のダメージ（必要なら2に）
		fireworks_.push_back(s);
	}
}




// デストラクタ
Player::~Player() {}