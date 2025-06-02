#include "GameScene.h"
#include "transform.h"

using namespace KamataEngine;

void GameScene::Initialize() {

	// ファイルを指定してテクスチャを読み込む
	textureHandle_ = TextureManager::Load("sample.png");
	
	// 3Dモデルの生成
	model_ = Model::CreateFromOBJ("player",true);
	modelSkydome_ = Model::CreateFromOBJ("skydome", true);

	// モデルブロックの生成
	modelBlock_ = Model::Create();

	// カメラの初期化
	camera_.farZ = 1000.0f;
	camera_.Initialize();

	// デバッグカメラの初期化
	debugCamera_ = new DebugCamera(1280,720);

	// 自キャラの初期化
	player_ = new Player();
	player_->Initialize(model_, &camera_, textureHandle_);

	// 天球の初期化
	skydome_.Initialize(modelSkydome_, &camera_, textureHandle_);

	// マップチップフィールドの初期化
	mapChipField_ = new MapChipField();
	mapChipField_->LoadMapChipCsv("Resources/block.csv");

	GenerateBlocks();
}

void GameScene::Update() {
	// デバッグカメラの更新
	debugCamera_->Update();

	#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_C)) {
		// デバッグカメラの有効/無効を切り替える
		isDebugCameraActive = !isDebugCameraActive;
	}

	#endif 

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

	// 自キャラの更新
	player_->Update();

	// 天球の更新
	skydome_.Update();

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
}

void GameScene::Draw() {
	// DirectXCommonインスタンスの取得
	DirectXCommon* dx_common = DirectXCommon::GetInstance();
	Model::PreDraw(dx_common->GetCommandList());

	// 自キャラの描画
	player_->Draw();

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
	delete model_;
	delete modelSkydome_;
	// 自キャラの解放
	delete player_;
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
