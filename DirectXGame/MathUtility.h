#pragma once
#include "KamataEngine.h"

using namespace KamataEngine;

// x軸回転行列
Matrix4x4 MakeRoteXMatrix(float radian) {
	Matrix4x4 result = {
	    1, 0, 0, 0, 0, cosf(radian), sinf(radian), 0, 0, -sinf(radian), cosf(radian), 0, 0, 0, 0, 1,
	};
	return result;
}

// Y軸回転行列
Matrix4x4 MakeRotateYMatrix(float radian) {
	Matrix4x4 result = {
	    cosf(radian), 0, -sinf(radian), 0, 0, 1, 0, 0, sinf(radian), 0, cosf(radian), 0, 0, 0, 0, 1,
	};
	return result;
}

// Z軸回転行列
Matrix4x4 MakeRotateZMatrix(float radian) {
	Matrix4x4 result = {
	    cosf(radian), sinf(radian), 0, 0, -sinf(radian), cosf(radian), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
	};
	return result;
}

// 平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, translate.x, translate.y, translate.z, 1};
	return result;
}

// ベクトルの積
Vector3 MultiplyVector(const Vector3& v, const Matrix4x4& m) {
	return {
	    v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0] + m.m[3][0], v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1] + m.m[3][1],
	    v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2] + m.m[3][2]};
}

Vector3 Multiply(float scalar, const Vector3& v) { return {scalar * v.x, scalar * v.y, scalar * v.z}; }

// 行列の積
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {
	Matrix4x4 result = {};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			for (int k = 0; k < 4; ++k) {
				result.m[i][j] += m1.m[i][k] * m2.m[k][j];
			}
		}
	}
	return result;
}

// アフィン変換行列
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {
	Matrix4x4 scaleMatrix = {scale.x, 0, 0, 0, 0, scale.y, 0, 0, 0, 0, scale.z, 0, 0, 0, 0, 1};
	Matrix4x4 rotateXMatrix = MakeRoteXMatrix(rotate.x);
	Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotate.z);
	Matrix4x4 rotateMatrix = Multiply(Multiply(rotateXMatrix, rotateYMatrix), rotateZMatrix);
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);
	return Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);
}

// 座標変換
Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix) {
	float x = vector.x * matrix.m[0][0] + vector.y * matrix.m[1][0] + vector.z * matrix.m[2][0] + matrix.m[3][0];
	float y = vector.x * matrix.m[0][1] + vector.y * matrix.m[1][1] + vector.z * matrix.m[2][1] + matrix.m[3][1];
	float z = vector.x * matrix.m[0][2] + vector.y * matrix.m[1][2] + vector.z * matrix.m[2][2] + matrix.m[3][2];
	float w = vector.x * matrix.m[0][3] + vector.y * matrix.m[1][3] + vector.z * matrix.m[2][3] + matrix.m[3][3];

	if (w != 0.0f) {
		x /= w;
		y /= w;
		z /= w;
	}

	return {x, y, z};
}

// 線形補間（Linear Interpolation）
Vector3 Lerp(const Vector3& a, const Vector3& b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t}; }
