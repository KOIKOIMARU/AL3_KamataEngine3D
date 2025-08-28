#include "GameScene.h"
#include "Transform.h"
#include <algorithm> // std::min, std::max, remove_if
#include <cmath>     // std::cos, std::sin


using namespace KamataEngine;

void GameScene::Initialize() {

	// マップチップフィールドの初期化
	mapChipField_ = new MapChipField();
	mapChipField_->LoadMapChipCsv("Resources/block_easy.csv");

	// ★ 追加：イベント層
	mapChipField_->LoadEventCsv("Resources/event.csv");


	// ファイルを指定してテクスチャを読み込む
	textureHandle_ = TextureManager::Load("sample.png");
	
	// 3Dモデルの生成
	playerModel_ = Model::CreateFromOBJ("player",true);
	enemyModel_ = Model::CreateFromOBJ("enemy", true);
	particleModel_ = Model::CreateFromOBJ("particle", true);
	modelSkydome_ = Model::CreateFromOBJ("sky_sphere", true);

	// モデルブロックの生成
	modelBlock_ = Model::Create();


	// カメラの初期化
	camera_.farZ = 1000.0f;
	camera_.Initialize();

	// デバッグカメラの初期化
	debugCamera_ = new DebugCamera(1280,720);

	// 座標をマップチップ番号で指定
	// プレイヤー
	// 左端から 7 タイル右にずらす（幅が小さいマップでも安全）
	const uint32_t spawnX = std::min<uint32_t>(3, mapChipField_->GetNumHorizontal() - 2);
	const uint32_t spawnY = 16;
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(spawnX, spawnY);

	uint32_t fgTex = TextureManager::Load("bg/festival.png");
	fgOverlay_ = Sprite::Create(fgTex, {0, 0}, {1, 1, 1, 1});
	fgOverlay_->SetSize({1280, 720}); // 画面サイズに合わせる

	// 自キャラの初期化
	player_ = new Player();
	player_->Initialize(playerModel_,&camera_,playerPosition);

	SpawnAheadEnemies_(); // ★追加：開幕分を先に湧かせる

	/* 敵の初期化
	for (int32_t i = 0; i < 3; ++i) {
		Enemy* newEnemy = new Enemy();
		Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(5+i, 18);
		newEnemy->Initialize(enemyModel_, &camera_, enemyPosition);

		enemies_.push_back(newEnemy);
	}*/

	   // ★ 最初のボスゲート列を探す
	for (uint32_t x = 0; x < mapChipField_->GetNumHorizontal(); ++x) {
		if (!mapChipField_->GetEventsInColumn(x, EventChipType::kBossGate).empty()) {
			bossGateX_ = x;
			break;
		}
	}

	wtEnemyBullet_.Initialize();

	wtDanmaku_.Initialize();


	wtFirework_.Initialize();
	wtSpark_.Initialize();
	wtPickup_.Initialize();


	// 仮生成
	deathParticles_ = new DeathParticles();
	deathParticles_->Initialize(particleModel_, &camera_, playerPosition);

	// 天球の初期化
	skydome_.Initialize(modelSkydome_, &camera_, textureHandle_);

	GenerateBlocks();

// …map/load 後
	cameraController_ = new CameraController();
	cameraController_->SetCamera(&camera_);
	cameraController_->SetTarget(player_);

	// 可動域はマップから
	const float left = mapChipField_->GetMapChipPositionByIndex(0, 0).x - 4.0f;
	const float right = mapChipField_->GetMapChipPositionByIndex(mapChipField_->GetNumHorizontal() - 1, 0).x + 4.0f;
	const float top = mapChipField_->GetMapChipPositionByIndex(0, 0).y + 2.0f;
	const float bottom = mapChipField_->GetMapChipPositionByIndex(0, mapChipField_->GetNumVirtical() - 1).y - 2.0f;
	cameraController_->SetMovableArea({left, right, bottom, top});

	// 表示調整
	cameraController_->SetZoomDistance(12.5f);
	cameraController_->SetYOffset(2.2f);

	// 開始だけ左寄せ：右マージンを広げてスナップ
	cameraController_->SetFollowMargin({-3.0f, 7.0f, -2.0f, 2.0f});
	cameraController_->ForceSnap(); // ★ Initializeの代わりにこれ1回でOK

	 auto m = cameraController_->GetFollowMargin();
	auto area = cameraController_->GetMovableArea();
	const auto& wt = player_->GetWorldTransform();

	float desiredX = std::min(wt.translation_.x + m.right, area.right);
	camera_.translation_.x = desiredX;
	camera_.UpdateMatrix();

	// ここより左へは行かせない
	cameraController_->SetXMin(camera_.translation_.x);

	// タイマで元のマージンに戻す
	introMarginTimer_ = 1.0f;




	// その後プレイヤーに渡す
	player_->SetMapChipField(mapChipField_);

	// ★ ポーズ用オーバーレイ生成（黒・半透明）
	uint32_t white = TextureManager::Load("white1x1.png");
	pauseOverlay_ = Sprite::Create(white, {0, 0}, {0, 0, 0, 0.45f});
	pauseOverlay_->SetSize({1280, 720});


	    // ポーズ用初期化
	pauseSelected_ = 0;     // 0:再開
	exitTo_ = ExitTo::None; // 終了先なし

	phase_ = Phase::kPlay;        

	// ▼ 代わりにゲージUI生成
	spGaugeBack_ = Sprite::Create(white, {20, 20}, {0, 0, 0, 0.45f}); // 枠（薄黒）
	spGaugeBack_->SetSize({200, 16});

	spGaugeFill_ = Sprite::Create(white, {20, 20}, {1.0f, 0.85f, 0.2f, 0.95f}); // 中身（黄）
	spGaugeFill_->SetSize({0, 16});                                             // 0から伸ばす

	// ▼ 簡易リズムUIの初期化
	InitRhythmUI_();

	wtLantern_.Initialize();

	fade_ = Sprite::Create(white, {0, 0}, {0, 0, 0, 1});
	fade_->SetSize({1280, 720});

	fadeT_ = 1.0f; // 最初は黒
	fadeDir_ = -1; // フェードイン開始
	phase_ = Phase::kFadeIn;

}

