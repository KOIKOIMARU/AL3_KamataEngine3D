#pragma once
#include <KamataEngine.h>

using namespace KamataEngine;

struct AABB {
	Vector3 min;
	Vector3 max;
};

inline KamataEngine::Vector2 operator*(const KamataEngine::Vector2& v, float s) { return {v.x * s, v.y * s}; }
inline KamataEngine::Vector2 operator*(float s, const KamataEngine::Vector2& v) { return v * s; }
inline KamataEngine::Vector2 operator+(const KamataEngine::Vector2& a, const KamataEngine::Vector2& b) { return {a.x + b.x, a.y + b.y}; }


Vector3 Lerp(const Vector3& a, const Vector3& b, float t);

void UpdateWorldTransform(KamataEngine::WorldTransform& worldTransform);