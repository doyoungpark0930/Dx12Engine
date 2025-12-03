#pragma once
#include <unordered_map>

class ModelLoader;
class Bone;

class Animation
{
public:  
    void OnInit(aniNode* rootNode, ModelLoader* model, float duration);

    ~Animation();

    Bone* GetBone() { return m_Bones; }

    inline float GetTicksPerSecond() { return m_TicksPerSecond; }

    inline float GetDuration() { return m_Duration; }

    inline const meshNode* GetRootNode() { return m_MeshRootNode; }

    inline int GetBoneCount() { return m_BoneCounter; }

    auto& GetBoneIDMap()
    {
        return m_BoneInfoMap;
    }

private:
    void InitBones(const aniNode* animation);
    void deleteNode(aniNode* node);

    float m_Duration;
    int m_TicksPerSecond;
    int m_BoneCounter = 0;
    Bone* m_Bones;
    meshNode* m_MeshRootNode;
    std::unordered_map<std::string, UINT> m_BoneInfoMap;
};