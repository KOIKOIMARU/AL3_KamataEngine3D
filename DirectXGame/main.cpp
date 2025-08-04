#include <Windows.h>
#include "KamataEngine.h"
#include "GameScene.h"
#include "TitleScene.h"

enum class Scene { kUnknown = 0,
	kTitle,
	kGame,
};

// 現在シーン
Scene scene = Scene::kUnknown;

TitleScene* titleScene = nullptr;
GameScene* gameScene = nullptr; // まだ未使用でもOK

void ChangeScene();

void UpdateScene();

void DrawScene();

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	
	// KamataEngineの初期化
	KamataEngine::Initialize(L"LE2B_08_コイズミ_リョウ_AL3");

	using namespace KamataEngine;

	// DirectXCommonインスタンスの取得
	DirectXCommon* dx_common = DirectXCommon::GetInstance();

	scene = Scene::kTitle;
	// タイトルシーンのインスタンスを作成
	titleScene = new TitleScene();
	// 初期化
	titleScene->Initialize();

	// KamataEngineのメインループ
	while (true) {
		// エンジンの更新
		if (KamataEngine::Update()) {
			break;
		}

		// 更新処理
		// シーン切り替え
		ChangeScene();
		// 現在シーン更新
		UpdateScene();


		// 描画開始
		dx_common->PreDraw();


		DrawScene();


		// 描画狩猟
		dx_common->PostDraw();
	}

	// ゲームシーンの解放
	delete titleScene;
	delete gameScene;

	// KamataEngineの終了処理
	KamataEngine::Finalize();

	return 0;
}

void ChangeScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene->IsFinished()) {
			// シーン変更
			scene = Scene::kGame;

			// 旧シーンの解放
			delete titleScene;
			titleScene = nullptr;

			// 新シーンの生成と初期化
			gameScene = new GameScene;
			gameScene->Initialize();
		}
		break;

	case Scene::kGame:
		if (gameScene->IsFinished()) {
			scene = Scene::kTitle;
			delete gameScene;
			gameScene = nullptr;

			titleScene = new TitleScene;
			titleScene->Initialize();
		}
		break;
	}
};

void UpdateScene() {
	switch (scene) {
	case Scene::kTitle:
		titleScene->Update();
		break;
	case Scene::kGame:
		gameScene->Update();
		break;
	}
}

void DrawScene() {
	switch (scene) {
	case Scene::kTitle:
		titleScene->Draw();
		break;
	case Scene::kGame:
		gameScene->Draw();
		break;
	}
}
