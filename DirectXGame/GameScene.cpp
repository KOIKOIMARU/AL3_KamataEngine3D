#include "GameScene.h"

using namespace KamataEngine;

void GameScene::Initialize() {

	// ファイルを指定してテクスチャを読み込む
	textureHandle_ = TextureManager::Load("sample.png");
	
	// 3Dモデルの生成
	model_ = Model::Create();

	// カメラの初期化
	camera_.Initialize();

	// 自キャラの初期化
	player_ = new Player();
	player_->Initialize(model_, &camera_, textureHandle_);
}

void GameScene::Update() {
	// 自キャラの更新
	player_->Update();
}

void GameScene::Draw() {
	// DirectXCommonインスタンスの取得
	DirectXCommon* dx_common = DirectXCommon::GetInstance();
	Model::PreDraw(dx_common->GetCommandList());
	// 自キャラの描画
	player_->Draw();
	Model::PostDraw();
}

// デストラクタ
GameScene::~GameScene() {
	// スプライトの解放
	delete model_;
	// 自キャラの解放
	delete player_;
}
