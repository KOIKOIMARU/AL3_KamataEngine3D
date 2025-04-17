#include "GameScene.h"

using namespace KamataEngine;

void GameScene::Initialize() {
	// ファイルを指定してテクスチャを読み込む
	textureHandle_ = TextureManager::Load("sample.png");

	// 音声データを読み込む
	soundDataHandle_ = Audio::GetInstance()->LoadWave("mokugyo.wav");

	// 音声データを再生する
	Audio::GetInstance()->PlayWave(soundDataHandle_);

	// スプライトインスタンスの生成
	sprite_ = Sprite::Create(textureHandle_, {100, 50});

	// 3Dモデルの生成
	model_ = Model::Create();

	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(100,100);

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();
	// カメラの初期化
	camera_.Initialize();

	// ライン描画が参照するカメラを指定する		
	PrimitiveDrawer::GetInstance()->SetCamera(&camera_);

	// 軸方向表示の表示を有効にする
	AxisIndicator::GetInstance()->SetVisible(true);
	// 軸方向表示が参照するビュープロジェクションを指定する
	AxisIndicator::GetInstance()->SetTargetCamera(&debugCamera_->GetCamera());


}

void GameScene::Update() {
	// スペースキーを押した瞬間
	if (Input::GetInstance()->TriggerKey(DIK_SPACE)) {
		// 音声停止
		Audio::GetInstance()->PlayWave(soundDataHandle_);
	}

	// スプライトの今の座標を取得
	Vector2 position = sprite_->GetPosition();
	// 座標を{2,1}移動
	position.x += 2.0f;
	position.y += 1.0f;
	// 移動した座標をスプライトに反映
	sprite_->SetPosition(position);

	// デバッグカメラの更新
	debugCamera_->Update();

	// デバッグテキストの表示
	#ifdef _DEBUG
	ImGui::Begin("Debug1");
	
	ImGui::Text("Kamata Tarou %d.%d.%d",2050,12,31);
	
	// float3入力ボックス
	ImGui::InputFloat3("InputFloat3", inputFloat3);
	// float3スライダー
	ImGui::SliderFloat3("SliderFloat3", inputFloat3, 0.0f, 1.0f);
	ImGui::End();
	#endif

	// デモウィンドウの表示を有効化

}

void GameScene::Draw() {
	// DirectXCommonインスタンスの取得
	DirectXCommon* dx_common = DirectXCommon::GetInstance();

	// スプライトの描画
	Sprite::PreDraw(dx_common->GetCommandList());

	// スプライトインスタンスの描画処理
	sprite_->Draw();

	// スプライトの描画後処理
	Sprite::PostDraw();

	// 3Dモデルの描画
	Model::PreDraw(dx_common->GetCommandList());

	// 3Dモデル描画処理
	model_->Draw(worldTransform_, camera_, textureHandle_);
	model_->Draw(worldTransform_, debugCamera_->GetCamera(), textureHandle_);

	// 3Dモデルの描画後処理
	Model::PostDraw();

	// ラインを描画する
	PrimitiveDrawer::GetInstance()->DrawLine3d({0, 0, 0}, {0, 10, 0}, {1.0f, 0.0f, 0.0f, 1.0f});


}

// デストラクタ
GameScene::~GameScene() {
	// スプライトの解放
	delete sprite_;
	delete model_;
	delete debugCamera_;
}
