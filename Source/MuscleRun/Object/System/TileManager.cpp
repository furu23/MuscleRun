#include "Object/System/TileManager.h"
#include "Object/Tile/MRTile.h"
#include "Object/Tile/Component/SpawnLocationComponent.h"
#include "GameFramework/Character.h"
#include "Object/Tile/DataAsset/DA_SpawnableObjects.h"
#include "Kismet/GameplayStatics.h"
#include "Sys/GameState/MRGameState.h"
#include "Character/MRPlayerCharacter.h"

ATileManager::ATileManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATileManager::BeginPlay()
{
	Super::BeginPlay();

	// 기본적인 검사부터 수행합니다.
	PlayerCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("ATileManager: PlayerCharacter not found!"));
		SetActorTickEnabled(false);
		return;
	}

	// 시작 시 처음으로 타일을 생성할 Transform을 초기화해줍니다.
	NextSpawnPointTransform = GetActorTransform();

	// 설정값에 따라 미리 타일들을 스폰해둡니다.
	for (int i = 0; i < NumInitialTiles; ++i)
	{
		SpawnTile();
	}

	// 타일이 생성되었다면 타일에 대한 관리를 시작합니다.
	if (ActiveTileGroups.Num() > 0)
	{
		CurrentTrackingTile = ActiveTileGroups[0].Tile;
		CurrentTrackingTileIndex = 0;
	}

	// GameState에 자신의 레퍼런스를 제공해 다른 클래스들이 접근할 수 있도록 합니다. (불안정)
	if (AMRGameState* GS = GetWorld()->GetGameState<AMRGameState>())
	{
		GS->RegisterTileManager(this);
	}
}

void ATileManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 기본 검사
	if (!PlayerCharacter || !CurrentTrackingTile) return;

	// EndArrowLocation, 즉 타일의 끝의 Transform을 가져옵니다.
	FVector TileEndLocation = CurrentTrackingTile->GetEndArrowTransform().GetLocation();
	FVector PlayerLocation = PlayerCharacter->GetActorLocation();

	FTileGroup& CurrentGroup = ActiveTileGroups[CurrentTrackingTileIndex];

	// 추적해서 저장한 현재 Tracking 중인 타일의 ExitDirection을 통해, 내적을 계산할 방향 벡터를 가져옵니다.
	FVector TileEndDirection = [this, CurrentGroup]()->FVector {
		switch (CurrentGroup.ExitDirection)
		{
		case ETrackDirection::East:
			return FVector(0, 1, 0);
		case ETrackDirection::West:
			return FVector(0, -1, 0);
		case ETrackDirection::North:
			return FVector(1, 0, 0);
		case ETrackDirection::South:
			return FVector(-1, 0, 0);
		default:
			return FVector::ZeroVector;
		}
	}();

	// 내적을 통해 '타일의 통과 여부'를 검사합니다.
	if (FVector::DotProduct(PlayerLocation - TileEndLocation, TileEndDirection) > 0.f)
	{
		UE_LOG(LogTemp, Log, TEXT("DotProduct Result : %.2f <= %s,  %s"),
		FVector::DotProduct(PlayerLocation - TileEndLocation, TileEndDirection),
			*(PlayerLocation - TileEndLocation).ToString(),
			*TileEndDirection.ToString());
		CurrentTrackingTileIndex++;

		// TArray의 인덱스로 다음 추적 타일에 안전하게 접근합니다.
		if (ActiveTileGroups.IsValidIndex(CurrentTrackingTileIndex))
		{
			CurrentTrackingTile = ActiveTileGroups[CurrentTrackingTileIndex].Tile;
			SpawnTile();
			DestroyOldestTileGroup();
		}
	}

	// 이번에는 타일의 끝부분의 '오른쪽' 방향 벡터를 가져옵니다.
	FVector PlaneOrigin = CurrentGroup.Tile->GetEndArrowTransform().GetLocation();
	FVector PlaneNormal = CurrentGroup.Tile->GetEndArrowTransform().GetRotation().GetRightVector();
	FVector VectorToPlayer = PlayerLocation - PlaneOrigin;


	if (ensure(ActiveTileGroups.IsValidIndex(CurrentTrackingTileIndex)))
	{
		if (ensure(CurrentGroup.bIsTurnTrigger))
		{
			// 오른쪽 방향 벡터와 거리를 내적해서 '회전 여부'를 검사합니다.
			const float DotProduct = FVector::DotProduct(VectorToPlayer, PlaneNormal);
			//UE_LOG(LogTemp, Log, TEXT("DotProduct Result In Rotation Logic : %.2f"), DotProduct);
			if (DotProduct > 0.f)
			{
				AMRPlayerCharacter* PlayerCharacterRef = Cast<AMRPlayerCharacter>(PlayerCharacter);
				if (ensure(PlayerCharacterRef))
				{
					// Character를 회전시킵니다.
					PlayerCharacterRef->ExecuteForceTurn(CurrentTrackingTile->GetEndArrowTransform(), CurrentGroup.ExitDirection);
					CurrentGroup.bIsTurnTrigger = false;
				}
			}
		}

	}
}

