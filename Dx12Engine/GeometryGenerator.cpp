#include "pch.h"
#include "ModelLoader.h"
#include "GeometryGenerator.h"

using namespace DirectX;

MeshDataInfo GeometryGenerator::ReadMeshFromFile(char* basePath, const char* filename, const char** animationNames, int animationNum)
{
	ModelLoader modelLoader;
	MeshDataInfo result;
	modelLoader.Load(basePath, filename);

	result.Materials = modelLoader.m_materials;
	result.materialNum = modelLoader.m_materialNum;
	result.meshes = modelLoader.m_meshes;
	result.meshNum = modelLoader.m_meshesNum;
	result.rootNode = modelLoader.rootNode;

	if (animationNum > 0)
	{
		modelLoader.AnimationLoad(basePath, animationNames, animationNum);
		result.m_animations = modelLoader.m_animations;
	}

	Normalize(Vector3(0.0f), 1.0f, result, modelLoader);

	return result;

}

void GeometryGenerator::Normalize(const Vector3 center,
	const float longestLength,
	MeshDataInfo& result, ModelLoader& modelLoader)
{

	// Normalize vertices
	Vector3 vmin(1000, 1000, 1000);
	Vector3 vmax(-1000, -1000, -1000);

	for (int i = 0; i < result.meshNum; i++) {
		for (int j = 0; j < result.meshes[i].verticesNum; j++) {
			vmin.x = XMMin(vmin.x, result.meshes[i].vertices[j].Pos.x);
			vmin.y = XMMin(vmin.y, result.meshes[i].vertices[j].Pos.y);
			vmin.z = XMMin(vmin.z, result.meshes[i].vertices[j].Pos.z);
			vmax.x = XMMax(vmax.x, result.meshes[i].vertices[j].Pos.x);
			vmax.y = XMMax(vmax.y, result.meshes[i].vertices[j].Pos.y);
			vmax.z = XMMax(vmax.z, result.meshes[i].vertices[j].Pos.z);
		}
	}

	float dx = vmax.x - vmin.x, dy = vmax.y - vmin.y, dz = vmax.z - vmin.z;
	float scale = longestLength / XMMax(XMMax(dx, dy), dz);
	Vector3 translation = -(vmin + vmax) * 0.5f + center; //¿øÁ¡À¸·Î ¿Å±ä ÈÄ + center

	for (int i = 0; i < result.meshNum; i++) {
		for (int j = 0; j < result.meshes[i].verticesNum; j++) {
			result.meshes[i].vertices[j].Pos = (result.meshes[i].vertices[j].Pos + translation) * scale;

		}
	}

	modelLoader.defaultTransform = Matrix::CreateTranslation(translation) * Matrix::CreateScale(scale);
	result.m_defaultTransform = modelLoader.defaultTransform;
}

JustMeshData GeometryGenerator::MakeSquare(const float scale, const float texScale) {
	JustMeshData result;
	Vertex* vertices = new Vertex[4];
	UINT* indices = new UINT[6];

	vertices[0].Pos = Vector3(-1.0f, 1.0f, 0.0f) * scale;
	vertices[1].Pos = Vector3(1.0f, 1.0f, 0.0f) * scale;
	vertices[2].Pos = Vector3(1.0f, -1.0f, 0.0f) * scale;
	vertices[3].Pos = Vector3(-1.0f, -1.0f, 0.0f) * scale;

	vertices[0].Normal = Vector3(0.0f, 0.0f, -1.0f);
	vertices[1].Normal = Vector3(0.0f, 0.0f, -1.0f);
	vertices[2].Normal = Vector3(0.0f, 0.0f, -1.0f);
	vertices[3].Normal = Vector3(0.0f, 0.0f, -1.0f);

	vertices[0].Tex = Vector2(0.0f, 0.0f) * texScale;
	vertices[1].Tex = Vector2(1.0f, 0.0f) * texScale;
	vertices[2].Tex = Vector2(1.0f, 1.0f) * texScale;
	vertices[3].Tex = Vector2(0.0f, 1.0f) * texScale;

	vertices[0].tangent = Vector3(1.0f, 0.0f, 0.0f);
	vertices[1].tangent = Vector3(1.0f, 0.0f, 0.0f);
	vertices[2].tangent = Vector3(1.0f, 0.0f, 0.0f);
	vertices[3].tangent = Vector3(1.0f, 0.0f, 0.0f);

	UINT temp[6] = {
		0, 1, 2, 0, 2, 3
	};

	memcpy(indices, temp, sizeof(temp));

	result.vertices = vertices;
	result.verticesNum = 4;
	result.indices = indices;
	result.indicesNum = 6;

	return result;
}

