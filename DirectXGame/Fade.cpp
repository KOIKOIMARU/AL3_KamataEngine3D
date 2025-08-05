#include "Fade.h"

using namespace KamataEngine;

void Fade::Initialize() {
	// 単色白テクスチャ（白1x1）をロードして黒色で塗る
	uint32_t whiteTexHandle = KamataEngine::TextureManager::Load("white1x1.png");

	// スプライト生成（ポインタなのでdelete必要）
	sprite_ = Sprite::Create(
	    whiteTexHandle, Vector2(0.0f, 0.0f), Vector4(0.0f, 0.0f, 0.0f, 1.0f) // ← 色を黒（透明度1.0）
	);

	// サイズを画面サイズに変更（仮に1280x720とする）
	sprite_->SetSize(KamataEngine::Vector2(1280, 720));

	// 色を黒・不透明に設定（RGBA）
	sprite_->SetColor(KamataEngine::Vector4(0, 0, 0, 0.5)); // 黒、不透明
}

void Fade::Update() {
	// フェード状態による分岐
	switch (status_) {
	case Status::None:
		// 何もしない
		break;

	case Status::FadeIn:
		// 1フレーム分の秒数をカウントアップ
		counter_ += 1.0f / 60.0f;

		// 経過時間がduration_を超えないようにする
		counter_ = std::min(counter_, duration_);

		// アルファ値を1.0 → 0.0 に向かって減少（黒→透明）
		sprite_->SetColor(Vector4(0, 0, 0, 1.0f - std::clamp(counter_ / duration_, 0.0f, 1.0f)));
		break;

	case Status::FadeOut:
		// 1フレーム分の秒数をカウントアップ
		counter_ += 1.0f / 60.0f;

		// 経過時間がduration_を超えないようにする
		counter_ = std::min(counter_, duration_);

		// アルファ値を0.0 → 1.0 に向かって増加（透明→黒）
		sprite_->SetColor(Vector4(0, 0, 0, std::clamp(counter_ / duration_, 0.0f, 1.0f)));


		break;
	}
}


void Fade::Draw() {
	if (status_ == Status::None) {
		return; // フェード中でなければ描画しない（負荷軽減）
	}

	Sprite::PreDraw(DirectXCommon::GetInstance()->GetCommandList(), Sprite::BlendMode::kNormal); // アルファ合成に変更
	sprite_->Draw();
	Sprite::PostDraw();
}

void Fade::Start(Status status, float duration) {
	status_ = status;
	duration_ = duration;
	counter_ = 0.0f;
}

void Fade::Stop() { status_ = Status::None; }

bool Fade::IsFinished() const {
	switch (status_) {
	case Status::FadeIn:
	case Status::FadeOut:
		return counter_ >= duration_;
	default:
		return true;
	}
}
