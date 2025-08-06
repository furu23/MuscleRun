#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MRGameMode.generated.h"

UCLASS()
class MUSCLERUN_API AMRGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMRGameMode();

	// --- Public API (Game World에서 호출) ---
	// 다른 액터(캐릭터, 아이템 등)가 게임 규칙에 영향을 주기 위해 호출하는 함수들입니다.

	/**
	 * @brief 점수를 추가합니다. 코인이나 아이템이 이 함수를 호출합니다.
	 * @param ScoreToAdd 추가할 점수
	 */
	UFUNCTION(BlueprintCallable, Category = "GameMode")
	void AddScore(int32 ScoreToAdd);

	/**
	 * @brief 플레이어가 사망했을 때 호출될 함수입니다. 캐릭터의 OnDeath 델리게이트에 바인딩됩니다.
	 */
	void PlayerDied();

protected:
	virtual void BeginPlay() override;

private:
	/**
	 * @brief 게임을 시작 상태로 전환합니다. (예: 카운트다운 후 InProgress로)
	 */
	void StartGame();

	/**
	 * @brief 게임을 종료 상태로 전환합니다.
	 */
	void EndGame();

	/**
	 * @brief 시간에 따라 게임의 난이도(속도)를 점진적으로 올립니다.
	 */
	void IncreaseDifficulty();

	// --- 설정 변수들 ---

	// 난이도가 상승하기까지의 시간 간격 (초)
	UPROPERTY(EditDefaultsOnly, Category = "GameMode|Difficulty")
	float TimeBetweenDifficultyIncrease = 5.0f;

	// 한 번에 증가할 속도 배율
	UPROPERTY(EditDefaultsOnly, Category = "GameMode|Difficulty")
	float SpeedIncreaseAmount = 0.1f;

	// --- 상태 변수들 ---

	// 난이도 상승 타이머를 위한 핸들
	FTimerHandle IncreaseDifficultyTimerHandle;

	// === 🎵 배경음악 관련 ===

// 에디터에서 설정할 BGM 사운드
	UPROPERTY(EditAnywhere, Category = "GameMode|Audio")
	USoundBase* BGM;

	// 재생 제어를 위한 오디오 컴포넌트
	UPROPERTY()
	class UAudioComponent* BGMComponent;
};
