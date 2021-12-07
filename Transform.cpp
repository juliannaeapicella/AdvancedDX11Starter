#include "Transform.h"

using namespace DirectX;

Transform::Transform()
{
	// Start with an identity matrix and basic transform data
	XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
	XMStoreFloat4x4(&worldInverseTransposeMatrix, XMMatrixIdentity());

	position = XMFLOAT3(0, 0, 0);
	pitchYawRoll = XMFLOAT3(0, 0, 0);
	scale = XMFLOAT3(1, 1, 1);

	// No need to recalc yet
	matricesDirty = false;

	parent = 0;
}

void Transform::MoveAbsolute(float x, float y, float z)
{
	position.x += x;
	position.y += y;
	position.z += z;
	matricesDirty = true;
	MarkChildTransformsDirty();
}

void Transform::MoveRelative(float x, float y, float z)
{
	// Create a direction vector from the params
	// and a rotation quaternion
	XMVECTOR movement = XMVectorSet(x, y, z, 0);
	XMVECTOR rotQuat = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));

	// Rotate the movement by the quaternion
	XMVECTOR dir = XMVector3Rotate(movement, rotQuat);

	// Add and store, and invalidate the matrices
	XMStoreFloat3(&position, XMLoadFloat3(&position) + dir);
	matricesDirty = true;
	MarkChildTransformsDirty();
}

void Transform::Rotate(float p, float y, float r)
{
	pitchYawRoll.x += p;
	pitchYawRoll.y += y;
	pitchYawRoll.z += r;
	matricesDirty = true;
	MarkChildTransformsDirty();
}

void Transform::Scale(float x, float y, float z)
{
	scale.x *= x;
	scale.y *= y;
	scale.z *= z;
	matricesDirty = true;
	MarkChildTransformsDirty();
}

void Transform::SetPosition(float x, float y, float z)
{
	position.x = x;
	position.y = y;
	position.z = z;
	matricesDirty = true;
	MarkChildTransformsDirty();
}

void Transform::SetRotation(float p, float y, float r)
{
	pitchYawRoll.x = p;
	pitchYawRoll.y = y;
	pitchYawRoll.z = r;
	matricesDirty = true;
	MarkChildTransformsDirty();
}

void Transform::SetRotationQuat(float x, float y, float z, float w)
{
	XMFLOAT3 euler = QuatToEuler(XMFLOAT4(x, y, z, w));
	SetRotation(euler.x, euler.y, euler.z);
}

void Transform::SetScale(float x, float y, float z)
{
	scale.x = x;
	scale.y = y;
	scale.z = z;
	matricesDirty = true;
	MarkChildTransformsDirty();
}

DirectX::XMFLOAT3 Transform::GetPosition() { return position; }

DirectX::XMFLOAT3 Transform::GetPitchYawRoll() { return pitchYawRoll; }

DirectX::XMFLOAT3 Transform::GetScale() { return scale; }


DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
	UpdateMatrices();
	return worldMatrix;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
	UpdateMatrices();
	return worldMatrix;
}

void Transform::AddChild(Transform* child)
{
	if (child != 0 && IndexOfChild(child) == -1) {
		// get matrices
		XMFLOAT4X4 world = GetWorldMatrix();
		XMMATRIX wm = XMLoadFloat4x4(&world);

		XMFLOAT4X4 childWorld = child->GetWorldMatrix();
		XMMATRIX cwm = XMLoadFloat4x4(&childWorld);

		// "subtract" parent transforms from child transforms
		XMMATRIX worldInv = XMMatrixInverse(0, wm);
		XMMATRIX relcwm = cwm * worldInv; // relative world matrix

		XMFLOAT4X4 relativeChildWorld;
		XMStoreFloat4x4(&relativeChildWorld, relcwm);
		child->SetTransformsFromMatrix(relativeChildWorld);

		children.push_back(child);
		child->parent = this;
		MarkChildTransformsDirty();
	}
}

void Transform::RemoveChild(Transform* child)
{
	int index = IndexOfChild(child);

	if (index != -1) {
		XMFLOAT4X4 childWorld = child->GetWorldMatrix();
		child->SetTransformsFromMatrix(childWorld);

		children.erase(children.begin() + index);
		child->SetParent(0);
		MarkChildTransformsDirty();
	}
}

