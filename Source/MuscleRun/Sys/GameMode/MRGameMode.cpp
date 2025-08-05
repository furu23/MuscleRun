#include "Sys/GameMode/MRGameMode.h"
#include "Sys/GameState/MRGameState.h"
#include "Public/Save/MRSaveManager.h"
#include "GameFramework/Character.h" // 예시를 위해 포함
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

AMRGameMode::AMRGameMode()
{
	// 사용할 기본 GameState 클래스를 지정합니다.
	GameStateClass = AMRGameState::StaticClass();
}

void AMRGameMode::BeginPlay()
{
	Super::BeginPlay();

	// === 🎵 배경음악 재생 ===
	if (BGM)
	{
		BGMComponent = UGameplayStatics::SpawnSound2D(this, BGM, 1.0f, 1.0f, 0.0f, nullptr, true);
		if (BGMComponent)
		{
			BGMComponent->Play();
		}
	}
	// 게임이 시작되면, StartGame 함수를 호출하여 게임 흐름을 시작합니다.
	StartGame();

	// TODO: 캐릭터의 OnDeath 델리게이트를 PlayerDied 함수에 바인딩하는 로직이 필요합니다.
	// ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	// if (PlayerCharacter) { PlayerCharacter->OnDeath.AddDynamic(this, &AMRGameMode::PlayerDied); }
}

void AMRGameMode::AddScore(int32 ScoreToAdd)
{
	// 1. '전광판'인 GameState를 가져옵니다.
	if (AMRGameState* GS = GetGameState<AMRGameState>())
	{
		// 2. GameState의 점수 변경 함수를 호출하여 값을 업데이트합니다.
		GS->AddScore(ScoreToAdd);
	}
}

void AMRGameMode::PlayerDied()
{
	UE_LOG(LogTemp, Log, TEXT("Player has died. Ending game."));
	EndGame();

}

void AMRGameMode::StartGame()
{
	if (AMRGameState* GS = GetGameState<AMRGameState>())
	{
		// 1. 게임 상태를 'InProgress'로 변경하라고 GameState에 명령합니다.
		GS->SetCurrentState(EMRGameState::InProgress);

		// 2. 난이도 상승 타이머를 시작합니다.
		GetWorld()->GetTimerManager().SetTimer(
			IncreaseDifficultyTimerHandle,
			this,
			&AMRGameMode::IncreaseDifficulty,
			TimeBetweenDifficultyIncrease,
			true // 루핑
		);
	}
}

void AMRGameMode::EndGame()
{
	if (AMRGameState* GS = GetGameState<AMRGameState>())
	{
		// 1. 게임 상태를 'GameOver'로 변경하라고 GameState에 명령합니다.
		GS->SetCurrentState(EMRGameState::GameOver);

		// 2. 더 이상 필요 없는 타이머들을 정지시킵니다.
		GetWorld()->GetTimerManager().ClearTimer(IncreaseDifficultyTimerHandle);

		// TODO: 게임 오버 UI를 띄우거나, 플레이어 입력을 막는 등의 로직을 처리합니다.
		
		UMRSaveManager* SaveManager = GetGameInstance()->GetSubsystem<UMRSaveManager>();
		if (SaveManager)
		{
			// 최고 점수 갱신 시에만 저장
			if (SaveManager->UpdateHighScore(GS->GetCurrentScore()))
			{
				SaveManager->SaveGame();
			}
		}
	}
}

void AMRGameMode::IncreaseDifficulty()
{
	if (AMRGameState* GS = GetGameState<AMRGameState>())
	{
		// 1. 현재 속도 배율을 GameState에서 읽어옵니다.
		const float CurrentMultiplier = GS->GetGameSpeedMultiplier();

		// 2. 새로운 속도 배율을 계산합니다.
		const float NewMultiplier = CurrentMultiplier + SpeedIncreaseAmount;

		// 3. GameState에 새로운 속도 배율을 설정하라고 명령합니다.
		GS->SetGameSpeedMultiplier(NewMultiplier);

		UE_LOG(LogTemp, Log, TEXT("Difficulty Increased! New speed multiplier: %f"), NewMultiplier);
	}
}