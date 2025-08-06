// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Component/MRItemEffectManagerComponent.h"
#include "GameFramework/Actor.h"
#include "ItemBaseActor.generated.h"

/*
* 아이템의 기본 뼈대가 될 클래스입니다.
* 아이템의 효과를 제외한 모든 기본 속성들을 전부 포함합니다.
* 구성을 간략화해서 다음과 같은 기능만을 포함합니다!
* 1. 컴포넌트들
* 2. 오버래핑 바인딩 함수.
*/
UCLASS()
class MUSCLERUN_API AItemBaseActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AItemBaseActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Item")
	class UBoxComponent* TriggerVolume;

	UPROPERTY(EditAnywhere, Category = "Item")
	class UStaticMeshComponent* MeshComp;

	UPROPERTY(EditAnywhere, Category = "Effect")
	class UParticleSystemComponent* PickupEffectComp;

	// 아이템 타입을 에디터에 넣어주세요.
	UPROPERTY(EditAnywhere, Category = "Item")
	EItemEffectTypes ItemType;

	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* PickupSound;

	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* PickupSoundCue; // PickupSound를 랜덤하게 가능하게함. (선택사항)

	UPROPERTY(EditAnywhere, Category = "Sound")
	USoundBase* VanishSound;

	UPROPERTY(EditAnywhere, Category = "Effect")
	UParticleSystem* PickupEffect;

	UPROPERTY(EditAnywhere, Category = "Effect")
	UParticleSystem* VanishEffect;

	FTimerHandle TimerHandle;


protected:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void DestroySelf();  // 자기 자신(this)을 파괴한다.
	void PlayPickupEffect();
	void EffectComponentDestroy();
	void PlayVanishEffect();
};
