#include "Skydome.h"
#include <assert.h>

using namespace KamataEngine;

void Skydome::Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, uint32_t textureHandle) {
	assert(model);
	assert(camera);

	model_ = model;
	camera_ = camera;
	textureHandle_ = textureHandle;

	worldTransform_.Initialize();
}

void Skydome::Update() {
	worldTransform_.TransferMatrix();
}

void Skydome::Draw() {
	model_->Draw(worldTransform_, *camera_);
}
