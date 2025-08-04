#include "GameScene.h"
#include "transform.h"

using namespace KamataEngine;

void GameScene::Initialize() {

	// マップチップフィールドの初期化
	mapChipField_ = new MapChipField();
	mapChipField_->LoadMapChipCsv("Resources/block.csv");


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
	Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(1, 18);


	// 自キャラの初期化
	player_ = new Player();
	player_->Initialize(playerModel_, &camera_,playerPosition);

	// 敵の初期化
	for (int32_t i = 0; i < 3; ++i) {
		Enemy* newEnemy = new Enemy();
		Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(5+i, 18);
		newEnemy->Initialize(enemyModel_, &camera_, enemyPosition);

		enemies_.push_back(newEnemy);
	}

	// 仮生成
	deathParticles_ = new DeathParticles();
	deathParticles_->Initialize(particleModel_, &camera_, playerPosition);

	// 天球の初期化
	skydome_.Initialize(modelSkydome_, &camera_, textureHandle_);

	GenerateBlocks();

	// カメラコントローラーの生成・初期化
	cameraController_ = new CameraController();
	cameraController_->SetCamera(&camera_);
	cameraController_->SetTarget(player_);
	cameraController_->Initialize();
	cameraController_->Reset();

	// その後プレイヤーに渡す
	player_->SetMapChipField(mapChipField_);

	// ゲームプレイフェーズから開始
	phase_ = Phase::kPlay;

}

void GameScene::Update() {

	#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_C)) {
		// デバッグカメラの有効/無効を切り替える
		isDebugCameraActive = !isDebugCameraActive;
	}

	#endif 

switch (phase_) {
	case Phase::kPlay:
		UpdatePlayPhase();
		break;
	case Phase::kDeath:
		UpdateDeathPhase();
		break;
	}
}

void GameScene::Draw() {
	// DirectXCommonインスタンスの取得
	DirectXCommon* dx_common = DirectXCommon::GetInstance();
	Model::PreDraw(dx_common->GetCommandList());

	// 自キャラの描画
	if (phase_ == Phase::kPlay) {
		player_->Draw();
	}


	// 敵の描画
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Draw();
		}
	}

	// デスパーティクルの描画
	if (deathParticles_) {
		deathParticles_->Draw();
	}

	// 天球の描画
	skydome_.Draw();

	// ブロックの描画
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue; // nullptrチェック
			}
			// ブロックの描画
			modelBlock_->Draw(*worldTransformBlock, camera_);
		}
	}
	Model::PostDraw();
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
	AABB aabbPlayer = player_->GetAABB();

	for (Enemy* enemy : enemies_) {
		AABB aabbEnemy = enemy->GetAABB();

		if (IsCollisionAABB(aabbPlayer, aabbEnemy)) {
			player_->OnCollision(enemy);
			enemy->OnCollision(player_);
		}
	}
}

void GameScene::UpdatePlayPhase() {
	// 天球の更新
	skydome_.Update();
	// プレイヤーの更新
	player_->Update();
	// 敵の更新
	for (Enemy* enemy : enemies_) {
		if (enemy) {
			enemy->Update();
		}
	}
	// カメラコントローラの更新
	cameraController_->Update();
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
	// 全ての当たり判定の更新
	CheckAllCollision();

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