void GameScene::Update() {

	#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_C)) {
		// デバッグカメラの有効/無効を切り替える
		isDebugCameraActive = !isDebugCameraActive;
	}

	#endif 

	  const float dt = 1.0f / 60.0f;

	// フェード駆動
	if (fadeDir_ != 0) {
		fadeT_ += fadeDir_ * 2.5f * dt; // 0.4秒程度で全遷移
		fadeT_ = std::clamp(fadeT_, 0.0f, 1.0f);

		if (fadeDir_ < 0 && fadeT_ <= 0.0f) {
			// フェードイン完了
			fadeDir_ = 0;
			phase_ = Phase::kPlay;
		}
		if (fadeDir_ > 0 && fadeT_ >= 1.0f) {
			// フェードアウト完了 → 終了通知
			fadeDir_ = 0;
			finished_ = true;
			if (onExitComplete_)
				onExitComplete_(exitTo_);
		}
	}

	    // ★ Pでポーズ切替
	if (Input::GetInstance()->TriggerKey(DIK_P)) {
		paused_ = !paused_;
	}

    // ★ ポーズ中はゲーム進行を止め、メニュー入力だけ受け付ける
	if (paused_) {
		if (isDebugCameraActive) {
			debugCamera_->Update();
			camera_.matView = debugCamera_->GetViewMatrix();
			camera_.matProjection = debugCamera_->GetProjectionMatrix();
			camera_.TransferMatrix();
		} else {
			camera_.UpdateMatrix();
		}

		HandlePauseMenuInput(); // ★ ここでメニュー操作
		return;                 // それ以外の更新は止める
	}

	  // ▼ SPゲージ（左下へ移動）
	uint32_t white = TextureManager::Load("white1x1.png");

	// 背景（薄黒） 左下に置く
	const float gaugeW = 200.0f;
	const float gaugeH = 16.0f;
	const float margin = 18.0f;
	//const float screenW = 1280.0f;
	const float screenH = 720.0f;

	spGaugeBack_ = Sprite::Create(white, {margin, screenH - margin - gaugeH}, {0, 0, 0, 0.45f});
	spGaugeBack_->SetSize({gaugeW, gaugeH});

	spGaugeFill_ = Sprite::Create(white, {margin, screenH - margin - gaugeH}, {1.0f, 0.85f, 0.2f, 0.95f});
	spGaugeFill_->SetSize({0, gaugeH}); // 中身はUpdateで横幅だけ伸ばす

	// Initialize() の終わりの方で
	texHeartOn_ = TextureManager::Load("heart_on.png");   // 赤ハート
	texHeartOff_ = TextureManager::Load("heart_off.png"); // 空ハート（なければ作ってね）

	const int maxHp = 3; // 無ければ手書きで 3 など
	lifeOn_.reserve(maxHp);
	lifeOff_.reserve(maxHp);

	lifeIconSize_ = 64; // 1個のサイズ(px)

	float x = (float)lifeMarginL_;
	float y = (float)lifeMarginT_;
	for (int i = 0; i < maxHp; ++i) {
		auto on = Sprite::Create(texHeartOn_, {x, y}, {1, 1, 1, 1});
		auto off = Sprite::Create(texHeartOff_, {x, y}, {1, 1, 1, 0.55f});
		on->SetSize({(float)lifeIconSize_, (float)lifeIconSize_});
		off->SetSize({(float)lifeIconSize_, (float)lifeIconSize_});
		lifeOn_.push_back(on);
		lifeOff_.push_back(off);
		x += lifeIconSize_ + lifeIconGap_;
	}




	switch (phase_) {
	case Phase::kFadeIn: {
		// 進行は止めつつ、見た目に必要な更新だけ
		    skydome_.Update();
		cameraController_->Update(); // カメラ行列更新

    // ★プレイヤーのWTだけ転送（動かさない）
		player_->SyncTransformOnly();

		// ★敵もWTだけ転送
		for (auto* e : enemies_) {
			if (!e)
				continue;
			e->SyncTransformOnly();
		}


		for (auto& line : worldTransformBlocks_) {
			for (auto* wt : line) {
				if (!wt) continue;
				UpdateWorldTransform(*wt); // ブロック行列を用意
				
			}
			
		}
		break;
		
	}

	case Phase::kPlay:
		UpdatePlayPhase();
		break;
	case Phase::kDeath:
		UpdateDeathPhase();
		break;
	}
}

void GameScene::Draw() {
	DirectXCommon* dx = DirectXCommon::GetInstance();



	// ===== 3Dパス（モデル）================================================
	Model::PreDraw(dx->GetCommandList());

	// プレイヤー
	if (phase_ == Phase::kPlay || phase_ == Phase::kFadeIn) {
		player_->Draw();
	}

	// 敵
	for (Enemy* enemy : enemies_) {
		if (enemy && !enemy->IsDead()) {
			enemy->Draw();
		}
	}

	if (phase_ == Phase::kDeath && deathParticles_) {
		deathParticles_->Draw();
		
	}


	//花火 
		
	for (const auto& fw : player_->GetFireworks()) { wtFirework_.translation_ = fw.pos; float s = std::max(0.35f, fw.radius * 1.2f); // 半径に応じて 
	wtFirework_.scale_ = {s, s, s};
	UpdateWorldTransform(wtFirework_); particleModel_->Draw(wtFirework_, camera_); }

	// スパーク
	for (const auto& s : sparks_) {
		wtSpark_.translation_ = s.pos;
		wtSpark_.scale_ = {0.6f, 0.6f, 0.6f};
		UpdateWorldTransform(wtSpark_);
		particleModel_->Draw(wtSpark_, camera_);
	}

	// 提灯
	for (const auto& L : lanterns_) {
		float bob = std::sin(L.t * 6.0f) * 0.06f;
		wtLantern_.translation_ = {L.pos.x, L.pos.y + bob, L.pos.z};
		wtLantern_.scale_ = {0.5f, 0.5f, 0.5f};
		UpdateWorldTransform(wtLantern_);
		particleModel_->Draw(wtLantern_, camera_);
	}

	// ブロック
	for (auto& line : worldTransformBlocks_) {
		for (WorldTransform* wt : line) {
			if (!wt)
				continue;
			UpdateWorldTransform(*wt);
			modelBlock_->Draw(*wt, camera_);
		}
	}

	// 自弾・敵弾
	if (phase_ == Phase::kPlay) {
		DrawDanmakuBullets_();
		DrawEnemyBullets_();
	}

	// ★天球は一番最後に（ZWrite=false 推奨）
	skydome_.Draw();

	Model::PostDraw(); // ← これ必須！

	// ===== 2Dパス（スプライト）============================================
	Sprite::PreDraw(dx->GetCommandList(), Sprite::BlendMode::kNormal);

	if (!paused_) {
		DrawRhythmUI_();
	}
	DrawLifeUI_();  // ★← これを追加（ライフ左上）
	DrawStockUI_(); // SPゲージ（左下）

	if (paused_ && pauseOverlay_) {
		pauseOverlay_->Draw();
	}
	if (fade_) {
		fade_->SetColor({0, 0, 0, std::clamp(fadeT_, 0.0f, 1.0f)});
		fade_->Draw();
	}

	Sprite::PostDraw();

	// ★ここで掃除
	CleanupDeadEnemies_();
}



