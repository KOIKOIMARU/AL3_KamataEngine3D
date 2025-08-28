#include "ResultScene.h"
#include <cmath>
using namespace KamataEngine;

namespace {
// 画面サイズ
constexpr float kW = 1280.0f;
constexpr float kH = 720.0f;
// ロゴは正方形素材
constexpr float kSquareTex = 768.0f;

// 中央(cx,cy)に幅w,高さhで置く時の左上座標に変換
inline Vector2 ToTopLeft(float cx, float cy, float w, float h) { return {cx - w * 0.5f, cy - h * 0.5f}; }

// Resources/ 有無の両対応ローダ
uint32_t TryLoad(const char* p1, const char* p2 = nullptr) {
	uint32_t h = TextureManager::Load(p1);
	if (!h && p2)
		h = TextureManager::Load(p2);
	return h;
}
} // namespace

void ResultScene::Initialize() {
	// 種類ごとに画像を切替
	if (kind_ == Kind::GameOver) {
		texBG_ = TryLoad("gameOver.png", "Resources/gameOver.png");
		texLogo_ = TryLoad("gameOverName.png", "Resources/gameOverName.png");
	} else { // Clear
		texBG_ = TryLoad("gameClear.png", "Resources/gameClear.png");
		texLogo_ = TryLoad("gameClearName.png", "Resources/gameClearName.png");
	}
	texPress_ = TryLoad("pressSpace.png", "Resources/pressSpace.png");

	// 背景：画面いっぱい（左上原点系想定）
	spBG_ = Sprite::Create(texBG_, {0, 0}, {1, 1, 1, 1});
	if (spBG_)
		spBG_->SetSize({kW, kH});

	// ロゴ：画面やや上寄りに中央配置
	const float logoW = 560.0f;                            // 好みで調整
	const float logoH = logoW * (kSquareTex / kSquareTex); // 正方形なので=logoW
	Vector2 lp = ToTopLeft(kW * 0.5f, 280.0f, logoW, logoH);
	spLogo_ = Sprite::Create(texLogo_, lp, {1, 1, 1, 1});
	if (spLogo_)
		spLogo_->SetSize({logoW, logoH});

	// PRESS SPACE：下部中央
	const float pressW = 420.0f;
	Vector2 pp = ToTopLeft(kW * 0.5f, 560.0f, pressW, pressW);
	spPress_ = Sprite::Create(texPress_, pp, {1, 1, 1, 1});
	if (spPress_)
		spPress_->SetSize({pressW, pressW});

}

void ResultScene::Update() {
	t_ += 1.0f / 60.0f;

	// Press Space の点滅
	if (spPress_) {
		float a = 0.65f + 0.35f * std::sin(t_ * 6.0f);
		spPress_->SetColor({1, 1, 1, a});
	}

	auto* in = Input::GetInstance();
	if (in->TriggerKey(DIK_SPACE)) { // タイトルへ戻る
		retry_ = false;
		finished_ = true;
	}
	if (in->TriggerKey(DIK_R)) { // リトライ
		retry_ = true;
		finished_ = true;
	}
}

void ResultScene::Draw() {
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
