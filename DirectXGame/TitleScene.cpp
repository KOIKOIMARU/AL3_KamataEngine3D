#include "TitleScene.h"
#include <algorithm>
#include <cmath>
using namespace KamataEngine;

static uint32_t TryLoad(const char* p1, const char* p2 = nullptr) {
	uint32_t h = TextureManager::Load(p1);
	if (!h && p2)
		h = TextureManager::Load(p2);
	return h;
}

void TitleScene::Initialize() {
	texBG_ = TryLoad("gameTitle.png", "Resources/gameTitle.png");
	texLogo_ = TryLoad("titleName.png", "Resources/titleName.png");
	texPress_ = TryLoad("pressSpace.png", "Resources/pressSpace.png");

	spBG_ = Sprite::Create(texBG_, {0.0f, 0.0f}, {1, 1, 1, 1});
	if (spBG_)
		spBG_->SetSize({1280.0f, 720.0f});

	const float logoW = 500.0f, logoH = logoW;
	const float marginX = 500.0f, marginY = 300.0f;
	spLogo_ = Sprite::Create(texLogo_, {marginX, marginY}, {1, 1, 1, 1});
	if (spLogo_)
		spLogo_->SetSize({logoW, logoH});

	const float pressW = 400.0f, pressH = pressW;
	const float pressX = 24.0f, pressY = 560.0f - pressH * 0.5f;
	spPress_ = Sprite::Create(texPress_, {pressX, pressY}, {1, 1, 1, 1});
	if (spPress_)
		spPress_->SetSize({pressW, pressH});

}

void TitleScene::Update() {
	t_ += 1.0f / 60.0f;

	if (spPress_) {
		float a = 0.7f + 0.3f * std::sin(t_ * 6.0f);
		spPress_->SetColor({1, 1, 1, a});
	}

	// 決定（重複していた分は削除）
	if (Input::GetInstance()->TriggerKey(DIK_SPACE) || Input::GetInstance()->TriggerKey(DIK_RETURN)) {
		finished_ = true;
		bgmFadeOut_ = true;
	}

	// フェードアウト（0.6秒）
	if (bgmFadeOut_ && bgmVoice_ != -1) {
		auto* au = Audio::GetInstance();
		const float fadeSec = 0.6f;
		bgmVol_ = std::max(0.0f, bgmVol_ - (1.0f / fadeSec) * (1.0f / 60.0f));
		au->SetVolume(bgmVoice_, bgmVol_);
		if (bgmVol_ <= 0.0f) {
			au->StopWave(bgmVoice_);
			bgmVoice_ = -1;
		}
	}
}

void TitleScene::Draw() {
	auto* dx = DirectXCommon::GetInstance();
	Sprite::PreDraw(dx->GetCommandList(), Sprite::BlendMode::kNormal);
	if (spBG_)
		spBG_->Draw();
	if (spLogo_)
		spLogo_->Draw();
	if (spPress_)
		spPress_->Draw();
	Sprite::PostDraw();
}
