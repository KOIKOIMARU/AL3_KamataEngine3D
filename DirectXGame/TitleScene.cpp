#include "TitleScene.h"
#include <numbers> 

using namespace KamataEngine;

void TitleScene::Initialize() {
	titleModel_ = KamataEngine::Model::CreateFromOBJ("text", true);
	titleTransform_.Initialize();

	// X軸で立てて、Y軸で反転して前を向かせる
	titleTransform_.rotation_.x = 1.5708f; // +90度（立たせる）
	titleTransform_.rotation_.y = 3.1416f; // +180度（背面→正面）
	titleTransform_.translation_ = {0.0f, 0.0f, -45.0f};

	playerModel_ = KamataEngine::Model::CreateFromOBJ("player", true);
	playerTransform_.Initialize();
	playerTransform_.translation_ = {0.0f, -1.0f, -45.0f};

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, kFadeDuration);

	camera_ = new KamataEngine::Camera();
	camera_->Initialize(); // ★カメラ初期化を忘れずに！
}

void TitleScene::Update() {
	static float t = 0.0f;
	t += 1.0f / 60.0f;

	switch (phase_) {
	case Phase::kFadeIn:
		fade_->Update();
		if (fade_->IsFinished()) {
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		// スペースキーでフェードアウト開始
		if (Input::GetInstance()->PushKey(DIK_SPACE)) {
			fade_->Start(Fade::Status::FadeOut, kFadeDuration);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		fade_->Update();
		if (fade_->IsFinished()) {
			finished_ = true; // ゲームシーンなどへ遷移するトリガー
		}
		break;
	}

	// モデルのアニメーション（常に）
	titleTransform_.translation_.y = std::sin(t * 1.5f * std::numbers::pi_v<float>) * 0.2f + 1.0f;
	UpdateWorldTransform(titleTransform_);
	UpdateWorldTransform(playerTransform_);
	camera_->UpdateMatrix();
}

void TitleScene::Draw() { 

	// DirectXCommonインスタンスの取得
	DirectXCommon* dx_common = DirectXCommon::GetInstance();
	Model::PreDraw(dx_common->GetCommandList());
	titleModel_->Draw(titleTransform_, *camera_); 
	playerModel_->Draw(playerTransform_, *camera_);
	fade_->Draw();
	Model::PostDraw();
}

TitleScene::~TitleScene() { delete fade_; }
