#pragma once

class ModelLoader;
class Animation;

class Animator
{
public:
	void OnInit(Animation* Animation, int interrutibleStateNum, Matrix defaultTransform);
	void RequestAnimation(Animation* Animation);
	bool IsInterruptibleState();
	void UpdateAnimation(float dt);
	void CalculateBoneTransform(const meshNode* node, Matrix parentTransform);
	void BlendCalculateBoneTransform(const meshNode* node, Animation* CurrentAnimation, Animation* BlendingAnimation, Matrix parentTransform);

	Matrix* GetFinalBoneMatrices() { return m_FinalBoneMatrices; }
	bool IsActiveAnimation();
	~Animator();

private:
	Matrix* m_FinalBoneMatrices = nullptr;
	Matrix m_defaultTransform;
	Animation* m_InterruptibleState = nullptr;
	Animation* m_CurrentAnimation = nullptr;
	Animation* m_PendingAnimation = nullptr;
	Animation* m_BlendingAnimation = nullptr;

	int m_InterrutibleStateNum = 0;
	bool m_IsAnimationActive = false; //interruptibleState가 아닌 애니메이션이 현재 동작중인가?

	float m_NewTime;
	float m_CurrentTime;
	float m_DeltaTime;

	bool m_IsBlended = false;
	float m_EndedCurrentTime;
	float m_BlendTime;
	float m_BlendTimeLimit;

};