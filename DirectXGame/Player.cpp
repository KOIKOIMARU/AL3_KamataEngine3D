#include "Player.h"
#include <assert.h>

using namespace KamataEngine;

void Player::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, uint32_t textureHandle) {
	// NULLポインタチェック
	assert(model);

	// モデルの設定
	model_ = model;
	camera_ = camera;
	textureHandle_ = textureHandle;
	

	// ワールド変換データの初期化
	worldTransform_.Initialize();
}

void Player::Update() {
	// 行列を定数バッファに転送
	worldTransform_.TransferMatrix();

}

void Player::Draw() {  
   model_->Draw(worldTransform_, *camera_, textureHandle_);  
}

// デストラクタ
Player::~Player() {
}