JustMeshData GeometryGenerator::MakeBox(const float scale) {

	JustMeshData result;

	Vertex* vertices = new Vertex[24];
	UINT* indices = new UINT[36];

	//À­¸é
	vertices[0].Pos = Vector3(-1.0f, 1.0f, -1.0f) * scale;
	vertices[1].Pos = Vector3(-1.0f, 1.0f, 1.0f) * scale;
	vertices[2].Pos = Vector3(1.0f, 1.0f, 1.0f) * scale;
	vertices[3].Pos = Vector3(1.0f, 1.0f, -1.0f) * scale;

	vertices[0].Normal = Vector3(0, 1, 0);
	vertices[1].Normal = Vector3(0, 1, 0);
	vertices[2].Normal = Vector3(0, 1, 0);
	vertices[3].Normal = Vector3(0, 1, 0);

	vertices[0].Tex = Vector2(0, 0);
	vertices[1].Tex = Vector2(1, 0);
	vertices[2].Tex = Vector2(1, 1);
	vertices[3].Tex = Vector2(0, 1);


	//¾Æ·§¸é
	vertices[4].Pos = Vector3(-1.0f, -1.0f, -1.0f) * scale;
	vertices[5].Pos = Vector3(1.0f, -1.0f, -1.0f) * scale;
	vertices[6].Pos = Vector3(1.0f, -1.0f, 1.0f) * scale;
	vertices[7].Pos = Vector3(-1.0f, -1.0f, 1.0f) * scale;

	vertices[4].Normal = Vector3(0, -1, 0);
	vertices[5].Normal = Vector3(0, -1, 0);
	vertices[6].Normal = Vector3(0, -1, 0);
	vertices[7].Normal = Vector3(0, -1, 0);

	vertices[4].Tex = Vector2(0, 0);
	vertices[5].Tex = Vector2(1, 0);
	vertices[6].Tex = Vector2(1, 1);
	vertices[7].Tex = Vector2(0, 1);

	//¾Õ¸é
	vertices[8].Pos = Vector3(-1.0f, -1.0f, -1.0f) * scale;
	vertices[9].Pos = Vector3(-1.0f, 1.0f, -1.0f) * scale;
	vertices[10].Pos = Vector3(1.0f, 1.0f, -1.0f) * scale;
	vertices[11].Pos = Vector3(1.0f, -1.0f, -1.0f) * scale;

	vertices[8].Normal = Vector3(0, 0, -1);
	vertices[9].Normal = Vector3(0, 0, -1);
	vertices[10].Normal = Vector3(0, 0, -1);
	vertices[11].Normal = Vector3(0, 0, -1);

	vertices[8].Tex = Vector2(0, 0);
	vertices[9].Tex = Vector2(1, 0);
	vertices[10].Tex = Vector2(1, 1);
	vertices[11].Tex = Vector2(0, 1);

	//µÞ¸é
	vertices[12].Pos = Vector3(-1.0f, -1.0f, 1.0f) * scale;
	vertices[13].Pos = Vector3(1.0f, -1.0f, 1.0f) * scale;
	vertices[14].Pos = Vector3(1.0f, 1.0f, 1.0f) * scale;
	vertices[15].Pos = Vector3(-1.0f, 1.0f, 1.0f) * scale;

	vertices[12].Normal = Vector3(0, 0, 1);
	vertices[13].Normal = Vector3(0, 0, 1);
	vertices[14].Normal = Vector3(0, 0, 1);
	vertices[15].Normal = Vector3(0, 0, 1);

	vertices[12].Tex = Vector2(0, 0);
	vertices[13].Tex = Vector2(1, 0);
	vertices[14].Tex = Vector2(1, 1);
	vertices[15].Tex = Vector2(0, 1);

	//¿ÞÂÊ
	vertices[16].Pos = Vector3(-1.0f, -1.0f, 1.0f) * scale;
	vertices[17].Pos = Vector3(-1.0f, 1.0f, 1.0f) * scale;
	vertices[18].Pos = Vector3(-1.0f, 1.0f, -1.0f) * scale;
	vertices[19].Pos = Vector3(-1.0f, -1.0f, -1.0f) * scale;

	vertices[16].Normal = Vector3(-1, 0, 0);
	vertices[17].Normal = Vector3(-1, 0, 0);
	vertices[18].Normal = Vector3(-1, 0, 0);
	vertices[19].Normal = Vector3(-1, 0, 0);

	vertices[16].Tex = Vector2(0, 0);
	vertices[17].Tex = Vector2(1, 0);
	vertices[18].Tex = Vector2(1, 1);
	vertices[19].Tex = Vector2(0, 1);

	//¿À¸¥ÂÊ
	vertices[20].Pos = Vector3(1.0f, -1.0f, 1.0f) * scale;
	vertices[21].Pos = Vector3(1.0f, -1.0f, -1.0f) * scale;
	vertices[22].Pos = Vector3(1.0f, 1.0f, -1.0f) * scale;
	vertices[23].Pos = Vector3(1.0f, 1.0f, 1.0f) * scale;

	vertices[20].Normal = Vector3(1, 0, 0);
	vertices[21].Normal = Vector3(1, 0, 0);
	vertices[22].Normal = Vector3(1, 0, 0);
	vertices[23].Normal = Vector3(1, 0, 0);

	vertices[20].Tex = Vector2(0, 0);
	vertices[21].Tex = Vector2(1, 0);
	vertices[22].Tex = Vector2(1, 1);
	vertices[23].Tex = Vector2(0, 1);

	UINT temp[36] = {
	0,  1,  2,  0,  2,  3,
	4,  5,  6,  4,  6,  7,
	8,  9, 10,  8, 10, 11,
	12, 13, 14, 12, 14, 15,
	16, 17, 18, 16, 18, 19,
	20, 21, 22, 20, 22, 23
	};

	memcpy(indices, temp, sizeof(temp));

	result.vertices = vertices;
	result.verticesNum = 24;
	result.indices = indices;
	result.indicesNum = 36;

	return result;
}