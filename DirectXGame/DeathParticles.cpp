#include "DeathParticles.h"
#include <assert.h>
#include <numbers>

using namespace KamataEngine;

void DeathParticles::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position) {
	// NULLポインタチェック
	assert(model);
	assert(camera);

	// モデルの設定
	model_ = model;
	camera_ = camera;

	// ワールド変換データの初期化
	for (WorldTransform& worldTransform : worldTransforms_) {
		worldTransform.Initialize();
		worldTransform.translation_ = position; // 全て同じ位置に配置
	}

	objectColor_.Initialize();
	color_ = {1.0f, 1.0f, 1.0f, 1.0f}; // 最初は不透明
}

void DeathParticles::Update() {
	// 終了なら何もしない
	if (isFinished_)
		return;

	// 時間カウント進行
	counter_ += 1.0f / 60.0f;

	if (counter_ >= kDuration) {
		counter_ = kDuration;
		isFinished_ = true;
		return;
	}

	for (uint32_t i = 0; i < kNumParticles; ++i) {
		// 基本速度ベクトル（右向き）
		Vector3 velocity = {kSpeed, 0.0f, 0.0f};

		// 各方向に回転させる
		float angle = kAngleUnit * i;
		Matrix4x4 rotation = MakeRotateZ(angle);
		velocity = Transform(velocity, rotation);

		// 移動
		worldTransforms_[i].translation_ += velocity;

		color_.w = std::clamp(1.0f - (counter_ / kDuration), 0.0f, 1.0f); // アルファ値を減少
		objectColor_.SetColor(color_);                                    // 色変更オブジェクトにセット

		// ワールド行列更新
		UpdateWorldTransform(worldTransforms_[i]);
	}
}


void DeathParticles::Draw() {
	if (isFinished_)
		return;

	for (auto& worldTransform : worldTransforms_) {
		model_->Draw(worldTransform, *camera_, &objectColor_);
	}
}

Matrix4x4 DeathParticles::MakeRotateZ(float angle) {
	Matrix4x4 result = {};

	float cosTheta = std::cosf(angle);
	float sinTheta = std::sinf(angle);

	result.m[0][0] = cosTheta;
	result.m[0][1] = sinTheta;
	result.m[1][0] = -sinTheta;
	result.m[1][1] = cosTheta;
	result.m[2][2] = 1.0f;
	result.m[3][3] = 1.0f;

	return result;
}

// ベクトルを行列で変換（平行移動を考慮しない）
Vector3 DeathParticles::Transform(const Vector3& v, const Matrix4x4& m) {
	Vector3 result;
	result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0];
	result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1];
	result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2];
	return result;
}
