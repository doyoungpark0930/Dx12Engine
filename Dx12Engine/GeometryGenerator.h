#pragma once

class ModelLoader;
class GeometryGenerator
{
public:
	static MeshDataInfo ReadMeshFromFile(char* basePath, const char* filename, const char** animationNames, int animationNum);
	static void Normalize(const Vector3 center,
		const float longestLength, MeshDataInfo& result, ModelLoader& modelLoader);

	static JustMeshData MakeSquare(const float scale);
	static JustMeshData MakeBox(const float scale);

};