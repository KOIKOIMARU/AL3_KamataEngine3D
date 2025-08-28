#include "Enemy.h"
#include <assert.h>
#include <cmath> // ★std::lerp 用
#include <numbers>

using namespace KamataEngine;

void Enemy::Initialize(Model* model, Camera* camera, const Vector3& position) {
	assert(model);
	assert(camera);
	model_ = model;
	camera_ = camera;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / -2.0f;

	// 初期速度（フレーム単位）
	vel_ = {-0.0f, 0.0f, 0.0f};

	walkTimer_ = (rand() / (float)RAND_MAX) * kWalkMotionTime;

	hp_ = 5;
	dead_ = false;

	patrolCenterX_ = worldTransform_.translation_.x;
   
	sizeScale_ = 1.0f; // 通常は等倍
	worldTransform_.scale_ = {sizeScale_, sizeScale_, sizeScale_};
}

void Enemy::Update() {
	if (state_ == State::Dead || dead_)
		return;

	const float dt = 1.0f / 60.0f;

	// 重力
	vel_.y += gravity_;


    if (isBoss_) {
		TickBoss_(dt); // ← ボス専用AI
		// 向きは基本プレイヤー方向
		if (target_) {
			float dx = target_->GetWorldPosition().x - worldTransform_.translation_.x;
			if (std::fabs(dx) > 1e-4f)
				dir_ = (dx >= 0.0f) ? +1 : -1;
		} else if (std::fabs(vel_.x) > 1e-6f) {
			dir_ = (vel_.x >= 0.0f) ? +1 : -1;
		}
	} else {
		// 既存の通常敵ロジック
		switch (state_) {
		case State::Patrol:
			TickPatrol_(dt);
			break;
		case State::Chase:
			TickChase_(dt);
			break;
		case State::Attack:
			TickAttack_(dt);
			break;
		case State::Stunned:
			break;
		default:
			break;
		}
	}

	
	// 歩行見た目
	walkTimer_ += dt;
	{
		float param = std::sin(2.0f * std::numbers::pi_v<float> * walkTimer_ / kWalkMotionTime);
		float t = (param + 1.0f) * 0.5f;
		float degree = std::lerp(kWalkMotionAngleStart, kWalkMotionAngleEnd, t);
		worldTransform_.rotation_.x = degree * (std::numbers::pi_v<float> / 180.0f);
	}

	// ヒット点滅
	if (hitFlash_) {
		hitFlashTimer_ -= dt;
		if (hitFlashTimer_ <= 0.0f)
			hitFlash_ = false;
	}

	Integrate_(dt);
	CollideWithMap_();

	// ★最終的な向き決定（無条件で回す）
	// ★最終的な向き決定
	if (!isBoss_) {
		int wantDir = dir_;
		if (state_ == State::Chase || state_ == State::Attack) {
			if (target_) {
				float dx = target_->GetWorldPosition().x - worldTransform_.translation_.x;
				if (std::fabs(dx) > 1e-4f)
					wantDir = (dx >= 0.0f) ? +1 : -1;
			}
		} else {
			if (std::fabs(vel_.x) > 1e-4f)
				wantDir = (vel_.x >= 0.0f) ? +1 : -1;
		}
		dir_ = wantDir;
	}

	worldTransform_.rotation_.y = (dir_ > 0) ? +std::numbers::pi_v<float> / 2.0f : -std::numbers::pi_v<float> / 2.0f;

	UpdateWorldTransform(worldTransform_);
}

void Enemy::Draw() { model_->Draw(worldTransform_, *camera_); }

Vector3 Enemy::GetWorldPosition() const {
	Vector3 worldPos;
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];
	return worldPos;
}

AABB Enemy::GetAABB() const {
	Vector3 pos = GetWorldPosition();
	float halfW = (kWidth * 0.5f) * sizeScale_;
	float halfH = (kHeight * 0.5f) * sizeScale_;
	return {
	    {pos.x - halfW, pos.y - halfH, pos.z - halfW},
        {pos.x + halfW, pos.y + halfH, pos.z + halfW}
    };
}

