// Fill out your copyright notice in the Description page of Project Settings.


#include "Save/MRSaveManager.h"
#include "MRSaveGame.h"
#include <Kismet/GameplayStatics.h>

void UMRSaveManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// 게임이 시작되면, 즉시 저장 파일을 로드해서 메모리에 올려둔다.             
	LoadGame();
}

void UMRSaveManager::SaveGame()
{
	if (CurrentSaveGame)
	{
		// 현재 메모리에 있는 CurrentSaveGame 객체를 파일로 '저장'한다.                           
		// 비동기(Async) 버전을 사용하면 저장 중 랙을 방지할 수 있지만, 지금은 동기 버전으로도 충분하다.                           
		UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SaveSlotName, UserIndex);
	}
}

void UMRSaveManager::LoadGame()
{
	// 1. 지정된 슬롯에 저장 파일이 '존재하는지' 확인한다.             
	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, UserIndex))
	{
		// 2. 존재한다면, 파일에서 데이터를 읽어와 UMRSaveGame 객체로 '로드'한다.                           
		//    LoadGameFromSlot은 USaveGame 포인터를 반환하므로, 우리 클래스로 캐스팅해야 한다.                           
		CurrentSaveGame = Cast<UMRSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex));
	}
	else
	{
		// 3. 존재하지 않는다면, 새로운 UMRSaveGame 객체를 '생성'한다.                           
		//    이 객체는 생성자에 정의된 기본값을 가진다 (HighScore = 0 등).                           
		CurrentSaveGame = Cast<UMRSaveGame>(UGameplayStatics::CreateSaveGameObject(UMRSaveGame::StaticClass()));
	}
}

int32 UMRSaveManager::GetHighScore() const
{
	//현재 세이브 데이터(CurrentSaveGame)가 유효한지 확인한 뒤, 유효하면 HighScore를 반환하고, 아니면 기본값 0점을 반환합니다.
	return CurrentSaveGame ? CurrentSaveGame->HighScore : 0;
}

bool UMRSaveManager::UpdateHighScore(int32 NewScore)
{
	if (CurrentSaveGame && NewScore > CurrentSaveGame->HighScore)
	{
		//현재 세이브 데이터가 있고, 새로운 점수가 기존 최고 점수보다 높을 경우 → 최고 점수 갱신!
		CurrentSaveGame->HighScore = NewScore;
		//그리고 갱신했다면 true
		return true;
	}
	// 안 했다면 false 반환
	return false;
}

float UMRSaveManager::GetVolume() const
{
	return CurrentSaveGame ? CurrentSaveGame->MasterVolume : 0;
}

bool UMRSaveManager::UpdateVolume(float NewVolume) const
{
	if (CurrentSaveGame)
	{
		CurrentSaveGame->MasterVolume = NewVolume;
		return true;
	}
	return false;
}