// デストラクタ
GameScene::~GameScene() {
	// デバッグカメラの解放
	delete debugCamera_;
	// スプライトの解放
	delete playerModel_;
	delete modelSkydome_;
	// 自キャラの解放
	delete player_;
	// 敵の解放
	for (Enemy* enemy : enemies_) {
		delete enemy;
	}
	enemies_.clear();
	// パーティクルの解放
	delete deathParticles_;
	// マップチップフィールドの解放
	delete mapChipField_;
	// モデルブロックの解放
	delete modelBlock_;
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
		//worldTransformBlockLine.clear();
	}
	worldTransformBlocks_.clear();
	// カメラコントローラーの解放
	delete cameraController_;
	delete enemyModel_;
	delete particleModel_;
	delete pauseOverlay_;
	delete spGaugeBack_;
	delete spGaugeFill_;
	spGaugeBack_ = spGaugeFill_ = nullptr;

	for (auto* s : ringDots_) {
		delete s;
	}
	for (auto* s : targetDots_) {
		delete s;
	}
	ringDots_.clear();

	targetDots_.clear();

	delete fade_; // 追加
	fade_ = nullptr;
}

void GameScene::GenerateBlocks() {
	// 要素数
	const uint32_t numBlockVirtical = mapChipField_->GetNumVirtical();
	const uint32_t numBlockHorizontal = mapChipField_->GetNumHorizontal();

	worldTransformBlocks_.resize(numBlockVirtical);
	for (uint32_t i = 0; i < numBlockVirtical; i++) {
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}

	for (uint32_t i = 0; i < numBlockVirtical; i++) {
		for (uint32_t j = 0; j < numBlockHorizontal; j++) {
			if (mapChipField_->GetMapChipTypeByIndex(j, i) == MapChipType::kBlock) {
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransformBlocks_[i][j] = worldTransform;
				worldTransformBlocks_[i][j]->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
			}
		}
	}
}

inline bool GameScene::IsCollisionAABB(const AABB& a, const AABB& b) {
	return (a.min.x <= b.max.x && a.max.x >= b.min.x) && (a.min.y <= b.max.y && a.max.y >= b.min.y) && (a.min.z <= b.max.z && a.max.z >= b.min.z);
}


void GameScene::CheckAllCollision() {
	// --- プレイヤー vs 敵（既存） ---
	AABB aabbPlayer = player_->GetAABB();
	for (Enemy* enemy : enemies_) {
		if (!enemy)
			continue;
		AABB aabbEnemy = enemy->GetAABB();
		if (IsCollisionAABB(aabbPlayer, aabbEnemy)) {
			player_->OnCollision(enemy);
		}
	}

	// --- 花火 vs 敵 ---
	auto& fws = player_->GetFireworks();
	for (Enemy* e : enemies_) {
		if (!e)
			continue;

		AABB eaabb = e->GetAABB();
		Vector3 ec{(eaabb.min.x + eaabb.max.x) * 0.5f, (eaabb.min.y + eaabb.max.y) * 0.5f, 0.0f};
		float er = std::max(eaabb.max.x - eaabb.min.x, eaabb.max.y - eaabb.min.y) * 0.5f;

		for (auto& fw : fws) {
			if (fw.life <= 0.0f)
				continue;

			Vector3 d{ec.x - fw.pos.x, ec.y - fw.pos.y, 0.0f};
			float dist2 = d.x * d.x + d.y * d.y;
			float hitR = er + fw.radius;

			if (dist2 <= hitR * hitR) {
				SpawnExplosion_(fw.pos, 14, 0.18f, 0.35f);
				cameraController_->AddHitstop(0.05f);
				cameraController_->AddShake(0.06f, 0.12f);

				// ★ ダメージを弾に持たせた値で
				e->ApplyDamage(std::max(1, fw.damage));
				AddSP_(kSPGainPerDamage_ * std::max(1, fw.damage));

				if (fw.type == FireworkType::Pierce) {
					// 貫通：残す（寿命だけ削る）
					fw.life = std::max(0.05f, fw.life - 0.18f);
					continue; // 他の敵にも
				} else if (fw.type == FireworkType::Burst) {
					// ★ 仕込んだ爆散パラメータで予約してから元弾を消す
					player_->QueueBurstAt(fw.pos, fw.burstCount, fw.burstSpd, fw.burstLife, fw.burstRad);
					fw.life = 0.0f;
					break;
				} else {
					// Rocket：消す
					fw.life = 0.0f;
					break;
				}
			}
		}
	}

	// --- プレイヤー弾幕弾 vs 敵 ---
	auto circleRectHit = [](const Vector3& c, float r, const AABB& a) {
		float cx = std::clamp(c.x, a.min.x, a.max.x);
		float cy = std::clamp(c.y, a.min.y, a.max.y);
		float dx = c.x - cx, dy = c.y - cy;
		return (dx * dx + dy * dy) <= (r * r);
	};

	for (auto& db : danmakuBullets_) {
		if (db.life <= 0.0f)
			continue;
		for (Enemy* e : enemies_) {
			if (!e)
				continue;
			AABB ea = e->GetAABB();
			if (circleRectHit(db.pos, db.r, ea)) {
				e->ApplyDamage(1);                        // ヒットで1ダメ（調整可）
				SpawnExplosion_(db.pos, 8, 0.20f, 0.25f); // ちょい火花
				AddSP_(kSPGainDanmakuHit_);
				db.life = -1.0f;                          // 弾は消える（貫通させたければ消さない）
				break;
			}
		}
	}
}



