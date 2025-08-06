#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Delegates/DelegateCombinations.h"
#include "Object/System/TileManager.h"
#include "Data/MRDataType.h"
#include "MRGameState.generated.h"


// 델리게이트 선언

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameScoreChanged, int32, Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGameStateChanged, EMRGameState, state);


UCLASS()
class MUSCLERUN_API AMRGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AMRGameState();

	void RegisterTileManager(ATileManager* Manager);

	// --- Public API (Getters) ---
	// 다른 액터(특히 UI)들이 현재 게임 상태 정보를 읽어갈 수 있는 함수들입니다.

	UFUNCTION(BlueprintPure, Category = "GameState")
	int32 GetCurrentScore() const { return CurrentScore; }

	UFUNCTION(BlueprintPure, Category = "GameState")
	int32 GetHighScore() const { return HighScore; }

	UFUNCTION(BlueprintPure, Category = "GameState")
	float GetGameSpeedMultiplier() const { return GameSpeedMultiplier; }

	UFUNCTION(BlueprintPure, Category = "GameState")
	EMRGameState GetCurrentState() const { return CurrentState; }

	// --- 상태 변경 함수들 (GameMode만 호출해야 함) ---
	// 이 함수들은 오직 AMRGameMode만이 호출하여 게임의 상태를 변경해야 합니다.

	void AddScore(int32 Amount);
	void SetGameSpeedMultiplier(float NewMultiplier);
	void SetCurrentState(EMRGameState NewState);

	void SaveHighScore();
	void LoadHighScore();

	void SaveVolumes();
	void LoadVolumes();

	// --- 델리게이트 변수들 (상태 및 점수 방송용) ---
	
	FOnGameScoreChanged OnScoreChanged;
	FOnGameStateChanged OnStateChanged;

protected:
	/**
	 * @brief 현재 월드에 존재하는 TileManager의 유일한 참조.
	 * 다른 모든 액터는 이 변수를 통해 TileManager에 접근합니다.
	 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Managers")
	TObjectPtr<class ATileManager> TileManagerRef;

private:
	// --- 상태 변수들 ---
	// 이 변수들의 실제 '값'이 모든 플레이어에게 공유됩니다.

	// 현재 점수
	UPROPERTY(VisibleAnywhere, Category = "GameState")
	int32 CurrentScore;

	UPROPERTY(VisibleAnywhere, Category = "GameState")
	int32 HighScore;

	// 현재 게임 속도 배율
	UPROPERTY(VisibleAnywhere, Category = "GameState")
	float GameSpeedMultiplier;

	// 현재 게임의 상태 (시작 대기, 진행 중, 게임 오버 등)
	UPROPERTY(VisibleAnywhere, Category = "GameState")
	EMRGameState CurrentState;
};