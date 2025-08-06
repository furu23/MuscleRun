// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MRHealthComponent.generated.h"

/*
* MRPlayerCharacter의 체력을 관리하는 컴포넌트입니다.
* 필수적으로 구현해야 할 것들은 다음과 같습니다.
* 1. 체력
* 2. 체력을 회복하는 로직, 체력이 감소되는 로직
* 3. 체력이 낮아졌을 때, 체력이 0이 되었을 때 델리게이트를 통해 BroadCast합니다.
* 변경 및 확장의 필요성을 느끼신다면 고민없이 그렇게 해주세요! 다만 이야기해주세요!
* UPROPERTY, UFUNCTION과 리플렉션 인자들은 생각해서 넣어주세요.
*/
// 체력이 줄어들었을 때의 상태를 알리기 위한 델리게이트 선언입니다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, HealthAmount);

// 체력이 0이 되었을 때의 상태를 알리기 위한 델리게이트 선언입니다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthBecomeToZero);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MUSCLERUN_API UMRHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMRHealthComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// 체력조작
	void GetDamage(float Amount); // 주로 Character에게서 호출되는 체력이 감소되는 함수입니다.

	void Heal(float Amount); // 체력을 회복하는 함수입니다.

	float RetHealth() { return CurrentHealth; }

	float RetMaxHealth() { return MaxHealth; }

	/*
	* 여기 델리게이트 인자를 선언해주세요!
	*/
	UPROPERTY(BlueprintAssignable)			//블루 프린트에서 체력변화, 0이벤트에 바인딩 할 수있게 선언된 Delegate변수
	FOnHealthChanged OnHealthChanged;			

	UPROPERTY(BlueprintAssignable)
	FOnHealthBecomeToZero OnHealthBecomeToZero;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Health", meta = (AllowPrivateAccess = "true")) //최대 체력 값 블루 프린트에서 읽기만 가능하고 수정은 에디터에서 가능
		float MaxHealth = 100.0f;

	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true")) // 현재 체력 값 /어디서든 볼 수 있지만 직접 수정은 안됨
		float CurrentHealth = -1.0f;
};