void Enemy::CollideWithMap_() {
	if (!map_)
		return;

	const float halfW = (kWidth * 0.5f) * sizeScale_;
	const float halfH = (kHeight * 0.5f) * sizeScale_;
	const float eps = 0.001f;

	const auto& pos = worldTransform_.translation_;

	// 足元
	auto footIS = map_->GetMapChipIndexSetByPosition({pos.x, pos.y - (halfH + 0.02f), 0});
	if (map_->SafeGet(footIS.xIndex, footIS.yIndex) == MapChipType::kBlock) {
		auto r = map_->GetRectByIndex(footIS.xIndex, footIS.yIndex);
		worldTransform_.translation_.y = r.top + halfH + eps;
		vel_.y = 0.0f;
		onGround_ = true;
	} else {
		onGround_ = false;
	}

	// 横
	float dirf = (vel_.x >= 0.0f) ? +1.0f : -1.0f;
	if (std::fabs(vel_.x) > 1e-6f) {
		float probeX = pos.x + dirf * (halfW + 0.02f);
		Vector3 pTop{probeX, pos.y + halfH * 0.35f, 0};
		Vector3 pBot{probeX, pos.y - halfH * 0.35f, 0};
		auto topIS = map_->GetMapChipIndexSetByPosition(pTop);
		auto botIS = map_->GetMapChipIndexSetByPosition(pBot);
		bool hitTop = (map_->SafeGet(topIS.xIndex, topIS.yIndex) == MapChipType::kBlock);
		bool hitBot = (map_->SafeGet(botIS.xIndex, botIS.yIndex) == MapChipType::kBlock);

		if (hitTop || hitBot) {
			auto is = hitTop ? topIS : botIS;
			auto rc = map_->GetRectByIndex(is.xIndex, is.yIndex);
			if (dirf > 0)
				worldTransform_.translation_.x = rc.left - halfW - eps;
			else
				worldTransform_.translation_.x = rc.right + halfW + eps;
			vel_.x = 0.0f;

			// 突進中に壁で止まったら終点ばら撒き
			if (isBoss_ && bossPhase_ == BossPhase::DashRun) {
				FireRadial_(worldTransform_.translation_, /*num=*/6, /*spd=*/0.20f); // ★ 10→6発、0.24→0.20
				bossPhase_ = BossPhase::Idle;
				phaseT_ = 0.0f;
				bossRest_ = 0.45f; // ★ 休憩追加
			}


			if (!isBoss_ && state_ == State::Patrol) {
				dir_ *= -1;
			}

		}
	}
}


void Enemy::OnCollision(const Player*) {}

void Enemy::ApplyDamage(int damage) {
	if (dead_)
		return;
	hp_ -= std::max(1, damage);

	// ノックバック（簡易）
	vel_.x += (vel_.x >= 0 ? -1.0f : 1.0f) * 0.8f;

	// 点滅
	hitFlash_ = true;
	hitFlashTimer_ = 0.10f;

	if (hp_ <= 0) {
		dead_ = true;
		state_ = State::Dead;
		if (onKilled_)
			onKilled_(worldTransform_.translation_); // ★
	}
}

void Enemy::SyncTransformOnly() { UpdateWorldTransform(worldTransform_); }

// --- State ticks ---
void Enemy::TickPatrol_(float dt) {
	(void)dt;
	vel_.x = dir_ * speedPatrol_;

	float x = worldTransform_.translation_.x;
	if (x < (patrolCenterX_ - patrolHalf_))
		dir_ = +1;
	if (x > (patrolCenterX_ + patrolHalf_))
		dir_ = -1;

	  if (kind_ == EnemyKind::Hopper) {
		hopTimer_ -= dt;
		// 地上かつクールダウン終了なら小ジャンプ
		if (onGround_ && hopTimer_ <= 0.0f) {
			vel_.y = params_.hopPower;
			hopTimer_ = params_.hopCooldown;
		}
	}


	if (map_) {
		const float lookAhead = 0.6f;
		const float footDown = 0.51f;
		const float dirf = (dir_ >= 0 ? 1.f : -1.f);

		const auto& pos = worldTransform_.translation_; // ★ ここ
		auto footIS = map_->GetMapChipIndexSetByPosition({pos.x + dirf * lookAhead, pos.y - footDown, 0});
		auto bodyIS = map_->GetMapChipIndexSetByPosition({pos.x + dirf * lookAhead, pos.y, 0});

		bool hole = (map_->SafeGet(footIS.xIndex, footIS.yIndex) != MapChipType::kBlock);
		bool wall = (map_->SafeGet(bodyIS.xIndex, bodyIS.yIndex) == MapChipType::kBlock);

		if (hole || wall) {
			dir_ *= -1;
			vel_.x = dir_ * speedPatrol_;
			
		}
	}

	if (target_) {
		auto tp = target_->GetWorldPosition();
		const auto& me = worldTransform_.translation_; // ★ ここも
		float dx = tp.x - me.x, dy = tp.y - me.y;
		if (dx * dx + dy * dy <= aggroRange_ * aggroRange_)
			state_ = State::Chase;
	}
}