void GameScene::UpdatePlayPhase() {


	// ★ クリア条件：敵が全滅したらクリア
	if (bossSpawned_ && enemies_.empty()) {
		if (fadeDir_ == 0) { // 多重起動ガード
			resultKind_ = ResultScene::Kind::Clear;
			RequestExit(ExitTo::Result); // ← これでフェードアウト開始
		}
		return;
	}

	// 天球の更新
	skydome_.Update();
	// プレイヤーの更新
	player_->Update();

if (cameraController_ && cameraController_->IsXLocked()) {
		// プレイヤーZとカメラZの距離
		float playerZ = player_->GetWorldPosition().z;
		float camZ = camera_.translation_.z;
		float dist = std::abs(playerZ - camZ);

		// --- 射影行列から tan(fovY/2) と アスペクト比 を復元 ---
		// DirectX の透視射影:  m00 = 1 / (aspect * tan(fov/2)),  m11 = 1 / tan(fov/2)
		const auto& P = camera_.matProjection; // ← あなたの型に合わせて
		float m00 = P.m[0][0];
		float m11 = P.m[1][1];

		float tanHalfFovY = (m11 != 0.0f) ? (1.0f / m11) : 0.0f;
		float aspect = (m00 != 0.0f) ? (m11 / m00) : 1.7777778f; // 予備で16:9

		float halfH = dist * tanHalfFovY; // 画面半高(ワールド)
		float halfW = halfH * aspect;     // 画面半幅(ワールド)

		// 画面端からの余白（好みで）
		const float padL = 0.6f;
		const float padR = 0.6f;

		float camX = camera_.translation_.x;
		float minX = camX - halfW + padL;
		float maxX = camX + halfW - padR;

		auto& wt = const_cast<WorldTransform&>(player_->GetWorldTransform());
		//float before = wt.translation_.x;
		wt.translation_.x = std::clamp(wt.translation_.x, minX, maxX);

		// 押し戻した時に横速度を消したい場合（APIがあれば）
		// if (before != wt.translation_.x) { player_->ZeroXVelocity(); }
	}



	// スーパーファイアワーク発動（Q）
	// UpdatePlayPhase() 内
	// ▼ 元の「UseStock_()」呼び出しはそのまま利用可（中身を差し替える）
	if (Input::GetInstance()->TriggerKey(DIK_Q) && superCD_ <= 0.0f) {
		UseStock_();      // ← ゲージ満タンなら発動＆ゲージ消費
		superCD_ = 0.35f; // 連打抑制
	}


	superCD_ = std::max(0.0f, superCD_ - 1.0f / 60.0f);


	    // ★ 追加：道中でイベントEから敵を出す
	SpawnAheadEnemies_();

	// 敵の更新
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}
	// カメラコントローラの更新
	cameraController_->Update();

		auto area = cameraController_->GetMovableArea();
		const float eps = 1e-3f;
		bool camAtLeft = (camera_.translation_.x <= area.left + eps);

		if (camAtLeft || cameraController_->IsXLocked()) {
		    // 射影から画面半幅を復元
		    const auto& P = camera_.matProjection;
		    float m00 = P.m[0][0], m11 = P.m[1][1];
		    float tanHalfFovY = (m11 != 0.0f) ? (1.0f / m11) : 0.0f;
		    float aspect = (m00 != 0.0f) ? (m11 / m00) : (16.0f / 9.0f);

		    float dist = std::abs(player_->GetWorldPosition().z - camera_.translation_.z);
		    float halfH = dist * tanHalfFovY;
		    float halfW = halfH * aspect;

		    // 画面左の内側に少し余白
		    const float padL = 0.6f;
		    float minX = camera_.translation_.x - halfW + padL;

		    auto& wt = const_cast<WorldTransform&>(player_->GetWorldTransform());
		    if (wt.translation_.x < minX) {
			    wt.translation_.x = minX;
		    }
	    }

	// カメラの処理
	if (isDebugCameraActive) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetViewMatrix();
		camera_.matProjection = debugCamera_->GetProjectionMatrix();
		// ビュープロジェクション行列の転送
		camera_.TransferMatrix();
	} else {
		// ビュープロジェクション行列の転送と更新
		camera_.UpdateMatrix();
	}


	// ブロックの更新
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue; // nullptrチェック
			}
			// アフィン変換行列の作成

			UpdateWorldTransform(*worldTransformBlock);
		}
	}

	UpdateEnemyBullets_(1.0f / 60.0f);
	UpdateDanmakuBullets_(1.0f / 60.0f); // ← 追加

	// 全ての当たり判定の更新
	CheckAllCollision();

	
	    // ★ 追加：ゲート到達でボス導入
	CheckBossGateAndSpawnBoss_();

// スパーク更新
	for (auto& s : sparks_) {
		s.pos.x += s.vel.x;
		s.pos.y += s.vel.y;
		s.pos.z += s.vel.z;

		// 減衰弱めに
		s.vel.x *= 0.98f;
		s.vel.y *= 0.98f;
		s.vel.z *= 0.98f;

		s.life -= 1.0f / 60.0f;
		s.pos.z = 0.0f;
	}


	sparks_.erase(std::remove_if(sparks_.begin(), sparks_.end(), [](const Spark& sp) { return sp.life <= 0.0f; }), sparks_.end());

	{
		const float dt = 1.0f / 60.0f;
		for (auto& L : lanterns_) {
			L.t += dt;
			L.life -= dt;
			// ふわふわ揺れ（見た目だけ。実座標は変えず描画時にオフセットでもOK）
		}
		lanterns_.erase(std::remove_if(lanterns_.begin(), lanterns_.end(), [](const Lantern& L) { return L.life <= 0.0f; }), lanterns_.end());
	}

	// UpdatePlayPhase() の最後の方に
	introMarginTimer_ = std::max(0.0f, introMarginTimer_ - (1.0f / 60.0f));
	if (introMarginTimer_ == 0.0f) {
		// 一度だけ戻す（戻したらもう実行しない工夫を）
		cameraController_->SetFollowMargin({-3.0f, 3.0f, -2.0f, 2.0f});
		introMarginTimer_ = -1.0f; // 無効化
	}



	UpdateRhythmUI_();

	//	フェーズ切り替え
	ChangePhase();
}

