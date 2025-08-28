#pragma once

#include <d3d12.h>
#include <math\Matrix4x4.h>
#include <math\Vector3.h>
#include <type_traits>
#include <wrl.h>

namespace KamataEngine {

// ★ 追加：単位行列ヘルパー
inline Matrix4x4 Identity4x4() noexcept {
	Matrix4x4 m{}; // 0初期化
	m.m[0][0] = 1.0f;
	m.m[1][1] = 1.0f;
	m.m[2][2] = 1.0f;
	m.m[3][3] = 1.0f;
	return m;
}

// 定数バッファ用データ構造体
struct ConstBufferDataCamera {
	Matrix4x4 view = Identity4x4();       // ★ 初期化
	Matrix4x4 projection = Identity4x4(); // ★ 初期化
	Vector3 cameraPos = {0.0f, 0.0f, 0.0f};
};

/// <summary>
/// カメラ
/// </summary>
class Camera {
public:
#pragma region ビュー行列の設定
	Vector3 rotation_ = {0, 0, 0};
	Vector3 translation_ = {0, 0, -50};
#pragma endregion

#pragma region 射影行列の設定
	float fovAngleY = 45.0f * 3.141592654f / 180.0f;
	float aspectRatio = (float)16 / 9;
	float nearZ = 0.1f;
	float farZ = 1000.0f;
#pragma endregion

	// ★ 未初期化警告対策：単位行列で初期化
	Matrix4x4 matView = Identity4x4();
	Matrix4x4 matProjection = Identity4x4();

	Camera() = default;
	~Camera() = default;

	void Initialize();
	void CreateConstBuffer();
	void Map();
	void UpdateMatrix();
	void TransferMatrix();
	void UpdateViewMatrix();
	void UpdateProjectionMatrix();
	const Microsoft::WRL::ComPtr<ID3D12Resource>& GetConstBuffer() const { return constBuffer_; }

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer_;
	ConstBufferDataCamera* constMap = nullptr;
	Camera(const Camera&) = delete;
	Camera& operator=(const Camera&) = delete;
};

static_assert(!std::is_copy_assignable_v<Camera>);

} // namespace KamataEngine
