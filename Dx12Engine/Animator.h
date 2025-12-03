#pragma once

class ModelLoader;
class Animation;

class Animator
{
public:
	void OnInit(Animation* Animation, int animationNum, Matrix defaultTransform);
	void RequestAnimation(Animation* Animation);
	void UpdateAnimation(float dt);
	void CalculateBoneTransform(const meshNode* node, Matrix parentTransform);
	void BlendCalculateBoneTransform(const meshNode* node, Animation* CurrentAnimation, Animation* BlendingAnimation, Matrix parentTransform);

	Matrix* GetFinalBoneMatrices() { return m_FinalBoneMatrices; }
	~Animator();

private:
	Matrix* m_FinalBoneMatrices = nullptr;
	Animation* m_IdleAnimation = nullptr;
	Animation* m_CurrentAnimation = nullptr;
	Animation* m_PendingAnimation = nullptr;
	Animation* m_BlendingAnimation = nullptr;
	Matrix m_defaultTransform;


	float m_NewTime;
	float m_CurrentTime;
	float m_DeltaTime;

	bool m_IsBlended = false;
	float m_EndedCurrentTime;
	float m_BlendTime;

};