void Enemy::TickChase_(float dt) {
	if (!target_) {
		state_ = State::Patrol;
		return;
	}

	auto tp = target_->GetWorldPosition();
	auto me = worldTransform_.translation_;
	float dx = tp.x - me.x;
	float dy = tp.y - me.y;
	float d2 = dx * dx + dy * dy;

	if (kind_ == EnemyKind::Shooter) {
		float prefer = params_.preferRange;
		// 近すぎ→離れる、遠すぎ→近づく、ちょうど→停止
		if (std::fabs(dx) > 1e-4f) {
			if (std::sqrt(d2) > prefer + 0.4f)
				vel_.x = (dx >= 0 ? +1.f : -1.f) * params_.speedChase;
			else if (std::sqrt(d2) < prefer - 0.4f)
				vel_.x = (dx >= 0 ? -1.f : +1.f) * params_.speedChase;
			else
				vel_.x = 0.0f;
		}

		// 視線が通る＆クールダウン0で射撃
		shootCD_ = std::max(0.0f, shootCD_ - 1.0f / 60.0f);
		if (shootCD_ <= 0.0f && map_ /*LOS判定*/) {
			// 簡易LOS：敵とプレイヤの間を数ステップでサンプリング
			bool blocked = false;
			int steps = 12;
			for (int i = 1; i <= steps; i++) {
				float t = i / (float)steps;
				Vector3 p{me.x + dx * t, me.y + dy * t, 0};
				auto is = map_->GetMapChipIndexSetByPosition(p);
				if (map_->SafeGet(is.xIndex, is.yIndex) == MapChipType::kBlock) {
					blocked = true;
					break;
				}
			}
			if (!blocked && onShoot_) {
				Vector3 dir{(dx >= 0 ? 1.f : -1.f), 0, 0};
				onShoot_(Vector3{me.x, me.y, 0}, dir, params_.projectileSpeed);
				shootCD_ = params_.shootCooldown;
			}
		}

		// 近すぎたら「攻撃状態」にしない（射撃はChase中で行う）
		if (d2 > (aggroRange_ * aggroRange_ * 1.5f))
			state_ = State::Patrol;
		return;
	}

	 // Walker/Hopper 共通の接近
	vel_.x = (dx >= 0 ? +1.f : -1.f) * params_.speedChase;

	// ★Hopper は追跡中もホッピング
	if (kind_ == EnemyKind::Hopper) {
		hopTimer_ -= dt;
		if (onGround_ && hopTimer_ <= 0.0f) {
			vel_.y = params_.hopPower;
			hopTimer_ = params_.hopCooldown;
		}
	}
	if (d2 <= attackRange_ * attackRange_) {
		state_ = State::Attack;
		atkTimer_ = atkWindup_ + atkCooldown_;
		vel_.x = 0.0f;
	}
	if (d2 > (aggroRange_ * aggroRange_ * 1.5f))
		state_ = State::Patrol;
}


void Enemy::TickAttack_(float dt) {
	atkTimer_ = std::max(0.0f, atkTimer_ - dt);
	vel_.x = 0.0f;
	if (atkTimer_ <= 0.0f)
		state_ = State::Chase;
}

void Enemy::Integrate_(float) {
	worldTransform_.translation_.x += vel_.x;
	worldTransform_.translation_.y += vel_.y;
}



static EnemyParams kPresets[] = {
    /* Walker */ {.hp = 5, .speedPatrol = 0.013f, .speedChase = 0.030f, .gravity = -0.015f},
    /* Hopper */
     {.hp = 4, .speedPatrol = 0.012f, .speedChase = 0.026f, .gravity = -0.017f, .hopPower = 0.32f, .hopCooldown = 0.8f},
    /* Shooter*/
     {.hp = 3, .speedPatrol = 0.010f, .speedChase = 0.022f, .gravity = -0.015f, .preferRange = 3.5f, .shootCooldown = 1.0f, .projectileSpeed = 0.22f},
};

