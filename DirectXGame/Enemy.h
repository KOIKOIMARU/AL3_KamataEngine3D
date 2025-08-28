#pragma once
#include "KamataEngine.h"
#include "Player.h" 
#include "MapChipField.h"
#include "Transform.h"
#define NOMINMAX
#include <algorithm>
#include <functional>
#include <cstdint>

#undef min
#undef max

class MapChipField;
class Player;

	struct EnemyParams {
	int hp = 5;
	float speedPatrol = 0.013f;
	float speedChase = 0.030f;
	float gravity = -0.015f;
	// Hopper
	float hopPower = 0.28f;   // ジャンプ力
	float hopCooldown = 0.9f; // ジャンプ間隔
	// Shooter
	float preferRange = 3.0f; // これくらいの距離を保つ
	float shootCooldown = 1.2f;
	float projectileSpeed = 0.18f; // タイル/フレーム相当
};


class Enemy {
public:
	~Enemy();

	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);
	void Update();
	void Draw();

	KamataEngine::Vector3 GetWorldPosition() const;
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }
	AABB GetAABB() const;

	void OnCollision(const Player* player);
	void Kill() { dead_ = true; }
	bool IsDead() const { return dead_; }

	void ApplyDamage(int damage = 1);

	void SetOnKilledCallback(std::function<void(const KamataEngine::Vector3&)> cb) { onKilled_ = std::move(cb); }

	void SetMap(MapChipField* m) { map_ = m; }
	void SetTarget(Player* p) { target_ = p; } // ★追加
	// 既存 public を置き換え（中は変更）
	void SetBoss(bool f, float scale = 1.8f);


	void SyncTransformOnly();

	enum class EnemyKind : uint8_t { Walker = 0, Hopper = 1, Shooter = 2, /*あとで増やせる*/ };


	void SetKind(EnemyKind k);

	EnemyKind GetKind() const { return kind_; }

	// （オプション）弾生成コールバック
	void SetOnShootCallback(std::function<void(const KamataEngine::Vector3& pos, const KamataEngine::Vector3& dir, float speed)> cb) { onShoot_ = std::move(cb); }


private:
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;

	// フレーム単位の速度（x,y,z）
	KamataEngine::Vector3 vel_{0, 0, 0};

	static inline const float kWalkMotionAngleStart = 0.0f;
	static inline const float kWalkMotionAngleEnd = 45.0f;
	static inline const float kWalkMotionTime = 1.0f;

	float walkTimer_ = 0.0f;

	static inline const float kWidth = 0.99f;
	static inline const float kHeight = 0.99f;

	bool dead_ = false;
	int hp_ = 5;

	std::function<void(const KamataEngine::Vector3&)> onKilled_;

	MapChipField* map_ = nullptr; // ★重複を解消（1つだけ残す）

	bool hitFlash_ = false;
	float hitFlashTimer_ = 0.0f;

	enum class State { Patrol, Chase, Attack, Stunned, Dead };
	State state_ = State::Patrol;

	Player* target_ = nullptr;

	bool onGround_ = false;

	// パラメータ（フレーム基準）
	float speedPatrol_ = 0.013f; // ≒ 0.78 tiles/sec
	float speedChase_ = 0.030f;  // ≒ 1.8 tiles/sec
	float gravity_ = -0.015f;
	float attackRange_ = 0.6f;
	float aggroRange_ = 6.0f;
	float atkWindup_ = 0.25f;
	float atkCooldown_ = 0.6f;
	float atkTimer_ = 0.0f;

	float patrolCenterX_ = 0.0f;
	float patrolHalf_ = 2.0f;

	int dir_ = 1;

	void TickPatrol_(float dt);
	void TickChase_(float dt);
	void TickAttack_(float dt);
	void Integrate_(float dt);
	void CollideWithMap_();


	
	EnemyKind kind_ = EnemyKind::Walker;
	EnemyParams params_;
	float hopTimer_ = 0.0f;
	float shootCD_ = 0.0f;

	std::function<void(const KamataEngine::Vector3&, const KamataEngine::Vector3&, float)> onShoot_;

	// 追加フィールド（private）
	float sizeScale_ = 1.0f; // 見た目＆当たり判定スケール

	// --- Boss 用 ---
	enum class BossPhase : uint8_t { Idle, Volley, DashPrep, DashRun, HopPrep, HopUp, HopFall, StompRecover };
	bool isBoss_ = false; // 既存
	BossPhase bossPhase_ = BossPhase::Idle;
	float phaseT_ = 0.0f;     // フェーズ用タイマ
	int volleyLeft_ = 0;      // ばら撒き残弾セット
	int dashDir_ = +1;        // 突進方向
	float dashSpeed_ = 0.10f; // 突進速度（タイル/フレーム相当のイメージ）
	float dashDur_ = 0.65f;   // 突進持続秒
	float hopVy_ = 0.42f;     // 踏みつけジャンプ初速
	float hopHSpeed_ = 0.06f; // ★追加：踏み付け中の水平速度
	int hpMax_ = 0;           // 最大HP（段階解禁に使う）
	float bossRest_ = 0.0f;   // パターン間の休憩（無防備タイム）

	// 追加メソッド（private）
	void TickBoss_(float dt);
	void FireSpreadTowards_(const KamataEngine::Vector3& target, int count, float spreadRad, float spd);
	void FireRadial_(const KamataEngine::Vector3& center, int num, float spd);



};
