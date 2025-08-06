#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/MRDataType.h"
#include "TileManager.generated.h"

// TArray<TSubclassOf<AMRTile>>를 담기 위한 구조체
USTRUCT(BlueprintType)
struct FTileClassArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<AMRTile>> TileClasses;
};

class AMRTile;
class USpawnLocationComponent; // [수정] 이름 변경 반영

/**
 * @class ATileManager
 * @brief 무한 러너 게임의 타일 생성, 파괴 및 흐름을 총괄하는 중앙 관리자 클래스입니다.
 */
UCLASS()
class ATileManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ATileManager();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	FORCEINLINE int32 GetTileGroupNum() { return ActiveTileGroups.Num(); }

private:
	void SpawnTile();
	void DestroyOldestTileGroup();
	void SpawnObjectsOnTile(AMRTile* TargetTile, TArray<TObjectPtr<AActor>>& OutSpawnedActors, ETrackDirection TileDirection);
	ETileType DecideNextTileGroup();

	int32 NumOfPreviousStraightTile = 0;

	FTimerHandle DestroyTimeHandler;

	// --- 설정 변수들 ---

	UPROPERTY(EditAnywhere, Category = "Tile Management|Config", meta = (ClampMin = "1"))
	int32 NumInitialTiles = 10;

	UPROPERTY(EditAnywhere, Category = "Tile Management|Config", meta = (ClampMin = "1"))
	int32 MinActiveTiles = 8;

	// --- 타일 관련 ---
    // 디자이너가 블루프린트 에디터에서 지정할 타일 클래스들
	UPROPERTY(EditAnywhere, Category = "Tile Management|Tile Types")
    TMap<ETileType, FTileClassArray> TileClassMap;


	// --- 상태 변수들 ---

	UPROPERTY()
	TObjectPtr<ACharacter> PlayerCharacter;

	TArray<FTileGroup> ActiveTileGroups;

	UPROPERTY()
	FTransform NextSpawnPointTransform;

	UPROPERTY()
	TObjectPtr<AMRTile> CurrentTrackingTile;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 CurrentTrackingTileIndex = 0;

	UPROPERTY()
	ETrackDirection LastTileExitDirection = ETrackDirection::North;
};