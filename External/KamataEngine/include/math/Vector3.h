#pragma once

namespace KamataEngine {

/// <summary>
/// 3次元ベクトル
/// </summary>
struct Vector3 final {
	float x;
	float y;
	float z;

	// += 演算子のオーバーロード
	Vector3& operator+=(const Vector3& rhs) {
		x += rhs.x;
		y += rhs.y;
		z += rhs.z;
		return *this;
	}
};

} 
namespace KamataEngine {

// Vector3 + Vector3 の演算子オーバーロード
inline Vector3 operator+(const Vector3& lhs, const Vector3& rhs) { return Vector3{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z}; }

}

inline KamataEngine::Vector3 operator*(const KamataEngine::Vector3& v, float scalar) { return {v.x * scalar, v.y * scalar, v.z * scalar}; }

inline KamataEngine::Vector3 operator*(float scalar, const KamataEngine::Vector3& v) {
	return v * scalar; // 上で定義したのを使う
}
