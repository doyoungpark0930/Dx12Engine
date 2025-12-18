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
        - `Resources\Pete\PeteT.dy`
        - `Resources\Pete\Running.ani`
   2. 파싱 코드
      - 확인 파일: `ModelLoader.cpp`
   3. 디버그 레이어, 루트 시그니처, 버텍스/인덱스 풀, DepthStencilView 생성
      - 확인 파일: `Renderer.cpp`
   4. 모델 생성 및 Draw 로직
      - 확인 파일: `Model.cpp`
   5. 스키닝 및 애니메이션 구현 : Bone.cpp, Animator.cpp, Animation.cpp

## 주요 구현 내용
• DirectX 12 기반 자체 렌더링 엔진 구현
  - Shadow Mapping + PCF 적용
  - Cube Mapping 기반 HDR 환경맵 및 Tone Mapping 구현
  - PBR(Material: Albedo / Metallic / Roughness / AO) 파이프라인 구축

• 3ds Max Export Plug-in 제작
  - Mesh / Animation 분리된 자체 포맷 설계
  - Skeletal Animation Skinning 구현
  - Animation 간 Blending 및 변환 로직 구현

• Asset 파이프라인 구축
  - Mixamo, CGTrader, Sketchfab, Unity Asset Store 모델 정규화
  - Mu용

• 자체 포맷 기반 로딩 최적화
  - 파일 상단(Header)에 Vertex / Index / Texture / Bone 개수 명시
  - 로딩 시작 시 전체 메모리 요구량을 즉시 산출 가능하도록 설계
  - 단일 할당 후 포인터 오프셋 방식으로 데이터 배치

  - 불필요한 Bone 제거(사용되지 않는 Bone Cull)
    · Vertex 가중치 기준으로 실제 사용 Bone만 유지
    · 스키닝 연산 및 메모리 사용량 감소

## gpu 버퍼 및 리소스 관리

Vertex / Index / ConstantBuffer:
Pool 기반으로 미리 할당 후 Sub-Allocation 방식으로 연속 관리

텍스처: Pool 미구현 상태 (현재 CreateCommittedResource 반복 호출 중)

## 향후 개선 예정
1) overframedBuffer(cpu - gpu 멀티스레딩 적용)
2)  다중 오브젝트 렌더링 시 프레임 저하 문제 분석
   => DX12 멀티스레딩 학습 및 적용 예정
3) PBR 일반 오브젝트 렌더링 시에도 톤매핑 적용
4) 텍스춰 pool 시스템 구축

포트폴리오 영상 : https://www.youtube.com/watch?v=Pzj5By9eyDg




블로그 학습 글
 https://pdy0930.tistory.com/

## 📂 관련 프로젝트

🧠 DirectX11 그래픽스 연습 포트폴리오
[👉 Practicing Graphics Skill by DX11](https://github.com/doyoungpark0930/Practcing-Graphics-Skill-by-Dx11)

