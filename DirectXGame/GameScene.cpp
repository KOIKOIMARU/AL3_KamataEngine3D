#include "GameScene.h"

using namespace KamataEngine;

// 関数の作成
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

void GameScene::Initialize() {

	// ファイルを指定してテクスチャを読み込む
	textureHandle_ = TextureManager::Load("sample.png");
	
	// 3Dモデルの生成
	model_ = Model::Create();

	// モデルブロックの生成
	modelBlock_ = Model::Create();

	// カメラの初期化
	camera_.Initialize();

	// デバッグカメラの初期化
	debugCamera_ = new DebugCamera(1280,720);

	// 自キャラの初期化
	player_ = new Player();
	player_->Initialize(model_, &camera_, textureHandle_);

	// 要素数
	const uint32_t kNumBlockVirtical = 10;
	const uint32_t kNumBlockHorizontal = 20;
	// ブロック1個分の横幅
	const float kBlockWidth = 2.0f;
	const float kBlockHeight = 2.0f;
	// 要素数を変更する
	worldTransformBlocks_.resize(kNumBlockVirtical);
	for (uint32_t i = 0; i < kNumBlockVirtical; i++) {
		worldTransformBlocks_[i].resize(kNumBlockHorizontal);
	};

	// ブロックの生成
	for (uint32_t i = 0; i < kNumBlockVirtical; i++) {
		for (uint32_t j = 0; j < kNumBlockHorizontal; j++) {

			if ((i % 2 == 0 ) &&(j % 2 == 0)) {
				continue; // この位置にはブロックを生成しない
			}

			worldTransformBlocks_[i][j] = new WorldTransform();
			worldTransformBlocks_[i][j]->Initialize();
			worldTransformBlocks_[i][j]->translation_.x = kBlockWidth * j;
			worldTransformBlocks_[i][j]->translation_.y = kBlockHeight * i;
		}
	}
}

void GameScene::Update() {
	// デバッグカメラの更新
	debugCamera_->Update();

	#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_C)) {
		// デバッグカメラの有効/無効を切り替える
		isDebugCameraActive = !isDebugCameraActive;
	}

	#endif 

	// カメラの処理
	if (isDebugCameraActive) {
		debugCamera_->Update();
		camera_.matView = debugCamera_->GetViewMatrix();
		camera_.matProjection = debugCamera_->GetProjectionMatrix();
		// ビュープロジェクション行列の転送
		camera_.TransferMatrix();
	} else {
		// ビュープロジェクション行列の転送と更新
		camera_.UpdateMatrix();
	}


	// 自キャラの更新
	player_->Update();

	// ブロックの更新
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue; // nullptrチェック
			}
			// アフィン変換行列の作成

			worldTransformBlock->matWorld_ = MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);

			worldTransformBlock->TransferMatrix();
		}
	}
}

void GameScene::Draw() {
	// DirectXCommonインスタンスの取得
	DirectXCommon* dx_common = DirectXCommon::GetInstance();
	Model::PreDraw(dx_common->GetCommandList());
	// 自キャラの描画
	player_->Draw();
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			if (!worldTransformBlock) {
				continue; // nullptrチェック
			}
			// ブロックの描画
			modelBlock_->Draw(*worldTransformBlock, camera_);
		}
	}
	Model::PostDraw();
}


// デストラクタ
GameScene::~GameScene() {
	// デバッグカメラの解放
	delete debugCamera_;
	// スプライトの解放
	delete model_;
	// 自キャラの解放
	delete player_;
	// モデルブロックの解放
	delete modelBlock_;
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
		//worldTransformBlockLine.clear();
	}
	worldTransformBlocks_.clear();
}
