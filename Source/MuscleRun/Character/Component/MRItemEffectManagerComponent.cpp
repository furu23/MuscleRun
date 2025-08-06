// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Component/MRItemEffectManagerComponent.h"
#include "Character/MRPlayerCharacter.h"
#include "GameFramework/Character.h"
#include "GameFrameWork/CharacterMovementComponent.h"
#include <Kismet/GameplayStatics.h>

// Sets default values for this component's properties
UMRItemEffectManagerComponent::UMRItemEffectManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UMRItemEffectManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

// 각각의 효과를 구현하는 함수입니다.
void UMRItemEffectManagerComponent::ApplyEffect(EItemEffectTypes ItemTypes)
{
	UE_LOG(LogTemp, Error, TEXT("%s: %d"), *GetOwner()->GetFName().ToString(), static_cast<int32>(ItemTypes))
	switch (ItemTypes)
	{
		case EItemEffectTypes::Score:
		{
			TempScore += 10;
			UE_LOG(LogTemp, Log, TEXT("Score +10! 현재 점수: %d"), TempScore);

			TObjectPtr<AMRGameState> CachedGameState = Cast<AMRGameState>(UGameplayStatics::GetGameState(this));
			CachedGameState->AddScore(10);
			break;
		}
		
		case EItemEffectTypes::Faster:
		{
			ACharacter* PlayerCharacter = Cast<ACharacter>(GetOwner());
			if (ensure(PlayerCharacter))
			{
				AMRPlayerCharacter* MRChar = Cast<AMRPlayerCharacter>(PlayerCharacter);
				if (ensure(MRChar))
				{
					const float BonusSpeed = 300.f;
					MRChar->SpeedBonus += BonusSpeed;

					UE_LOG(LogTemp, Log, TEXT("속도 증가! Bonus: %.1f"), MRChar->SpeedBonus);

					// 일정 시간 후 다시 감소
					
					FTimerDelegate TimerDel = FTimerDelegate::CreateLambda([MRChar, BonusSpeed]()
						{
							MRChar->SpeedBonus -= BonusSpeed;
							MRChar->SpeedBonus = FMath::Max(0.f, MRChar->SpeedBonus); // 음수 방지
							UE_LOG(LogTemp, Log, TEXT("속도 복원됨! 현재 Bonus: %.1f"), MRChar->SpeedBonus);
						});

					MRChar->GetWorldTimerManager().SetTimer(TimerHandle, TimerDel, 5.f, false);
				}
			}
			break;
		}
	
		case  EItemEffectTypes::NoDie:
		{
			ACharacter* PlayerCharacter = Cast<ACharacter>(GetOwner());
			if (PlayerCharacter)
			{
				AMRPlayerCharacter* MRCharacter = Cast<AMRPlayerCharacter>(PlayerCharacter);
				if (MRCharacter)
				{
					MRCharacter->bIsInvincible = true;

					//(선택) 일정 시간 후 다시 죽을 수 있도록 타이머 설정
					
					MRCharacter->GetWorldTimerManager().SetTimer(
					TimerHandle, [MRCharacter]() 
					{MRCharacter->bIsInvincible = false; }, 
					5.0f, // 불사 지속 시간 (초)
					false);
				}
			}
			break;
		}
		default:
		break;
	}	
}