void Transform::SetParent(Transform* newParent)
{
	parent = newParent;

	if (parent != 0 && parent->IndexOfChild(this) == -1) {
		// get matrices
		XMFLOAT4X4 pWorld = parent->GetWorldMatrix();
		XMMATRIX pwm = XMLoadFloat4x4(&pWorld);

		XMFLOAT4X4 world = GetWorldMatrix();
		XMMATRIX wm = XMLoadFloat4x4(&world);

		// "subtract" parent transforms from child transforms
		XMMATRIX worldInv = XMMatrixInverse(0, pwm);
		XMMATRIX relwm = wm * worldInv; // relative world matrix

		XMFLOAT4X4 relativeWorld;
		XMStoreFloat4x4(&relativeWorld, relwm);
		SetTransformsFromMatrix(relativeWorld);

		parent->children.push_back(this);
		parent->MarkChildTransformsDirty();
	}
	else if (parent == 0) {
		XMFLOAT4X4 world = GetWorldMatrix();
		SetTransformsFromMatrix(world);
	}
}

Transform* Transform::GetParent()
{
	return parent;
}

Transform* Transform::GetChild(unsigned int index)
{
	if (index < children.size()) 
		return children[index];

	return 0;
}

int Transform::IndexOfChild(Transform* child)
{
	auto it = std::find(children.begin(), children.end(), child);

	if (it != children.end())
		return distance(children.begin(), it);

	return -1;
}

unsigned int Transform::GetChildCount()
{
	return children.size();
}

void Transform::UpdateMatrices()
{
	// Are the matrices out of date (dirty)?
	if (matricesDirty)
	{
		// Create the three transformation pieces
		XMMATRIX trans = XMMatrixTranslationFromVector(XMLoadFloat3(&position));
		XMMATRIX rot = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&pitchYawRoll));
		XMMATRIX sc = XMMatrixScalingFromVector(XMLoadFloat3(&scale));

		// Combine and store the world
		XMMATRIX wm = sc * rot * trans;

		if (parent) {
			XMFLOAT4X4 parentWorld = parent->GetWorldMatrix();
			XMMATRIX pWorld = XMLoadFloat4x4(&parentWorld);

			wm *= pWorld;
		}

		XMStoreFloat4x4(&worldMatrix, wm);

		// Invert and transpose, too
		XMStoreFloat4x4(&worldInverseTransposeMatrix, XMMatrixInverse(0, XMMatrixTranspose(wm)));

		// All set
		matricesDirty = false;

		MarkChildTransformsDirty();
	}
}

void Transform::MarkChildTransformsDirty()
{
	for (int i = 0; i < children.size(); i++) {
		children[i]->matricesDirty = true;
		children[i]->MarkChildTransformsDirty();
	}
}

void Transform::SetTransformsFromMatrix(DirectX::XMFLOAT4X4 worldMatrix)
{
	XMVECTOR pos;
	XMVECTOR rot;
	XMVECTOR sc;
	XMMatrixDecompose(&sc, &rot, &pos, XMLoadFloat4x4(&worldMatrix));

	// convert to euler
	XMFLOAT4 quat;
	XMStoreFloat4(&quat, rot);
	pitchYawRoll = QuatToEuler(quat);

	XMStoreFloat3(&position, pos);
	XMStoreFloat3(&scale, sc);

	matricesDirty = true;
}

DirectX::XMFLOAT3 Transform::QuatToEuler(DirectX::XMFLOAT4 quat)
{
	// convert quaternion to rotation matrix
	XMMATRIX rMat = XMMatrixRotationQuaternion(XMLoadFloat4(&quat));

	XMFLOAT4X4 rot;
	XMStoreFloat4x4(&rot, rMat);

	// map values
	float pitch = (float)asin(-rot._32);
	float yaw = (float)atan2(rot._31, rot._33);
	float roll = (float)atan2(rot._12, rot._22);

	return XMFLOAT3(pitch, yaw, roll);
}
