#pragma once

#include <DirectXMath.h>
#include <vector>

class Transform
{
public:
	Transform();

	void MoveAbsolute(float x, float y, float z);
	void MoveRelative(float x, float y, float z);
	void Rotate(float p, float y, float r);
	void Scale(float x, float y, float z);

	void SetPosition(float x, float y, float z);
	void SetRotation(float p, float y, float r);
	void SetRotationQuat(float x, float y, float z, float w);
	void SetScale(float x, float y, float z);

	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetPitchYawRoll();
	DirectX::XMFLOAT3 GetScale();
	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();

	// hierarchy methods
	void AddChild(Transform* child);
	void RemoveChild(Transform* child);
	void SetParent(Transform* newParent);

	void SetTransformsFromMatrix(DirectX::XMFLOAT4X4 worldMatrix);

	Transform* GetParent();
	Transform* GetChild(unsigned int index);
	int IndexOfChild(Transform* child);
	unsigned int GetChildCount();

private:
	Transform* parent;
	std::vector<Transform*> children;

	// Raw transformation data
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 pitchYawRoll;
	DirectX::XMFLOAT3 scale;

	// World matrix and inverse transpose of the world matrix
	bool matricesDirty;
	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4X4 worldInverseTransposeMatrix;

	// Helper to update both matrices if necessary
	void UpdateMatrices();

	void MarkChildTransformsDirty();

	DirectX::XMFLOAT3 QuatToEuler(DirectX::XMFLOAT4 quat);
};