void GameScene::UpdateDeathPhase() {
	// 天球の更新
	skydome_.Update();
	// 敵の更新
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}
	// 死亡演出用の処理のみ
	if (deathParticles_) {
		deathParticles_->Update();
	}
	// カメラの処理
	if (isDebugCameraActive) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetViewMatrix();
		camera_.matProjection = debugCamera_->GetProjectionMatrix();
		// ビュープロジェクション行列の転送
		camera_.TransferMatrix();
	} else {
		// ビュープロジェクション行列の転送と更新
		camera_.UpdateMatrix();
	}
	// ブロックの更新
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue; // nullptrチェック
			}
			// アフィン変換行列の作成

			UpdateWorldTransform(*worldTransformBlock);
		}
	}

	if (deathParticles_ && deathParticles_->IsFinished()) {
		resultKind_ = ResultScene::Kind::GameOver;  
		finished_ = true;       
	}


}

void GameScene::ChangePhase() {
	switch (phase_) { 
	case Phase::kPlay:
		// プレイヤーが死亡したらフェーズ切り替え
		if (player_->IsDead()) {
			// フェーズをDeathに変更
			phase_ = Phase::kDeath;

			// デスパーティクルの発生位置 = プレイヤーの位置
			const Vector3& deathParticlesPosition = player_->GetWorldPosition();

			// デスパーティクルを再初期化（再生成）
			if (deathParticles_) {
				delete deathParticles_;
				deathParticles_ = nullptr;
			}
			deathParticles_ = new DeathParticles();
			deathParticles_->Initialize(particleModel_, &camera_, deathParticlesPosition);
		}

		break;
	case Phase::kDeath:
		break;
	}

}

void GameScene::SpawnExplosion_(const KamataEngine::Vector3& center, int num, float speed, float lifeSec) {


	const float zOffset = -0.2f;
	for (int i = 0; i < num; ++i) {
		float ang = (2.0f * 3.14159265f / num) * i;
		Spark s;
		s.pos = {center.x, center.y, center.z + zOffset};
		s.vel = {std::cos(ang) * speed, std::sin(ang) * speed, 0.0f};
		s.life = lifeSec;
		sparks_.push_back(s);
	}
}

void GameScene::HandlePauseMenuInput() {
	auto* in = Input::GetInstance();

	// 上下キーで項目移動（トリガー入力）
	if (in->TriggerKey(DIK_UP)) {
		pauseSelected_ = (pauseSelected_ + kPauseItems - 1) % kPauseItems;
	}
	if (in->TriggerKey(DIK_DOWN)) {
		pauseSelected_ = (pauseSelected_ + 1) % kPauseItems;
	}

	// HandlePauseMenuInput()
	if (in->TriggerKey(DIK_RETURN) || in->TriggerKey(DIK_SPACE)) {
		switch (pauseSelected_) {
		case 0: // 再開
			paused_ = false;
			break;
		case 1:                             // リトライ
			RequestExit(ExitTo::RetryGame); // ← これに置換
			break;
		case 2:                         // タイトル
			RequestExit(ExitTo::Title); // ← これに置換
			break;
		}
	}


	// Esc で即再開（任意）
	if (in->TriggerKey(DIK_ESCAPE)) {
		paused_ = false;
	}
}

void GameScene::SpawnAheadEnemies_() {
	if (bossSpawned_)
		return;

	uint32_t pCol = mapChipField_->GetMapChipIndexSetByPosition(player_->GetWorldPosition()).xIndex;
	uint32_t maxX = std::min(pCol + spawnLookAhead_, mapChipField_->GetNumHorizontal() - 1);

	for (uint32_t x = pCol; x <= maxX; ++x) {

		auto spotsE = mapChipField_->GetEventsInColumn(x, EventChipType::kEnemy);
		for (auto [ex, ey] : spotsE) {

			// ① 種別取得 → ② イベント消費
			uint8_t v = mapChipField_->GetEnemyVariant(ex, ey);
			mapChipField_->ConsumeEvent(ex, ey);

			Vector3 pos = mapChipField_->GetMapChipPositionByIndex(ex, ey);

			// ③ 種別決定
			Enemy::EnemyKind kind = Enemy::EnemyKind::Walker;
			if (v == 1)
				kind = Enemy::EnemyKind::Hopper;
			else if (v == 2)
				kind = Enemy::EnemyKind::Shooter;

			Enemy* e = new Enemy();
			e->Initialize(enemyModel_, &camera_, pos);
			e->SetMap(mapChipField_);
			e->SetTarget(player_);
			e->SetKind(kind);

		// GameScene::SpawnAheadEnemies_() の Shooter 用コールバック
			if (kind == Enemy::EnemyKind::Shooter) {
				e->SetOnShootCallback([this](const Vector3& p, const Vector3& dir, float sp) { SpawnEnemyBullet_(p, dir, sp); });
			}


			// SpawnAheadEnemies_() の e->SetOnKilledCallback(...) を下記に置換
			e->SetOnKilledCallback([this](const Vector3& p) {
				SpawnExplosion_(p, 20, 0.5f, 0.5f);
				// ※ アイテム生成なし
			});


			enemies_.push_back(e);
		}

		// 提灯（演出）
		auto spotsL = mapChipField_->GetEventsInColumn(x, EventChipType::kLanternRow);
		if (!spotsL.empty()) {
			for (auto [lx, ly] : spotsL) {
				mapChipField_->ConsumeEvent(lx, ly);
				lanterns_.push_back(Lantern{mapChipField_->GetMapChipPositionByIndex(lx, ly), 3.0f, 0.0f});
			}
		}
	}
}




void GameScene::CheckBossGateAndSpawnBoss_() {
	if (bossSpawned_ || bossGateX_ == UINT32_MAX)
		return;

	uint32_t pCol = mapChipField_->GetMapChipIndexSetByPosition(player_->GetWorldPosition()).xIndex;
	if (pCol >= bossGateX_) {
		// カメラ右端をゲート手前に固定
		float lockX = mapChipField_->GetMapChipPositionByIndex(std::max(1u, bossGateX_ - 1), 0).x;
		cameraController_->SetXMax(lockX);

		// …Bスポーンは今のままでOK…
		for (uint32_t x = bossGateX_; x < mapChipField_->GetNumHorizontal(); ++x) {
			auto bs = mapChipField_->GetEventsInColumn(x, EventChipType::kBossSpawn);
			if (!bs.empty()) {
				auto [bx, by] = bs.front();
				mapChipField_->ConsumeEvent(bx, by);
				Vector3 pos = mapChipField_->GetMapChipPositionByIndex(bx, by);
				Enemy* boss = new Enemy();
				boss->Initialize(enemyModel_, &camera_, pos);
				boss->SetMap(mapChipField_); // ★忘れずに
				boss->SetTarget(player_); 
				boss->SetOnKilledCallback([this](const Vector3& pos) {
					SpawnExplosion_(pos, 64, 0.9f, 1.0f);
					Pickup p;
					p.pos = pos;
					pickups_.push_back(p);
				});
				enemies_.push_back(boss);
				// ★ ここから追加
				bossEnemy_ = boss;
				bossSpawned_ = true;
				//boss->SetOnShootCallback([this](const Vector3& p, const Vector3& dir, float sp) { SpawnEnemyBullet_(p, dir, sp); });
				boss->SetBoss(true, 1.8f); // 好きな倍率に。例: 2.0f でもOK


				// いまのカメラ位置で水平固定（左右に動かない）
				cameraController_->LockX(camera_.translation_.x);

				// 雑魚全消し（ボス以外）
				CullNonBossEnemies_();
				// ★ ここまで追加

				break;
			}
		}
	}
}

