#pragma once
#include <KamataEngine.h>

using namespace KamataEngine;

struct AABB {
	Vector3 min;
	Vector3 max;
};

Vector3 Lerp(const Vector3& a, const Vector3& b, float t);

void UpdateWorldTransform(KamataEngine::WorldTransform& worldTransform);