#include "HowToScene.h"

using namespace KamataEngine;

void HowToScene::Initialize() {
	tex_ = TextureManager::Load("guid.png"); // ← 画像ファイル名
	sprite_ = Sprite::Create(tex_, {0, 0}, {1, 1, 1, 1});
	sprite_->SetSize({1280, 720});
}

void HowToScene::Update() {
	auto* in = Input::GetInstance();
	if (in->TriggerKey(DIK_SPACE) || in->TriggerKey(DIK_RETURN)) {
		finished_ = true; // 次へ進む
	}
}

void HowToScene::Draw() {
	auto* dx = DirectXCommon::GetInstance();
	Sprite::PreDraw(dx->GetCommandList(), Sprite::BlendMode::kNormal);

	if (sprite_)
		sprite_->Draw();

	Sprite::PostDraw();
}
