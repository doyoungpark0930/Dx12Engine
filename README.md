## 프로젝트 개요

DirectX 12를 이용해 게임 엔진을 직접 제작한 프로젝트입니다.
메모리 관리, 로우레벨 렌더링 구조,그리고 아트파이프라인을 이해하고 구축하는데 중점을 두었습니다.


## 개발 환경 및 설정

Language: C++14 (C 스타일 메모리 관리)

Build Type: Debug 전용
→ 디버그 레이어 사용을 위해 아래 설정 필요

Windows 설정 → 시스템 -> 선택적 기능 → Graphics Tools 설치

Windows 설정 → 개발자용 → 개발자 모드 ON

OS: Windows 10 이상 (10 미만은 내장 GPU 사용)

.sln파일 실행 후, Solution우클릭 -> Manager Nuget Packages for Solution -> Microsoft.Direct3D.D3D12다운

디버깅 도구: PIX
(DirectX 11 버전에서는 RenderDoc 사용 경험 있음)

   1. 자체 포맷 파일
      - 확인 경로:
        - `Resources\Sci_fi_girl\sci_fi_girl.dy`
        - `Resources\Rumba\RumbaT.dy`
        - `Resources\Rumba\Dancing.ani`
        - `Resources\Mixamo\mixamoT.dy`
        - `Resources\Mixamo\leftWalk.ani`
   2. 파싱 코드
      - 확인 파일: `ModelLoader.cpp`
   3. 디버그 레이어, 루트 시그니처, 버텍스/인덱스 풀, DepthStencilView 생성
      - 확인 파일: `Renderer.cpp`
   4. 모델 생성 및 Draw 로직
      - 확인 파일: `Model.cpp`
   5. 스키닝 및 애니메이션 구현 : Bone.cpp, Animator.cpp, Animation.cpp

## 주요 구현 내용
DirectX12 이용

3ds max sdk를 통한 Export plug-in 제작 및 자체 포멧 시스템을 통한 모델로딩 및 스키닝, 애니메이션 적용
=> mesh 포멧(.dy) 와 animation 포멧(.ani)로 분리하고 각각 따로 로딩 적용

애니메이션 변환 간 blending

mixamo, cgtrader, sketchfab, Unity asset store등 다양한 사이트에서 모델 다운로드 후 정리 과정을 거친 export

MultiMaterial 시스템 적용


## cpu 메모리 관리

스마트 포인터 및 STL 미사용(메모리를 생각하는 습관을 기르기 위한 연습)

C 스타일 메모리 직접 제어로 힙 할당/해제 최소화

종료 시 메모리 누수 없음 확인 완료

디버깅 중 std::string만 제한적으로 사용

자체 포멧 상단에 vertex, index, texture미리 개수 출력하고 로딩 시 메모리 사이즈 확립

불필요한 Bone 제거 및 Vertex/Index 개수를 상단에 기록하여
엔진 로딩 시 필요 메모리량을 즉시 계산 가능하도록 설계

## gpu 버퍼 및 리소스 관리

Vertex / Index / ConstantBuffer:
Pool 기반으로 미리 할당 후 Sub-Allocation 방식으로 연속 관리

텍스처: Pool 미구현 상태 (현재 CreateCommittedResource 반복 호출 중)

## 향후 개선 예정
1) overframedBuffer(cpu - gpu 멀티스레딩 적용)
2)  다중 오브젝트 렌더링 시 프레임 저하 문제 분석
   => DX12 멀티스레딩 학습 및 적용 예정
3) 텍스춰 pool 시스템 구축


![Adobe Express - 녹화_2025_11_28_12_17_27_344](https://github.com/user-attachments/assets/2f660679-d6e7-4342-9202-4b89fbf6f2be)
![Adobe Express - 녹화_2025_11_21_11_49_20_790](https://github.com/user-attachments/assets/65b9ebd9-b043-4561-928f-4f7f14b23501)





블로그 학습 글
 https://pdy0930.tistory.com/

## 📂 관련 프로젝트

🧠 DirectX11 그래픽스 연습 포트폴리오
[👉 Practicing Graphics Skill by DX11](https://github.com/doyoungpark0930/Practcing-Graphics-Skill-by-Dx11)

