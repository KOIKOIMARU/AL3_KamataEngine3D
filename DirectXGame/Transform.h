#pragma once
#include <KamataEngine.h>

using namespace KamataEngine;

Vector3 Lerp(const Vector3& a, const Vector3& b, float t);

void UpdateWorldTransform(KamataEngine::WorldTransform& worldTransform);