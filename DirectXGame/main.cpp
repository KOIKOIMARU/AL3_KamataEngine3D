#include <Windows.h>
#include "KamataEngine.h"
#include "GameScene.h"
#include "TitleScene.h"
#include "ResultScene.h"  
#include "HowToScene.h"  
#include "Fade.h"    

enum class Scene { kUnknown = 0, kTitle, kHowTo, kGame, kResult };


// 追加：フェード遷移の内部状態
enum class FadeFlow { None, FadingOut, Switching, FadingIn };

// 現在シーン
Scene scene = Scene::kUnknown;


TitleScene* titleScene = nullptr;
GameScene* gameScene = nullptr; // まだ未使用でもOK
HowToScene* howtoScene = nullptr;
ResultScene* resultScene = nullptr;    

Fade gFade; // ★ 追加：グローバルで1個

// 遷移制御
FadeFlow fadeFlow = FadeFlow::None;

// ★ どこへ遷移するか（フェードアウト完了後に使用）
Scene nextScene = Scene::kUnknown;

void RequestSceneChange(Scene to); // ★ 追加：切替要求
void TryProgressFadeFlow();        // ★ 追加：状態機械を進める

void UpdateScene();

void DrawScene();

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	
	// KamataEngineの初期化
	KamataEngine::Initialize(L"LE2B_08_コイズミ_リョウ_AL3");

	using namespace KamataEngine;

	// DirectXCommonインスタンスの取得
	DirectXCommon* dx_common = DirectXCommon::GetInstance();

	    // ★ フェード初期化
	gFade.Initialize();
	// 画面サイズが可変ならここでサイズ反映（固定1280x720なら不要）
	// gFade の Sprite を内部で 1280x720 にしているため、必要なら Fade::Initialize を調整

	// ★ 起動時フェードイン
	gFade.Start(Fade::Status::FadeIn, 0.5f);

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
		// --- シーン側の更新 ---
		UpdateScene();

		// --- フェード状態機械を進める（シーン切替はここで実行） ---
		TryProgressFadeFlow();

		// --- フェード更新 ---
		gFade.Update();

		// --- 描画 ---
		dx_common->PreDraw();
		DrawScene();
		gFade.Draw(); // ★ 最前面にフェード
		dx_common->PostDraw();
	}

	// ゲームシーンの解放
	delete titleScene;
	delete gameScene;
	delete resultScene;  
	delete howtoScene;  

	// KamataEngineの終了処理
	KamataEngine::Finalize();

	return 0;
}


void UpdateScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene) {
			titleScene->Update();
			if (titleScene->IsFinished() && fadeFlow == FadeFlow::None) {
				RequestSceneChange(Scene::kHowTo); // ★ Title → HowTo に変更
			}
		}
		break;

	case Scene::kHowTo: // ← 追加
		if (howtoScene) {
			howtoScene->Update();
			if (howtoScene->IsFinished() && fadeFlow == FadeFlow::None) {
				RequestSceneChange(Scene::kGame); // HowTo → Game
			}
		}
		break;

	case Scene::kGame:
		if (gameScene) {
			gameScene->Update();
			if (gameScene->IsFinished() && fadeFlow == FadeFlow::None) {
				switch (gameScene->GetExitTo()) {
				case GameScene::ExitTo::RetryGame:
					RequestSceneChange(Scene::kGame);
					break;
				case GameScene::ExitTo::Title:
					RequestSceneChange(Scene::kTitle);
					break;
				default:
					RequestSceneChange(Scene::kResult);
					break;
				}
			}
		}
		break;

	case Scene::kResult:
		if (resultScene) {
			resultScene->Update();
			if (resultScene->IsFinished() && fadeFlow == FadeFlow::None) {
				if (resultScene->WantsRetry())
					RequestSceneChange(Scene::kGame);
				else
					RequestSceneChange(Scene::kTitle);
			}
		}
		break;
	}
}


// ★ シーン描画
void DrawScene() {
	switch (scene) {
	case Scene::kTitle:
		if (titleScene)
			titleScene->Draw();
		break;
	case Scene::kHowTo:
		if (howtoScene)
			howtoScene->Draw();
		break; // ← 追加
	case Scene::kGame:
		if (gameScene)
			gameScene->Draw();
		break;
	case Scene::kResult:
		if (resultScene)
			resultScene->Draw();
		break;
	}
}


// ★ フェード付き切替の要求（いきなり切り替えず、まず FadeOut を始める）
void RequestSceneChange(Scene to) {
	if (fadeFlow != FadeFlow::None)
		return;
	nextScene = to; // ★
	gFade.Start(Fade::Status::FadeOut, 0.4f);
	fadeFlow = FadeFlow::FadingOut;
}


void TryProgressFadeFlow() {
	switch (fadeFlow) {
	case FadeFlow::FadingOut:
		if (gFade.IsFinished()) {
			switch (nextScene) {

			case Scene::kHowTo: // ← 追加：Title→HowTo
				if (scene == Scene::kTitle) {
					delete titleScene;
					titleScene = nullptr;
				} else if (scene == Scene::kGame) {
					delete gameScene;
					gameScene = nullptr;
				} // 逆戻り対応(任意)
				howtoScene = new HowToScene();
				howtoScene->Initialize();
				break;

			case Scene::kGame:
				// Title→Game 直行は今回なし。HowTo→Game を考慮して破棄元を分岐
				if (scene == Scene::kTitle) {
					delete titleScene;
					titleScene = nullptr;
				} else if (scene == Scene::kHowTo) {
					delete howtoScene;
					howtoScene = nullptr;
				} // ← 追加
				else if (scene == Scene::kResult) {
					delete resultScene;
					resultScene = nullptr;
				} else if (scene == Scene::kGame) {
					delete gameScene;
					gameScene = nullptr;
				} // リトライ
				gameScene = new GameScene();
				gameScene->Initialize();
				break;

			case Scene::kResult: {
				ResultScene::Kind kind = gameScene->GetResultKind();
				delete gameScene;
				gameScene = nullptr;
				resultScene = new ResultScene(kind);
				resultScene->Initialize();
			} break;

			case Scene::kTitle:
				if (scene == Scene::kResult) {
					delete resultScene;
					resultScene = nullptr;
				} else if (scene == Scene::kGame) {
					delete gameScene;
					gameScene = nullptr;
				} else if (scene == Scene::kHowTo) {
					delete howtoScene;
					howtoScene = nullptr;
				} // ← 追加
				titleScene = new TitleScene();
				titleScene->Initialize();
				break;

			default:
				break;
			}
			scene = nextScene;
			fadeFlow = FadeFlow::Switching;
		}
		break;

	case FadeFlow::Switching:
		gFade.Start(Fade::Status::FadeIn, 0.4f);
		fadeFlow = FadeFlow::FadingIn;
		break;

	case FadeFlow::FadingIn:
		if (gFade.IsFinished()) {
			fadeFlow = FadeFlow::None;
			nextScene = Scene::kUnknown;
		}
		break;
	default:
		break;
	}
}
