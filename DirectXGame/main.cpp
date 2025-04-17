#include <Windows.h>
#include "KamataEngine.h"
#include "GameScene.h"

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	
	// KamataEngineの初期化
	KamataEngine::Initialize(L"LE2B_08_コイズミ_リョウ_AL3");

	using namespace KamataEngine;

	// DirectXCommonインスタンスの取得
	DirectXCommon* dx_common = DirectXCommon::GetInstance();

	// ゲームシーンのインスタンスを作成
	GameScene* gameScene = new GameScene();
	// ゲームシーンの初期化
	gameScene->Initialize();

	// ImGuiManagerのインスタンスの取得
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();

	// KamataEngineのメインループ
	while (true) {
		// エンジンの更新
		if (KamataEngine::Update()) {
			break;
		}

		// ImGui受付開始
		imguiManager->Begin();

		// ゲームシーンの更新
		gameScene->Update();

		// ImGui受付終了
		imguiManager->End();

		// 描画開始
		dx_common->PreDraw();

		// ゲームシーンの描画
		gameScene->Draw();

		// 軸表示の描画
		AxisIndicator::GetInstance()->Draw();

		// ImGuiの描画
		imguiManager->Draw();

		// 描画狩猟
		dx_common->PostDraw();
	}

	// ゲームシーンの解放
	delete gameScene;

	// KamataEngineの終了処理
	KamataEngine::Finalize();

	return 0;
}