void Enemy::SetKind(EnemyKind k) {
	kind_ = k;

	// 例：Walker
	if (kind_ == EnemyKind::Walker) {
		params_ = EnemyParams{}; // いったんゼロ初期化
		params_.hp = 5;
		params_.speedPatrol = 0.013f;
		params_.speedChase = 0.030f;
		params_.gravity = -0.015f;
		params_.hopPower = 0.28f;
		params_.hopCooldown = 0.9f;
		params_.preferRange = 3.0f;
		params_.shootCooldown = 1.2f;
		params_.projectileSpeed = 0.18f;
	} else if (kind_ == EnemyKind::Hopper) {
		params_ = EnemyParams{};
		params_.hp = 4;
		params_.speedPatrol = 0.010f;
		params_.speedChase = 0.028f;
		params_.gravity = -0.018f;
		params_.hopPower = 0.34f;
		params_.hopCooldown = 0.7f;
		params_.preferRange = 2.0f;
		params_.shootCooldown = 1.2f;
		params_.projectileSpeed = 0.18f;
	} else { // Shooter
		params_ = EnemyParams{};
		params_.hp = 3;
		params_.speedPatrol = 0.010f;
		params_.speedChase = 0.022f;
		params_.gravity = -0.015f;
		params_.preferRange = 3.5f;
		params_.shootCooldown = 1.0f;
		params_.projectileSpeed = 0.22f;
	}

	// 既存フィールドへ反映
	hp_ = params_.hp;
	speedPatrol_ = params_.speedPatrol;
	speedChase_ = params_.speedChase;
	gravity_ = params_.gravity;

	hopTimer_ = 0.0f;
	shootCD_ = 0.0f;
}

// Enemy::SetBoss(bool f)
void Enemy::SetBoss(bool f, float scale) {
	isBoss_ = f;
	if (isBoss_) {
		onShoot_ = {}; // ボス弾OFFのまま
		sizeScale_ = scale; // 指定倍率で拡大
		worldTransform_.scale_ = {sizeScale_, sizeScale_, sizeScale_};
		hp_ = 50;
		gravity_ = -0.018f;

		dashSpeed_ = 0.12f;
		dashDur_ = 0.70f;
		hopVy_ = 0.46f;
		hopHSpeed_ = 0.06f; // ★ここ

		// 出現直後に動く
		bossPhase_ = BossPhase::DashPrep;
		phaseT_ = 0.0f;
	} else {
		sizeScale_ = 1.0f;
		worldTransform_.scale_ = {1, 1, 1};
	}
}



