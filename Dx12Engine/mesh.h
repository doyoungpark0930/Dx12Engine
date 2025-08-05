#pragma once


using namespace DirectX;

Vertex* CreateTriangleVertex()
{
	Vertex* vertices = new Vertex[3]
	{
		{ { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
	};
	return vertices;
}

Vertex* CreateSquareVertex()
{
	Vertex* vertices = new Vertex[6]
	{
		{ { -0.75f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { -0.25f, 0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },

		{ { -0.75f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { -0.75f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } }
	};

	return vertices;
}

