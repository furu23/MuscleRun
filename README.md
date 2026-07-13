# MuscleRun

Unreal Engine 5로 제작한 무한 러너 프로젝트입니다.  
플레이어 이동과 장애물·아이템 상호작용뿐 아니라, 진행 방향에 맞춰 타일을 생성하고 지난 구간을 정리하는 런타임 구조를 구현했습니다.

[![MuscleRun 플레이 영상](https://img.youtube.com/vi/cj1qKMBmgKA/maxresdefault.jpg)](https://youtu.be/cj1qKMBmgKA)

> [!WARNING]
> 이 저장소는 **완전한 배포본이 아니라 C++ 코드 아카이브**입니다. 상용 에셋을 포함한 `Content/`와 Blueprint, Map, Enhanced Input 설정 등이 공개 저장소에서 제외되어 있어 이 저장소만으로는 프로젝트를 빌드하거나 실행할 수 없습니다.

## 프로젝트 정보

| 항목 | 내용 |
| --- | --- |
| 장르 | 3D Infinite Runner |
| 엔진 | Unreal Engine 5.4 |
| 개발 시작 | 2025-07-24 |
| 상태 | 개발 완료 |
| 팀 구성 | 4인 |
| 담당 | 팀장, PM, 리드 디자인, 게임플레이 시스템 |

## 저장소 범위

### 포함된 내용

- 게임플레이 C++ 소스 코드
- 타일 생성·제거와 진행 방향 전환 로직
- 캐릭터 이동, 점프 버퍼, 슬라이드 등 플레이어 로직
- 장애물·아이템·체력·아이템 효과 관련 코드
- 세이브 매니저와 게임 진행 관련 클래스
- 프로젝트 설정 일부와 `.uproject`

### 포함되지 않은 내용

- 상용 에셋 및 프로젝트 제작에 사용한 전체 `Content/`
- Blueprint 클래스와 레벨 Map
- 캐릭터 애니메이션, 머티리얼, 사운드
- `InputAction`과 `InputMappingContext` 에셋
- 실제 패키징에 필요한 일부 의존 리소스

따라서 이 저장소는 실행 절차보다 **설계와 구현을 검토하기 위한 자료**에 가깝습니다. 실제 플레이 결과는 상단 영상에서 확인할 수 있습니다.

## 추천 열람 순서

1. [플레이 영상](https://youtu.be/cj1qKMBmgKA)으로 게임의 전체 흐름을 확인합니다.
2. [`TileManager.cpp`](Source/MuscleRun/Object/System/TileManager.cpp)에서 타일 생명주기를 확인합니다.
3. [`MRPlayerCharacter.cpp`](Source/MuscleRun/Character/MRPlayerCharacter.cpp)에서 입력과 이동 상태를 확인합니다.
4. [`SpawnLocationComponent.cpp`](Source/MuscleRun/Object/Tile/Component/SpawnLocationComponent.cpp)와 [`DA_SpawnableObjects.h`](Source/MuscleRun/Object/Tile/DataAsset/DA_SpawnableObjects.h)에서 스폰 데이터 구조를 확인합니다.
5. 더 자세한 설명은 [타일 및 스폰 루프 문서](docs/tile-and-spawn-loop.md)를 참고합니다.

## 핵심 게임플레이 흐름

```mermaid
flowchart LR
    A["플레이어가 현재 타일을 통과"] --> B["진행 방향과 타일 끝 지점 판정"]
    B --> C["다음 타일 그룹 생성"]
    C --> D["스폰 위치와 Data Asset로 오브젝트 배치"]
    D --> E["오래된 타일 그룹 제거"]
    E --> A
```

## 주요 구현

### 타일 생성과 정리

`TileManager`가 현재 진행 방향과 플레이어 위치를 기준으로 다음 타일을 생성하고, 더 이상 필요하지 않은 오래된 타일 그룹을 제거합니다. 직선 구간뿐 아니라 회전 구간에서도 다음 생성 방향이 자연스럽게 이어지도록 방향 상태를 관리합니다.

### 데이터 기반 오브젝트 스폰

타일 안의 `SpawnLocationComponent`가 배치 지점을 제공하고, `DA_SpawnableObjects`가 생성 가능한 오브젝트 구성을 데이터로 분리합니다. 레벨을 직접 수정하지 않고도 스폰 후보와 배치 규칙을 조정할 수 있도록 구성했습니다.

### 플레이어 액션과 상태 처리

`MRPlayerCharacter`는 좌우 이동, 점프, 슬라이드, 방향 전환에 필요한 상태를 관리합니다. 점프 입력을 즉시 처리할 수 없는 순간에도 의도를 잠시 보존하는 점프 버퍼를 두어 조작감을 보완했습니다.

### 게임 진행 데이터

`MRSaveManager`를 중심으로 저장 데이터 접근을 분리해, 게임플레이 클래스가 저장 방식의 세부 구현에 직접 의존하지 않도록 했습니다.

## 코드 내비게이션

| 관심 영역 | 시작 파일 | 확인할 내용 |
| --- | --- | --- |
| 타일 생명주기 | [`TileManager.cpp`](Source/MuscleRun/Object/System/TileManager.cpp) | 통과 판정, 다음 타일 생성, 오래된 타일 제거, 진행 방향 |
| 플레이어 로직 | [`MRPlayerCharacter.cpp`](Source/MuscleRun/Character/MRPlayerCharacter.cpp) | 이동, 점프 버퍼, 슬라이드, 회전 입력 |
| 스폰 지점 | [`SpawnLocationComponent.cpp`](Source/MuscleRun/Object/Tile/Component/SpawnLocationComponent.cpp) | 타일 내부 오브젝트 배치 지점 |
| 스폰 데이터 | [`DA_SpawnableObjects.h`](Source/MuscleRun/Object/Tile/DataAsset/DA_SpawnableObjects.h) | 생성 대상과 확률·구성 데이터 |
| 저장 시스템 | [`MRSaveManager.cpp`](Source/MuscleRun/Private/Save/MRSaveManager.cpp) | 저장 데이터 접근과 생명주기 |
| 게임 규칙 | [`MRGameMode.cpp`](Source/MuscleRun/Sys/GameMode/MRGameMode.cpp) | 게임 진행 규칙과 초기화 |
| 공유 상태 | [`MRGameState.cpp`](Source/MuscleRun/Sys/GameState/MRGameState.cpp) | 런타임 게임 상태 |

## 입력 정보

코드에서 사용하는 Enhanced Input 액션 이름은 다음과 같습니다.

- `IA_Left`
- `IA_Right`
- `IA_MTJump`
- `IA_Slide`
- `IA_Escape`

실제 키 바인딩은 공개되지 않은 `InputMappingContext` 에셋에 저장되어 있으므로, 저장소만으로 특정 키를 정확히 안내할 수 없습니다.

## 저장소 구조

```text
MuscleRun/
├─ Config/                         # 공개된 프로젝트 설정 일부
├─ Source/MuscleRun/
│  ├─ Character/                   # 플레이어 캐릭터
│  ├─ Data/                        # 공용 데이터 구조
│  ├─ Object/
│  │  ├─ System/                   # 타일 매니저
│  │  └─ Tile/                     # 타일·스폰 컴포넌트와 데이터
│  ├─ Private/                     # 게임 규칙과 세이브 구현
│  ├─ Public/                      # 외부에 노출되는 헤더
│  └─ Sys/                         # 프로젝트 시스템 코드
├─ docs/                           # 구현 해설
└─ MuscleRun.uproject
```

## 프로젝트에서 배운 점

- 무한 러너에서 생성 자체보다 **생성·전환·제거의 생명주기**를 일관되게 관리하는 것이 중요했습니다.
- 스폰 위치와 대상을 컴포넌트·Data Asset으로 분리하면 디자이너가 코드 변경 없이 반복 조정하기 쉬워집니다.
- 입력감은 기능 유무뿐 아니라 점프 버퍼처럼 플레이어 의도를 보존하는 작은 상태 처리에서 크게 달라집니다.
- 팀 프로젝트에서는 에셋 의존성과 공개 범위를 개발 초기부터 정리해야 재현 가능한 저장소를 만들 수 있다는 점을 체감했습니다.

## 한계와 개선 방향

- 공개 저장소에 실행에 필요한 Content가 없어 다른 환경에서 결과를 재현할 수 없습니다.
- 에셋 의존성 목록과 프로젝트 세팅을 개발 중에 별도로 기록하지 않아, 현재는 완전한 설치 가이드를 제공하기 어렵습니다.
- 다시 진행한다면 공개 가능한 최소 샘플 Map과 대체 에셋, 검증된 입력 설정을 별도 구성해 코드와 함께 배포하겠습니다.

## 문서와 링크

- [타일 및 스폰 루프 상세](docs/tile-and-spawn-loop.md)
- [Notion 프로젝트 페이지](https://app.notion.com/p/39cf36bcd9d981789b26d2d47fad3d26)
- [플레이 영상](https://youtu.be/cj1qKMBmgKA)
- [GitHub 저장소](https://github.com/furu23/MuscleRun)

## 팀

| 이름 | 담당 |
| --- | --- |
| 박동진 | 팀장, PM, 리드 디자이너, 게임플레이 시스템 |
| 김영채 | 아이템 개발 |
| 곽성찬 | 애니메이션 및 캐릭터 개발 |
| 김은숙 | 아트 |