void Enemy::TickBoss_(float dt) {
	phaseT_ += dt;

	// ★ パターン間の休憩（完全無行動）
	if (bossRest_ > 0.0f) {
		bossRest_ = std::max(0.0f, bossRest_ - dt);
		vel_.x = 0.0f;
		return;
	}

	auto me = worldTransform_.translation_;
	Vector3 tp = target_ ? target_->GetWorldPosition() : Vector3{me.x + (float)dir_, me.y, 0};

	switch (bossPhase_) {

// Enemy::TickBoss_(float dt) の Idle 分岐内
	case BossPhase::Idle: {
		vel_.x = 0.0f;
		if (phaseT_ < 0.40f)
			return;

		static int seq = 0;
		int s = seq++ % 3;

		// ★弾を使わない場合は Volley を飛ばして Dash/Hop だけ回す
		bool canShoot = (bool)onShoot_;
		if (!canShoot) {
			// 0:DashPrep, 1:HopPrep を交互
			s = (seq & 1) ? 1 : 2; // 1→DashPrep, 2→HopPrep
		}

		if (s == 0) {
			bossPhase_ = BossPhase::Volley; // ← canShoot 時のみ到達
			volleyLeft_ = 4;
		} else if (s == 1) {
			bossPhase_ = BossPhase::DashPrep;
		} else {
			bossPhase_ = BossPhase::HopPrep;
		}
		phaseT_ = 0.0f;
		break;
	}

	case BossPhase::Volley: {
		vel_.x = 0.0f;
		// ★ 間隔を長めに＆1回の散弾を軽く
		const float interval = 0.28f; // 0.20→0.28
		if (phaseT_ >= interval && volleyLeft_ > 0) {
			phaseT_ = 0.0f;
			--volleyLeft_;
			// ★ 5発→3発、±30°、速度も低め
			const float deg = 30.0f * 3.14159265f / 180.0f;
			FireSpreadTowards_(tp, /*count=*/3, /*spreadRad=*/deg, /*spd=*/0.22f);
		}
		if (volleyLeft_ <= 0 && phaseT_ > 0.35f) {
			bossPhase_ = BossPhase::Idle;
			phaseT_ = 0.0f;
			bossRest_ = 0.45f; // ★ 終了後に休憩
		}
		break;
	}

	case BossPhase::DashPrep: {
		vel_.x = 0.0f;
		if (target_)
			dashDir_ = (tp.x >= me.x) ? +1 : -1;
		if (phaseT_ >= 0.45f) { // ★ 0.30→0.45（予備動作長め）
			bossPhase_ = BossPhase::DashRun;
			phaseT_ = 0.0f;
		}
		break;
	}

	case BossPhase::DashRun: {
		vel_.x = dashDir_ * dashSpeed_;
		if (phaseT_ >= dashDur_) {
			// ★ リング弾も控えめ
			FireRadial_(me, /*num=*/6, /*spd=*/0.20f); // 10→6, 0.24→0.20
			bossPhase_ = BossPhase::Idle;
			phaseT_ = 0.0f;
			bossRest_ = 0.45f; // ★ 休憩
		}
		break;
	}

	case BossPhase::HopPrep: {
		vel_.x = 0.0f;
		if (phaseT_ >= 0.25f) { // 軽い溜め
			// ★ジャンプ直前に水平方向を決めて速度付与
			if (target_) {
				float dx = target_->GetWorldPosition().x - me.x;
				dashDir_ = (dx >= 0.0f) ? +1 : -1;
			}
			vel_.x = dashDir_ * hopHSpeed_; // ★水平始動
			vel_.y = hopVy_;                // 上へ
			bossPhase_ = BossPhase::HopUp;
			phaseT_ = 0.0f;
		}
		break;
	}

	case BossPhase::HopUp: {
		// ★上昇中も常に“いまのプレイヤー方向”へ水平移動
		if (target_) {
			float dx = target_->GetWorldPosition().x - me.x;
			int dir = (std::fabs(dx) > 1e-4f) ? ((dx >= 0.0f) ? +1 : -1) : dashDir_;
			vel_.x = dir * hopHSpeed_;
		}
		if (vel_.y <= 0.0f) {
			bossPhase_ = BossPhase::HopFall;
			phaseT_ = 0.0f;
		}
		break;
	}

	case BossPhase::HopFall: {
		// ★落下中も同様に追尾し続ける
		if (target_) {
			float dx = target_->GetWorldPosition().x - me.x;
			int dir = (std::fabs(dx) > 1e-4f) ? ((dx >= 0.0f) ? +1 : -1) : dashDir_;
			vel_.x = dir * hopHSpeed_;
		}
		if (onGround_) {
			// （弾OFFなのでFireRadial_は視覚だけ。必要ならここに画面揺れ/SE等を後で追加）
			bossPhase_ = BossPhase::StompRecover;
			phaseT_ = 0.0f;
		}
		break;
	}


	case BossPhase::StompRecover: {
		vel_.x = 0.0f;
		if (phaseT_ >= 0.50f) { // ★ 0.35→0.50（硬直少し長め＝チャンス）
			bossPhase_ = BossPhase::Idle;
			phaseT_ = 0.0f;
			bossRest_ = 0.50f; // ★ 次の行動まで休憩
		}
		break;
	}
	}
}

void Enemy::FireSpreadTowards_(const Vector3& target, int count, float spreadRad, float spd) {
	if (!onShoot_)
		return;
	Vector3 me = worldTransform_.translation_;

	float dx = target.x - me.x, dy = target.y - me.y;
	float baseAng = std::atan2(dy, dx);

	if (count <= 1) { // ガード
		Vector3 dir{std::cos(baseAng), std::sin(baseAng), 0};
		onShoot_(me, dir, spd);
		return;
	}

	int mid = count / 2;
	for (int i = 0; i < count; ++i) {
		float step = (mid != 0) ? (spreadRad / (float)mid) : 0.0f;
		float ang = baseAng + (float)(i - mid) * step;
		Vector3 dir{std::cos(ang), std::sin(ang), 0};
		onShoot_(me, dir, spd);
	}
}


void Enemy::FireRadial_(const Vector3& center, int num, float spd) {
	if (!onShoot_)
		return;
	const float twoPi = 6.28318530718f;
	for (int i = 0; i < num; ++i) {
		float ang = twoPi * (float)i / (float)num;
		Vector3 dir{std::cos(ang), std::sin(ang), 0};
		onShoot_(center, dir, spd);
	}
}


// dtor
Enemy::~Enemy() = default;
