#include "pch.h"
#include "Camera.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Camera camera;

Matrix Camera::GetViewRow() {

	Quaternion qPitch = Quaternion::CreateFromAxisAngle(Vector3(1, 0, 0), m_pitch);
	Quaternion qYaw = Quaternion::CreateFromAxisAngle(Vector3(0, 1, 0), -m_yaw);
	Quaternion q = qYaw * qPitch;
	q.Normalize();
	return Matrix::CreateTranslation(-m_eyePos) *
		Matrix::CreateFromQuaternion(q);
}

void Camera::UpdateMouse(float mouseNdcX, float mouseNdcY) {

	// 얼마나 회전할지 계산
	m_yaw = mouseNdcX * 2 * pi;     // 좌우 360도
	m_pitch = mouseNdcY * pi / 2; // 위 아래 90도
	UpdateViewDir();

}

void Camera::UpdateViewDir() {
	// 이동할 때 기준이 되는 정면/오른쪽 방향 계산

	Quaternion q = Quaternion::CreateFromYawPitchRoll(m_yaw, 0.0, 0.0f);
	q.Normalize();
	m_viewDir =
		Vector3::Transform(Vector3(0, 0, 1), Matrix::CreateFromQuaternion(q));

	m_rightDir = m_upDir.Cross(m_viewDir);
}

Matrix Camera::GetProjRow() {
	return XMMatrixPerspectiveFovLH(m_projFovAngleY, m_aspect, m_nearZ, m_farZ);
}

void Camera::UpdateKeyboard(const float dt, bool const keyPressed[256]) {

	if (keyPressed['W'])
		MoveForward(dt);
	if (keyPressed['S'])
		MoveForward(-dt);
	if (keyPressed['D'])
		MoveRight(dt);
	if (keyPressed['A'])
		MoveRight(-dt);
	if (keyPressed['E'])
		MoveUp(dt);
	if (keyPressed['Q'])
		MoveUp(-dt);

}

void Camera::MoveForward(float dt) {
	m_eyePos += m_viewDir * m_speed * dt;
}

void Camera::MoveUp(float dt) {
	m_eyePos += m_upDir * m_speed * dt;
}

void Camera::MoveRight(float dt) 
{
	m_eyePos += m_rightDir * m_speed * dt;
}

void Camera::SetAspect(float aspect)
{
	m_aspect = aspect;
}