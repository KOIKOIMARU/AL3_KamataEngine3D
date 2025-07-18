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
	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetTransform = target_->GetWorldTransform();

	// 追従対象の速度を取得
	Vector3 targetVelocity = target_->GetVelocity();

	// カメラの目標座標を更新（プレイヤーの位置 + オフセット + 速度による補正）
	targetPosition_ = targetTransform.translation_ + targetOffset_ + targetVelocity * kVelocityBias;

	// 座標補間で滑らかに追従（lerp）
	camera_->translation_ = Lerp(camera_->translation_, targetPosition_, kInterpolationRate);

	// ===== 追従対象が画面外に出ないように補正 =====
	camera_->translation_.x = std::max(camera_->translation_.x, targetTransform.translation_.x + kFollowMargin.left);
	camera_->translation_.x = std::min(camera_->translation_.x, targetTransform.translation_.x + kFollowMargin.right);
	camera_->translation_.y = std::max(camera_->translation_.y, targetTransform.translation_.y + kFollowMargin.bottom);
	camera_->translation_.y = std::min(camera_->translation_.y, targetTransform.translation_.y + kFollowMargin.top);

	// 移動範囲制限
	camera_->translation_.x = std::clamp(camera_->translation_.x, movableArea_.left, movableArea_.right);
	camera_->translation_.y = std::clamp(camera_->translation_.y, movableArea_.bottom, movableArea_.top);

	// カメラのワールドトランスフォームを更新
	camera_->UpdateMatrix();
}


void CameraController::SetTarget(Player* target) {
	target_ = target; // 自キャラのポインタを設定
}

// CameraController.cpp
void CameraController::SetCamera(Camera* camera) { 
	camera_ = camera; 
}


void CameraController::Reset() {
	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetTransform = target_->GetWorldTransform();
	// 追従対象とオフセットからカメラの位置を計算
	camera_->translation_ = targetTransform.translation_ + targetOffset_;
}
