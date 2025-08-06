// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MRSaveManager.generated.h"

class UMRSaveGame;
/**
 * 
 */
UCLASS()
class MUSCLERUN_API UMRSaveManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// 서브시스템이 처음 생성될 때 호출됨
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Save System")
	void SaveGame();

	UFUNCTION(BlueprintCallable, Category = "Save System")
	void LoadGame();

	UFUNCTION(BlueprintPure, Category = "Save System")
	int32 GetHighScore() const;

	UFUNCTION(BlueprintCallable, Category = "Save System")
	bool UpdateHighScore(int32 NewScore);

	float GetVolume() const;

	bool UpdateVolume(float NewVolume) const;


private:
	// 현재 메모리에 로드된 SaveGame 객체의 인스턴스
	UPROPERTY()
	TObjectPtr<UMRSaveGame> CurrentSaveGame;
	// 저장 파일의 이름과 인덱스. 상수로 관리하는 것이 좋다.
	const FString SaveSlotName = TEXT("DefaultSaveSlot");
	const int32 UserIndex = 0;
};
