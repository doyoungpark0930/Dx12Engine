#include "pch.h"
#include "ModelLoader.h"
#include "Animation.h"
#include "DXHelper.h"
#include "Bone.h"
#include "Animator.h"


void Animator::OnInit(Animation* Animation, int interrutibleStateNum, Matrix defaultTransform)
{
	m_CurrentTime = 0.0;

	//Animation => ani 0 / ani 1 / ani 2 / ani 3.. 중에 interruptibleStateNum이 3이라면 ani 2까지 해당
	m_InterruptibleState = Animation; 
	m_InterrutibleStateNum = interrutibleStateNum;

	m_CurrentAnimation = &Animation[0];
	m_PendingAnimation = &Animation[0];
	m_defaultTransform = defaultTransform;
	m_FinalBoneMatrices = new Matrix[ModelMatrixNum];
}

void Animator::RequestAnimation(Animation* Animation)
{
	m_PendingAnimation = Animation;
}
bool Animator::IsInterruptibleState()
{
	for (int i = 0; i < m_InterrutibleStateNum; i++)
	{
		if (m_CurrentAnimation == &m_InterruptibleState[i]) return true;
	}

	return false;
}
void Animator::UpdateAnimation(float dt)
{
	if (!m_CurrentAnimation) return;

	m_DeltaTime = dt;
	m_NewTime = m_CurrentTime + m_CurrentAnimation->GetTicksPerSecond() * dt;
	m_IsAnimationActive = false;
	if (!m_IsBlended)
	{
		//현재 InterruptibleState 상태이고, pending이 다른동작이라면, idle끊고 바로 동작
		if (IsInterruptibleState() && m_CurrentAnimation != m_PendingAnimation)
		{
			m_BlendingAnimation = m_PendingAnimation;
			m_IsBlended = true;
			m_EndedCurrentTime = m_CurrentTime;
			m_BlendTime = 0.0f;
			m_BlendTimeLimit = IsInterruptibleState() ? 0.1f : 0.2f;
		}
		else //현재 InterruptibleState가 아니거나, current와 pending이 같은 경우에는 끝나고 다음 애니메이션 적용
		{
			if (m_NewTime >= m_CurrentAnimation->GetDuration() - 600) //-600은 walking, running을 위한 임시보정
			{
				if (m_CurrentAnimation == m_PendingAnimation)
				{
					m_CurrentAnimation = m_PendingAnimation;
					m_CurrentTime = 0.0f;
				}
				else //blending
				{
					m_BlendingAnimation = m_PendingAnimation;
					m_IsBlended = true;
					m_EndedCurrentTime = m_CurrentTime;
					m_BlendTime = 0.0f;
					m_BlendTimeLimit = IsInterruptibleState() ? 0.1f : 0.2f;
				}

			}
			else
			{
				m_CurrentTime = m_NewTime;
				m_IsAnimationActive = IsInterruptibleState() ? false : true;
			}
		}

		CalculateBoneTransform(m_CurrentAnimation->GetRootNode(), Matrix());
	}
	else //애니메이션 전환 과정(블렌딩)상태라면
	{
		m_BlendTime += dt;
		BlendCalculateBoneTransform(m_CurrentAnimation->GetRootNode(), m_CurrentAnimation, m_BlendingAnimation, Matrix());
		m_IsAnimationActive = IsInterruptibleState() ? false : true;
		if (m_BlendTime > m_BlendTimeLimit)
		{
			m_IsBlended = false;
			m_CurrentAnimation = m_BlendingAnimation;
			m_CurrentTime = 0.0f;
		}
	}



}

void Animator::BlendCalculateBoneTransform(const meshNode* node, Animation* CurrentAnimation, Animation* BlendingAnimation, Matrix parentTransform)
{
	const std::string& nodeName = node->name;

	Matrix nodeTransform;
	Matrix globalTransformation;
	auto& boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
	Bone* CurBones = CurrentAnimation->GetBone();
	Bone* BlendingBones = BlendingAnimation->GetBone();

	if (boneInfoMap.find(nodeName) != boneInfoMap.end())
	{
		int boneIndex = boneInfoMap[nodeName];

		Vector3 curPtm = CurBones[boneIndex].GetPosition(m_EndedCurrentTime);
		Quaternion curRtm = CurBones[boneIndex].GetRotation(m_EndedCurrentTime);
		Vector3 curStm = CurBones[boneIndex].GetScaling(m_EndedCurrentTime);

		Vector3 blendingPtm = BlendingBones[boneIndex].GetPosition(0);
		Quaternion blendingRtm = BlendingBones[boneIndex].GetRotation(0);
		Vector3 blendingStm = BlendingBones[boneIndex].GetScaling(0);

		float scaleFactor = m_BlendTime / m_BlendTimeLimit;

		curPtm = Vector3::Lerp(
			curPtm,
			blendingPtm,
			scaleFactor
		);

		curRtm = Quaternion::Slerp(
			curRtm,
			blendingRtm,
			scaleFactor
		);

		curStm = Vector3::Lerp(
			curStm,
			blendingStm,
			scaleFactor
		);

		nodeTransform = Matrix::CreateScale(curStm) * Matrix::CreateFromQuaternion(curRtm) * Matrix::CreateTranslation(curPtm);

		globalTransformation = nodeTransform * parentTransform;

		Matrix offset = node->offset;
		m_FinalBoneMatrices[boneIndex] = (m_defaultTransform.Invert() * offset * globalTransformation * m_defaultTransform).Transpose();

		for (int i = 0; i < node->mNumChildren; i++)
			BlendCalculateBoneTransform(node->mChildren[i], CurrentAnimation, BlendingAnimation, globalTransformation);

	}
}

void Animator::CalculateBoneTransform(const meshNode* node, Matrix parentTransform)
{
	const std::string& nodeName = node->name;

	Matrix nodeTransform;
	Matrix globalTransformation;
	auto& boneInfoMap = m_CurrentAnimation->GetBoneIDMap();
	Bone* Bones = m_CurrentAnimation->GetBone();

	if (boneInfoMap.find(nodeName) != boneInfoMap.end())
	{
		int boneIndex = boneInfoMap[nodeName];
		Bones[boneIndex].Update(m_CurrentTime);

		nodeTransform = Bones[boneIndex].GetLocalTransform();

		globalTransformation = nodeTransform * parentTransform;

		Matrix offset = node->offset;
		m_FinalBoneMatrices[boneIndex] = (m_defaultTransform.Invert() * offset * globalTransformation * m_defaultTransform).Transpose();

		for (int i = 0; i < node->mNumChildren; i++)
			CalculateBoneTransform(node->mChildren[i], globalTransformation);
	}
	else //bone이 아닌 rootnode. 
	{
		int boneIndex = m_CurrentAnimation->GetBoneCount() - 1;
		Bones[boneIndex].Update(m_CurrentTime);
		nodeTransform = Bones[boneIndex].GetLocalTransform();

		globalTransformation = nodeTransform * Matrix();
		CalculateBoneTransform(node->mChildren[0], globalTransformation);
	}

}

bool Animator::IsActiveAnimation() //interruptibleAnimation을 제외한 애니메이션이 동작중인가를 반환
{
	return m_IsAnimationActive;
}
Animator::~Animator()
{
	SafeDeleteArray(&m_FinalBoneMatrices);
}