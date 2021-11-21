#include "ThirdPersonCamera.h"

#include "Input.h"

using namespace DirectX;

ThirdPersonCamera::ThirdPersonCamera(GameEntity* entity, float aspectRatio)
{
	this->entity = entity;

	pivot = new Transform();
	XMFLOAT3 entityPos = entity->GetTransform()->GetPosition();
	pivot->SetPosition(entityPos.x, entityPos.y, entityPos.z);

	camera = new Camera(
		0, 0, -15,	// Position
		3.0f,		// Move speed
		1.0f,		// Mouse look
		aspectRatio); // Aspect ratio

	cameraPos = new Transform();
	XMFLOAT3 pos = camera->GetTransform()->GetPosition();
	cameraPos->SetPosition(pos.x, pos.y, pos.z);

	pivot->AddChild(cameraPos);
}

ThirdPersonCamera::~ThirdPersonCamera()
{
	delete camera;
	delete pivot;
	delete cameraPos;
}

Camera* ThirdPersonCamera::GetCamera()
{
	return camera;
}

void ThirdPersonCamera::Update(float dt)
{
	Transform* cameraTransform = camera->GetTransform();

	// Current speed
	float speed = dt * 1.0f;

	// Get the input manager instance
	Input& input = Input::GetInstance();

	// Movement
	if (input.KeyDown(VK_RIGHT)) { pivot->Rotate(0, speed, 0); }
	if (input.KeyDown(VK_LEFT)) { pivot->Rotate(0, -speed, 0); }
	if (input.KeyDown(VK_UP)) { pivot->Rotate(speed, 0, 0); }
	if (input.KeyDown(VK_DOWN)) { pivot->Rotate(-speed, 0, 0); }

	XMFLOAT3 entityPos = entity->GetTransform()->GetPosition();
	pivot->SetPosition(entityPos.x, entityPos.y, entityPos.z);

	camera->GetTransform()->SetTransformsFromMatrix(cameraPos->GetWorldMatrix());

	camera->UpdateViewMatrix();
}