void GameScene::SpawnEnemyBullet_(const Vector3& p, const Vector3& dir, float sp) {
	EnemyBullet b;
	b.pos = p;

	// dir を正規化して速度に反映（速度は “フレーム単位” 想定）
	float n = std::sqrt(dir.x * dir.x + dir.y * dir.y);
	Vector3 nd = (n > 1e-6f) ? Vector3{dir.x / n, dir.y / n, 0} : Vector3{1, 0, 0};
	b.vel = {nd.x * sp, nd.y * sp, 0};

	b.r = 0.20f;
	b.life = 3.0f; // 3秒で消滅

	enemyBullets_.push_back(b);
}

void GameScene::UpdateEnemyBullets_(float dt) {
	auto circleRectHit = [](const Vector3& c, float r, const AABB& a) {
		float cx = std::clamp(c.x, a.min.x, a.max.x);
		float cy = std::clamp(c.y, a.min.y, a.max.y);
		float dx = c.x - cx, dy = c.y - cy;
		return (dx * dx + dy * dy) <= (r * r);
	};

	for (auto& b : enemyBullets_) {
		// 移動（速度はフレーム単位なら dt は不要。もし秒単位にしたければ *dt）
		b.pos.x += b.vel.x;
		b.pos.y += b.vel.y;
		b.life -= dt;

		// マップ衝突：ブロックに入ったら消す
		auto is = mapChipField_->GetMapChipIndexSetByPosition(b.pos);
		if (mapChipField_->SafeGet(is.xIndex, is.yIndex) == MapChipType::kBlock) {
			SpawnExplosion_(b.pos, 8, 0.12f, 0.25f);
			b.life = -1.0f; // kill
			continue;
		}

		// プレイヤー衝突
		AABB pa = player_->GetAABB();
		if (circleRectHit(b.pos, b.r, pa)) {
			// ここでプレイヤーにダメージ
			// 手元の Player にダメージAPIがある想定（なければ Kill() 等に差し替え）
			player_->ApplyDamage(1, &b.pos);

			cameraController_->AddHitstop(0.03f);
			cameraController_->AddShake(0.05f, 0.10f);
			SpawnExplosion_(b.pos, 10, 0.14f, 0.28f);
			b.life = -1.0f; // kill
			continue;
		}
	}

	// 消滅処理
	enemyBullets_.erase(std::remove_if(enemyBullets_.begin(), enemyBullets_.end(), [](const EnemyBullet& b) { return b.life <= 0.0f; }), enemyBullets_.end());
}

void GameScene::DrawEnemyBullets_() {
	for (const auto& b : enemyBullets_) {
		wtEnemyBullet_.translation_ = b.pos;
		wtEnemyBullet_.scale_ = {0.35f, 0.35f, 0.35f}; // 見た目のサイズ
		UpdateWorldTransform(wtEnemyBullet_);
		// 小さな球/パーティクルモデルでOK（particleModel_を流用）
		particleModel_->Draw(wtEnemyBullet_, camera_);
	}
}

void GameScene::AddSP_(float v) { spGauge_ = std::clamp(spGauge_ + v, 0.0f, 1.0f); }


void GameScene::AddStock_(int n) { fireworkStock_ = std::min(kMaxStock, fireworkStock_ + n); }

// 旧: ストック消費 → 新: ゲージ満タン時だけ発動
void GameScene::UseStock_() {
	if (spGauge_ < 1.0f)
		return;              // まだ溜まってない
	spGauge_ = 0.0f;         // 消費（全部）
	ActivateDanmakuSuper_(); // 必殺発動は従来どおり
}




void GameScene::ActivateSuperFirework_() {
	// プレイヤー中心に大爆発
	Vector3 c = player_->GetWorldPosition();
	SpawnExplosion_(c, 64, 0.9f, 1.0f);

	// 近くの敵を一掃（半径R）
	const float R = 3.2f;
	const float R2 = R * R;
	for (Enemy* e : enemies_) {
		if (!e)
			continue;
		AABB a = e->GetAABB();
		Vector3 ec{(a.min.x + a.max.x) * 0.5f, (a.min.y + a.max.y) * 0.5f, 0};
		Vector3 d{ec.x - c.x, ec.y - c.y, 0};
		if (d.x * d.x + d.y * d.y <= R2) {
			e->Kill();
		}
	}
	// 追加のスパーク（視認性）
	for (int i = 0; i < 24; ++i) {
		float ang = (2.0f * 3.14159265f / 24.0f) * i;
		Spark sp;
		sp.pos = c;
		sp.vel = {std::cos(ang) * 0.25f, std::sin(ang) * 0.25f, 0.0f};
		sp.life = 0.35f;
		sparks_.push_back(sp);
	}
}

void GameScene::DrawStockUI_() {
	if (!spGaugeBack_ || !spGaugeFill_)
		return;

	// 背景
	spGaugeBack_->Draw();

	// 中身（横幅をゲージ比率でスケール）
	const float maxW = 200.0f;
	float w = std::clamp(spGauge_, 0.0f, 1.0f) * maxW;
	spGaugeFill_->SetSize({w, 16});
	spGaugeFill_->Draw();
}


void GameScene::CleanupDeadEnemies_() {
	for (auto it = enemies_.begin(); it != enemies_.end();) {
		Enemy* e = *it;
		if (!e || e->IsDead()) {
			delete e;
			it = enemies_.erase(it);
		} else {
			++it;
		}
	}
}


