#include "MRGameState.h"
#include "MRSaveGame.h"
#include <Kismet/GameplayStatics.h>

AMRGameState::AMRGameState()
{
	// 변수들의 초기값을 설정합니다.
	CurrentScore = 0;
	GameSpeedMultiplier = 1.0f;
	CurrentState = EMRGameState::WaitingToStart;

	LoadHighScore(); // 게임 시작 시 최고 점수 불러오기
}

void AMRGameState::RegisterTileManager(ATileManager* Manager)
{
	TileManagerRef = Manager;
}

void AMRGameState::AddScore(int32 Amount)
{
	CurrentScore += Amount;

	if (CurrentScore > HighScore)
	{
		HighScore = CurrentScore;
		SaveHighScore(); // 최고점 갱신 시 저장
	}

	OnScoreChanged.Broadcast(CurrentScore);

	// TODO: UI가 이 델리게이트에 바인딩해서 점수를 업데이트하는 로직을 작성합니다.
}

void AMRGameState::SetGameSpeedMultiplier(float NewMultiplier)
{
	GameSpeedMultiplier = NewMultiplier;
}

void AMRGameState::SetCurrentState(EMRGameState NewState)
{
	if (CurrentState != NewState)
	{
		CurrentState = NewState;
		OnStateChanged.Broadcast(CurrentState);
	}
}

void AMRGameState::SaveHighScore()
{
	UMRSaveGame* SaveGameInstance = Cast<UMRSaveGame>(UGameplayStatics::CreateSaveGameObject(UMRSaveGame::StaticClass()));
	if (SaveGameInstance)
	{
		SaveGameInstance->HighScore = CurrentScore;
		UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("DefaultSlot"), 0);
	}
}

void AMRGameState::LoadHighScore()
{
	UMRSaveGame* Loaded = Cast<UMRSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("DefaultSlot"), 0));
	if (Loaded)
	{
		HighScore = Loaded->HighScore;
	}
}

void AMRGameState::SaveVolumes()
{
	UMRSaveGame* Loaded = Cast<UMRSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("DefaultSlot"), 0));
	if (Loaded)
	{
		;
	}

}

void AMRGameState::LoadVolumes()
{

}
