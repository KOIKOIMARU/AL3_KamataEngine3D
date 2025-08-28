#pragma once
#include "CameraController.h"
#include "Player.h"

using namespace KamataEngine;

void CameraController::Initialize() {
	// カメラのワールドトランスフォームを初期化
	camera_->Initialize();

	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetTransform = target_->GetWorldTransform();

	// 追従対象とオフセットからカメラの位置を計算
	Vector3 targetPos = targetTransform.translation_ + targetOffset_;

	// 範囲内に収める（X, Y だけ clamp）
	targetPos.x = std::clamp(targetPos.x, movableArea_.left, movableArea_.right);
	targetPos.y = std::clamp(targetPos.y, movableArea_.bottom, movableArea_.top);

	// カメラ位置を設定
	camera_->translation_ = targetPos;

	// カメラの行列を更新
	camera_->UpdateMatrix();
}

void CameraController::Update() {
	const WorldTransform& targetTransform = target_->GetWorldTransform();
	const Vector3 targetVelocity = target_->GetVelocity();
	const float dt = 1.0f / 60.0f;

	// タイマー
	if (hitstop_ > 0.0f)
		hitstop_ = std::max(0.0f, hitstop_ - dt);
	if (shakeTime_ > 0.0f)
		shakeTime_ = std::max(0.0f, shakeTime_ - dt);

	if (hitstop_ <= 0.0f) {
		targetPosition_ = targetTransform.translation_ + targetOffset_ + targetVelocity * kVelocityBias;
		camera_->translation_ = Lerp(camera_->translation_, targetPosition_, kInterpolationRate);

    // 追従の中心は “ターゲット＋オフセット”
		Vector3 focus = targetTransform.translation_ + Vector3{targetOffset_.x, targetOffset_.y, 0.0f};

// CameraController.cpp（Update内のこの4行を kFollowMargin → followMargin_ に）
		camera_->translation_.x = std::max(camera_->translation_.x, targetTransform.translation_.x + followMargin_.left);
		camera_->translation_.x = std::min(camera_->translation_.x, targetTransform.translation_.x + followMargin_.right);
		camera_->translation_.y = std::max(camera_->translation_.y, targetTransform.translation_.y + followMargin_.bottom);
		camera_->translation_.y = std::min(camera_->translation_.y, targetTransform.translation_.y + followMargin_.top);


		// 可動域
		camera_->translation_.x = std::clamp(camera_->translation_.x, movableArea_.left, movableArea_.right);
		camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);
	}

	// シェイク：clampの“後”にオフセットで加算
	if (shakeTime_ > 0.0f && shakeAmp_ > 0.0f) {
		// 位相進行
		const float twoPi = 6.28318530718f;
		shakePhaseX_ += shakeFreq_ * dt * twoPi;
		shakePhaseY_ += shakeFreq_ * dt * twoPi * 0.85f;

		// 指数減衰（なめらか系）
		const float tau = 0.22f;
		float amp = shakeAmp_ * std::exp(-(1.0f - std::exp(-shakeTime_ / tau)));

		// 基本波 + 少量ノイズ
		float nx = std::sin(shakePhaseX_) + 0.25f * unit_(rng_);
		float ny = std::sin(shakePhaseY_) + 0.25f * unit_(rng_);

		// オフセット
		Vector3 sh = {nx * amp, ny * amp, 0.0f};

		// 微小ロール（必要ならカメラ行列回転で適用。簡易はオフのまま）
		// float roll = (nx - ny) * amp * shakeRotScale_;
		// （行列回転を使う実装に差し替えるならここで）

		camera_->translation_ = camera_->translation_ + sh;
	}

	camera_->UpdateMatrix();
}

void CameraController::ForceSnap() {
	if (!camera_ || !target_)
		return;

	const WorldTransform& t = target_->GetWorldTransform();
	Vector3 p = t.translation_ + targetOffset_;

	// 可動域に収めてから反映
	p.x = std::clamp(p.x, movableArea_.left, movableArea_.right);
	p.y = std::clamp(p.y, movableArea_.bottom, movableArea_.top);

	camera_->translation_ = p;
	camera_->UpdateMatrix();
}


void CameraController::SetTarget(Player* target) {
	target_ = target; // 自キャラのポインタを設定
}

// CameraController.cpp
void CameraController::SetCamera(Camera* camera) { 
	camera_ = camera; 
}


// Reset を“素直に”直す（今の実装だとClampしておらず初期ズレの原因）
void CameraController::Reset() {
	if (!camera_ || !target_)
		return;

	const WorldTransform& t = target_->GetWorldTransform();
	Vector3 p = t.translation_ + targetOffset_;

	p.x = std::clamp(p.x, movableArea_.left, movableArea_.right);
	p.y = std::clamp(p.y, movableArea_.bottom, movableArea_.top);

	camera_->translation_ = p;
	camera_->UpdateMatrix();
}
