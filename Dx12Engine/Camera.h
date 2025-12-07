#pragma once

class Camera
{
public:
	void UpdateMouseDelta(float dx, float dy);
	void UpdateViewDir();
	Matrix GetViewRow();
	void SetAspect(float aspect);
	Matrix GetProjRow();
	void UpdateKeyboard(const float dt, bool const (&keyPressed)[256]);
	void MoveForward(float dt);
	void MoveUp(float dt);
	void MoveRight(float dt);

	void SetCharacterPos(Vector3 Pos);
	void SetEyePos();
	Vector3 GetFrontDir();
	Vector3 GetRightDir();
	Vector3 GetForwardLeftDir();
	Vector3 GetForwardRightDir();
	void SetYaw(float turnRad);

	Vector3 m_eyePos = Vector3(0.0f, 0.0f, -5.0f);

	bool IsFirstPersonView = false;
	bool IsMouseMoving = false;
	
private:
	Vector3 m_viewDir = Vector3(0.0f, 0.0f, 1.0f);
	Vector3 m_upDir = Vector3(0.0f, 1.0f, 0.0f);
	Vector3 m_rightDir = Vector3(1.0f, 0.0f, 0.0f);
	Vector3 m_frontDir = Vector3(0.0f, 0.0f, 1.0f);
	Vector3 m_forwardLeftDir = Vector3(1.0f, 0.0f, 0.0f);
	Vector3 m_forwardRightDir = Vector3(1.0f, 0.0f, 0.0f);

	Vector3 m_characterPosition = Vector3(0.0f, 0.0f, 0.0f);


	// roll, pitch, yaw
	// https://en.wikipedia.org/wiki/Aircraft_principal_axes
	float m_yaw = -0.618501f, m_pitch = -0.0785397f;

	float m_projFovAngleY = pi / 4;
	float m_nearZ = 0.01f;
	float m_farZ = 300.0f;
	float m_aspect;

	float m_speed = 3.0f;
};

extern Camera camera;