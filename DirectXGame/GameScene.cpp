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

	// 要素数
	const uint32_t kNumBlockVirtical = 10;
	const uint32_t kNumBlockHorizontal = 20;
	// ブロック1個分の横幅
	const float kBlockWidth = 2.0f;
	const float kBlockHeight = 2.0f;
	// 要素数を変更する
	worldTransformBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; i++) {
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	};

	// ブロックの生成
	for (uint32_t i = 0; i < kNumBlockVirtical; i++) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; j++) {

			if ((i % 2 == 0 ) &&(j % 2 == 0)) {
				continue; // この位置にはブロックを生成しない
			}

			worldTransformBlocks_[i][j] = new WorldTransform();
			worldTransformBlocks_[i][j]->Initialize();
			worldTransformBlocks_[i][j]->translation_.x = kBlockWidth * j;
			worldTransformBlocks_[i][j]->translation_.y = kBlockHeight * i;
		}
	}
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