void GameScene::SpawnUIBurst_(Judge j) {
	auto addRing = [&](float life, float r0, float r1, KamataEngine::Vector4 col) {
		UIRing r;
		r.life = life;
		r.r0 = r0;
		r.r1 = r1;
		r.col = col;
		r.t = 0;
		uiRings_.push_back(r);
	};

	switch (j) {
case Judge::Perfect: {
    // ターゲット円の半径を基準に二重丸
    auto vis = player_->GetRhythmVis();
    float R = vis.targetR;

    // 二重丸（固定半径でスッと消える）
    addRing(0.65f, R - 10.0f, R - 10.0f, {1.0f, 1.0f, 1.0f, 1.00f});  // 内側：白
    addRing(0.65f, R + 10.0f, R + 10.0f, {0.70f, 1.0f, 1.0f, 0.95f}); // 外側：薄シアン

    // お好み：ごく控えめの外向きリップル（要らなければ消してOK）
    addRing(0.40f, R + 10.0f, R + 140.0f, {0.80f, 1.0f, 1.0f, 0.40f});
    break;
}

	case Judge::Good:
		addRing(0.45f, 6.0f, 120.0f, {0.70f, 1.00f, 0.60f, 0.95f}); // ライム
		break;
	default:
		addRing(0.30f, 4.0f, 80.0f, {1.00f, 0.45f, 0.45f, 0.75f}); // 赤み控えめ
		break;
	}
}


void GameScene::InitRhythmUI_() {
	whiteTexRhythm_ = TextureManager::Load("white1x1.png");

	ringDots_.resize(kRhythmSeg_);
	targetDots_.resize(kRhythmSeg_);

	for (int i = 0; i < kRhythmSeg_; ++i) {
		ringDots_[i] = Sprite::Create(whiteTexRhythm_, {0, 0}, {1.0f, 0.85f, 0.2f, 0.95f});
		targetDots_[i] = Sprite::Create(whiteTexRhythm_, {0, 0}, {1.0f, 1.0f, 1.0f, 0.75f});
		ringDots_[i]->SetSize({3, 3});
		targetDots_[i]->SetSize({2, 2});
	}

}


void GameScene::UpdateRhythmUI_() {
	for (auto& e : player_->ConsumeUIShotEvents()) {
		SpawnUIBurst_(e.judge);
	}

	const float dt = 1.0f / 60.0f;

	// リング・レイ
	for (auto& r : uiRings_)
		r.t += dt;


	// 破棄
	uiRings_.erase(std::remove_if(uiRings_.begin(), uiRings_.end(), [](const UIRing& r) { return r.t >= r.life; }), uiRings_.end());
}


void GameScene::DrawRhythmUI_() {
	const KamataEngine::Vector2 center = {1100.0f, 560.0f};
	const float twoPi = 6.28318530718f;

	// --- ホールド中リング・ターゲット ---
	if (player_->IsAttackHolding()) {
		const auto vis = player_->GetRhythmVis();
		for (int i = 0; i < kRhythmSeg_; ++i) {
			float ang = twoPi * (float)i / (float)kRhythmSeg_;
			float x = center.x + std::cos(ang) * vis.ringR;
			float y = center.y + std::sin(ang) * vis.ringR;
			ringDots_[i]->SetPosition({x, y});
			ringDots_[i]->SetColor({1.0f, 0.85f, 0.2f, 0.95f});
			ringDots_[i]->SetSize({3, 3});
			ringDots_[i]->Draw();
		}
		for (int i = 0; i < kRhythmSeg_; ++i) {
			float ang = twoPi * (float)i / (float)kRhythmSeg_;
			float x = center.x + std::cos(ang) * vis.targetR;
			float y = center.y + std::sin(ang) * vis.targetR;
			targetDots_[i]->SetPosition({x, y});
			targetDots_[i]->SetColor({1.0f, 1.0f, 1.0f, 0.75f});
			targetDots_[i]->SetSize({2, 2});
			targetDots_[i]->Draw();
		}
	}

	// --- 破裂リング ---
	for (const auto& r : uiRings_) {
		float u = std::clamp(r.t / r.life, 0.0f, 1.0f);
		float e = 1.0f - (1.0f - u) * (1.0f - u);
		float radius = r.r0 + (r.r1 - r.r0) * e;
		float alpha = 1.0f - u;
		auto col = r.col;
		col.w *= alpha;

		for (int i = 0; i < kRhythmSeg_; ++i) {
			float ang = twoPi * (float)i / (float)kRhythmSeg_;
			float x = center.x + std::cos(ang) * radius;
			float y = center.y + std::sin(ang) * radius;
			ringDots_[i]->SetPosition({x, y});
			ringDots_[i]->SetColor(col);
			ringDots_[i]->SetSize({3, 3});
			ringDots_[i]->Draw();
		}
	}
}

void GameScene::RequestExit(ExitTo to) {
	if (fadeDir_ > 0)
		return; // 既に遷移中
	exitTo_ = to;
	pendingExit_ = to;
	fadeDir_ = +1; // フェードアウト開始
	if (onExitBegin_)
		onExitBegin_(to);
}

void GameScene::CullNonBossEnemies_() {
	for (auto it = enemies_.begin(); it != enemies_.end();) {
		Enemy* e = *it;
		if (!e) {
			it = enemies_.erase(it);
			continue;
		}
		if (e == bossEnemy_) {
			++it;
			continue;
		}         // ボスは残す
		delete e; // 雑魚は消す
		it = enemies_.erase(it);
	}
}

void GameScene::GetCameraWorldRect_(float z, float& left, float& right, float& bottom, float& top) {
	const auto& P = camera_.matProjection;
	float m00 = P.m[0][0], m11 = P.m[1][1];
	float tanHalfFovY = (m11 != 0.0f) ? (1.0f / m11) : 0.0f;
	float aspect = (m00 != 0.0f) ? (m11 / m00) : (16.0f / 9.0f);

	float dist = std::abs(z - camera_.translation_.z);
	float halfH = dist * tanHalfFovY;
	float halfW = halfH * aspect;

	left = camera_.translation_.x - halfW;
	right = camera_.translation_.x + halfW;
	bottom = camera_.translation_.y - halfH;
	top = camera_.translation_.y + halfH;
}