void ATileManager::SpawnTile()
{
	if (TileClassMap.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ATileManager: TileClass is not set!"));
		return;
	}

	ETileType TypeToSpawn = ETileType::TurnLeft; // 추후 로직 구현 DecideNextTileType()
	TSubclassOf<AMRTile>* FoundClassPtr = TileClassMap.Find(TypeToSpawn);

	if (ensure(FoundClassPtr && *FoundClassPtr))
	{
		// 해당 DA 그룹에서 타일을 하나 가져옵니다.
		TSubclassOf<AMRTile> ClassToSpawn = *FoundClassPtr;
		AMRTile* NewTile = GetWorld()->SpawnActor<AMRTile>(ClassToSpawn, NextSpawnPointTransform);
		if (ensure(NewTile))
		{
			FTileGroup NewGroup;
			NewGroup.Tile = NewTile;

			// 타일이 직선이 아닐 경우, LastTileExitDirection 을 업데이트 시키며 FTileGroup의 값에 저장합니다. (위에서 사용)
			if (TypeToSpawn != ETileType::Straight)
			{
				int32 CastedType = static_cast<int32>(LastTileExitDirection);
				switch (TypeToSpawn)
				{
				case ETileType::TurnLeft:
					CastedType -= 1;
					break;
				case ETileType::TurnRight:
					CastedType += 1;
					break;
				default:
					ensureMsgf(false, TEXT("CalculateNextDirection received an unexpected TileType!"));
					break;
				}
				LastTileExitDirection = static_cast<ETrackDirection>((CastedType + 4) % 4);

				// 회전 타일이므로 트리거를 true로 합니다.
				NewGroup.bIsTurnTrigger = true;
			}
			NewGroup.ExitDirection = LastTileExitDirection;

			SpawnObjectsOnTile(NewTile, NewGroup.ContainedActors);

			// TArray의 맨 뒤에 새 그룹을 추가합니다.
			ActiveTileGroups.Add(NewGroup);

			NextSpawnPointTransform = NewTile->GetEndArrowTransform();
		}
	}
}

void ATileManager::DestroyOldestTileGroup()
{
	// TArray의 Num() 함수를 사용합니다.
	if (ActiveTileGroups.Num() > MinActiveTiles)
	{
		// TArray의 첫 번째 요소를 가져오고 제거합니다.
		FTileGroup OldestGroup = ActiveTileGroups[0];
		ActiveTileGroups.RemoveAt(0);

		// 인덱스를 제거했으므로, 현재 추적 인덱스도 하나 줄여서 배열 범위를 맞춥니다.
		CurrentTrackingTileIndex--;

		for (TObjectPtr<AActor> ActorToDestroy : OldestGroup.ContainedActors)
		{
			if (IsValid(ActorToDestroy))
			{
				ActorToDestroy->Destroy();
			}
		}

		if (IsValid(OldestGroup.Tile))
		{
			OldestGroup.Tile->Destroy();
		}
	}
}

// Spawn Location Component들을 가져오고 순회해서 그 값들을 통해 오브젝트들을 생성하는 과정입니다.
void ATileManager::SpawnObjectsOnTile(AMRTile* TargetTile, TArray<TObjectPtr<AActor>>& OutSpawnedActors)
{
	if (!TargetTile) return;

	TArray<USpawnLocationComponent*> SpawnPoints;
	TargetTile->GetComponents<USpawnLocationComponent>(SpawnPoints);

	for (USpawnLocationComponent* Point : SpawnPoints)
	{
		if (Point && Point->ActorGroupToSpawn && Point->ActorGroupToSpawn->ActorClasses.Num() > 0)
		{
			const TArray<TSubclassOf<AActor>>& SpawnableClasses = Point->ActorGroupToSpawn->ActorClasses;
			TSubclassOf<AActor> ClassToSpawn = SpawnableClasses[FMath::RandRange(0, SpawnableClasses.Num() - 1)];

			FTransform SpawnTransform = Point->GetComponentTransform();
			AActor* SpawnedObject = GetWorld()->SpawnActor<AActor>(ClassToSpawn, SpawnTransform);
			//UE_LOG(LogTemp, Log, TEXT("SpawnedComponentTransform : %s, %s"), *Point->GetComponentTransform().ToString(), *Point->GetFName().ToString());
			if (SpawnedObject)
			{
				OutSpawnedActors.Add(SpawnedObject);
			}
		}
	}
}
