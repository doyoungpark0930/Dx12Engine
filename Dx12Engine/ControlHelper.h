#pragma once

Vector3 GetRotatedDir(const Vector3& dir, float radian)
{
	float rad = XMConvertToRadians(radian);

	Quaternion q = Quaternion::CreateFromAxisAngle(Vector3(0, 1, 0), rad);
	Matrix rot = Matrix::CreateFromQuaternion(q);

	Vector3 result = Vector3::Transform(dir, rot);
	result.Normalize();
	return result;
}

void MoveCharacter(const float dt, bool const (&keyPressed)[256], ObjectState& objectState) { //나중에 animationController cpp와 header로 옮기기


	if (keyPressed['W'] && keyPressed['A'] && keyPressed[16] && !camera.IsMouseMoving)
	{
		float turnRad = -0.4f; //-0.4도
		camera.SetYaw(turnRad);
		camera.UpdateViewDir();

		// "현재 front에서 살짝 회전된 방향"으로 이동
		Vector3 curvedDir = GetRotatedDir(camera.GetFrontDir(), turnRad);
		objectState.pos += curvedDir * 0.04f;
		camera.SetEyePos();

	}
	else if (keyPressed['W'] && keyPressed['A'] && keyPressed[16])
	{
		objectState.pos += camera.GetForwardLeftDir() * 0.04f;

	}
	else if (keyPressed['W'] && keyPressed['D'] && keyPressed[16] && !camera.IsMouseMoving)
	{
		float turnRad = 0.4f; //0.4도
		camera.SetYaw(turnRad);
		camera.UpdateViewDir();

		// "현재 front에서 살짝 회전된 방향"으로 이동
		Vector3 curvedDir = GetRotatedDir(camera.GetFrontDir(), turnRad);
		objectState.pos += curvedDir * 0.04f;
		camera.SetEyePos();
	}
	else if (keyPressed['W'] && keyPressed['D'] && keyPressed[16])
	{
		objectState.pos += camera.GetForwardRightDir() * 0.04f;
	}
	else if (keyPressed['W'] && keyPressed[16]) //32는 스페이스바
	{
		objectState.pos += camera.GetFrontDir() * 0.04f;
	}
	else if (keyPressed['W'] && keyPressed['A'])
	{
		objectState.pos += camera.GetForwardLeftDir() * 0.015f;
	}
	else if (keyPressed['W'] && keyPressed['D'])
	{
		objectState.pos += camera.GetForwardRightDir() * 0.015f;
	}
	else if (keyPressed['W'])
	{
		objectState.pos += camera.GetFrontDir() * 0.015f;
	}
	else if (keyPressed['S'] && keyPressed['A'])
	{
		objectState.pos -= camera.GetForwardRightDir() * 0.01f;
	}
	else if (keyPressed['S'] && keyPressed['D'])
	{
		objectState.pos -= camera.GetForwardLeftDir() * 0.01f;
	}
	else if (keyPressed['A'] && keyPressed[16])
	{
		objectState.pos -= camera.GetRightDir() * 0.03f;
	}
	else if (keyPressed['A'])
	{
		objectState.pos -= camera.GetRightDir() * 0.01f;
	}
	else if (keyPressed['D'] && keyPressed[16])
	{
		objectState.pos += camera.GetRightDir() * 0.03f;
	}
	else if (keyPressed['D'])
	{
		objectState.pos += camera.GetRightDir() * 0.01f;
	}
	else if (keyPressed['S'])
	{
		objectState.pos -= camera.GetFrontDir() * 0.01f;
	}
}
float WrapAngle(float angle) //좁은 각 사이를, 한바퀴 회전해서 도는 경우 방지, 예를들어 왼쪽대각선 5.7, 가운데 0.25인경우(3.14가 pi)
{
	while (angle > pi) angle -= 2 * pi;
	while (angle < -pi) angle += 2 * pi;
	return angle;
}