#pragma once
#include "KamataEngine.h"
#include "Transform.h" 
#define NOMINMAX
#include <algorithm>

#undef min
#undef max

/// <summary>
/// フェード
/// </summary>
class Fade {
public:
	enum class Status {
		None,   // フェード無し
		FadeIn, // フェードイン
		FadeOut // フェードアウト
	};

	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	// フェード開始
	void Start(Status status, float duration);

	// フェード停止
	void Stop();

	// フェード終了判定
	bool IsFinished() const;

private:
	// 現在のフェードの状態
	Status status_ = Status::None;

	KamataEngine::Sprite* sprite_ = nullptr;
	// フェード持続時間
	float duration_ = 0.0f;
	// 経過時間カウンター
	float counter_ = 0.0f;


};