// 全方位に大量生成（2リング）
void GameScene::ActivateDanmakuSuper_() {
	Vector3 c = player_->GetWorldPosition();
	SpawnDanmaku_(/*num=*/30, /*spd=*/0.28f, /*bounces=*/3, /*life=*/6.0f);
	SpawnDanmaku_(/*num=*/30, /*spd=*/0.22f, /*bounces=*/3, /*life=*/6.0f);
	SpawnExplosion_(c, 64, 0.9f, 0.6f);
}

// GameScene::SpawnDanmaku_()
// GameScene::SpawnDanmaku_()
void GameScene::SpawnDanmaku_(int num, float spd, int bounces, float life) {
	static constexpr float kDanmakuZBias = -0.25f; // 手前（符号は環境で ± 入れ替え可）

	const float twoPi = 6.28318530718f;
	Vector3 c = player_->GetWorldPosition();

	for (int i = 0; i < num; ++i) {
		float ang = twoPi * (float)i / (float)num;
		DanmakuBullet b{};
		b.pos = c;
		b.pos.z += kDanmakuZBias; // ★ここで“手前”へ
		b.vel = {std::cos(ang) * spd, std::sin(ang) * spd, 0.0f};
		b.r = 0.22f;
		b.life = life;
		b.bounces = bounces;
		danmakuBullets_.push_back(b);
	}
}



void GameScene::UpdateDanmakuBullets_(float dt) {
	if (danmakuBullets_.empty())
		return;

	// 全弾同じZを想定（Spawnで固定済み）
	float l, r, btm, top;
	GetCameraWorldRect_(danmakuBullets_.front().pos.z, l, r, btm, top);

	auto circleRectHit = [](const Vector3& c, float rr, const AABB& a) {
		float cx = std::clamp(c.x, a.min.x, a.max.x);
		float cy = std::clamp(c.y, a.min.y, a.max.y);
		float dx = c.x - cx, dy = c.y - cy;
		return (dx * dx + dy * dy) <= (rr * rr);
	};

	// 1セル貫通対策：軸ごとに「衝突→押し戻し→反転」を行う
	auto reflectWithBlocksAxis = [&](DanmakuBullet& b, float testX, float testY, bool axisX) -> bool {
		Vector3 c{testX, testY, b.pos.z};
		auto is = mapChipField_->GetMapChipIndexSetByPosition(c);

		const int W = (int)mapChipField_->GetNumHorizontal();
		const int H = (int)mapChipField_->GetNumVirtical();
		const int cx = std::clamp((int)is.xIndex, 0, W - 1);
		const int cy = std::clamp((int)is.yIndex, 0, H - 1);

		// 近傍チェック（3x3）
		const int ix0 = std::max(0, cx - 1), ix1 = std::min(W - 1, cx + 1);
		const int iy0 = std::max(0, cy - 1), iy1 = std::min(H - 1, cy + 1);

		const float eps = 0.001f;

		for (int iy = iy0; iy <= iy1; ++iy) {
			for (int ix = ix0; ix <= ix1; ++ix) {
				if (mapChipField_->SafeGet(ix, iy) != MapChipType::kBlock)
					continue;

				auto rc = mapChipField_->GetRectByIndex(ix, iy);
				AABB a{
				    {rc.left,  rc.bottom, -1.0f},
                    {rc.right, rc.top,    1.0f }
                };
				if (!circleRectHit(c, b.r, a))
					continue;

				if (axisX) {
					if (b.vel.x > 0)
						b.pos.x = rc.left - b.r - eps;
					else
						b.pos.x = rc.right + b.r + eps;
					b.vel.x = -b.vel.x;
				} else {
					if (b.vel.y > 0)
						b.pos.y = rc.bottom - b.r - eps;
					else
						b.pos.y = rc.top + b.r + eps;
					b.vel.y = -b.vel.y;
				}
				if (b.bounces > 0)
					--b.bounces;
				return true;
			}
		}
		return false;
	};

	for (auto& b : danmakuBullets_) {
		b.life -= dt;
		if (b.life <= 0.0f)
			continue;

		bool bounced = false;

		// --- X軸 ---
		float nx = b.pos.x + b.vel.x;
		if (reflectWithBlocksAxis(b, nx, b.pos.y, /*axisX=*/true)) {
			bounced = true;
		} else {
			b.pos.x = nx;
			// 画面端
			if (b.pos.x - b.r <= l) {
				b.pos.x = l + b.r;
				b.vel.x = -b.vel.x;
				bounced = true;
			} else if (b.pos.x + b.r >= r) {
				b.pos.x = r - b.r;
				b.vel.x = -b.vel.x;
				bounced = true;
			}
		}

		// --- Y軸 ---
		float ny = b.pos.y + b.vel.y;
		if (reflectWithBlocksAxis(b, b.pos.x, ny, /*axisX=*/false)) {
			bounced = true;
		} else {
			b.pos.y = ny;
			// 画面端
			if (b.pos.y - b.r <= btm) {
				b.pos.y = btm + b.r;
				b.vel.y = -b.vel.y;
				bounced = true;
			} else if (b.pos.y + b.r >= top) {
				b.pos.y = top - b.r;
				b.vel.y = -b.vel.y;
				bounced = true;
			}
		}

		// 跳ね返り回数を使い切ったら静かに消す
		if (bounced && b.bounces == 0) {
			b.life = -1.0f;
		}
	}

	// 後始末
	danmakuBullets_.erase(std::remove_if(danmakuBullets_.begin(), danmakuBullets_.end(), [](const DanmakuBullet& b) { return b.life <= 0.0f; }), danmakuBullets_.end());
}
void GameScene::DrawDanmakuBullets_() {
	for (const auto& b : danmakuBullets_) {
		if (b.life <= 0.0f)
			continue;
		wtDanmaku_.translation_ = b.pos; // ★Zは触らない
		float s = std::max(0.28f, b.r * 1.25f);
		wtDanmaku_.scale_ = {s, s, s};
		UpdateWorldTransform(wtDanmaku_);
		particleModel_->Draw(wtDanmaku_, camera_);
	}
}

void GameScene::DrawLifeUI_() {
	if (lifeOn_.empty())
		return;
	const int hp = std::max(0, player_ ? player_->GetHP() : 0);
	// 空（オフ）を並べる
	for (auto* s : lifeOff_)
		s->Draw();
	// 残HPぶんだけ上から点灯
	for (int i = 0; i < hp && i < (int)lifeOn_.size(); ++i)
		lifeOn_[i]->Draw();
}
