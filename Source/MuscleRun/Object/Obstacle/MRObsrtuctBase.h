// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MRObsrtuctBase.generated.h"

UCLASS(Abstract)
class MUSCLERUN_API AMRObsrtuctBase : public AActor
{
	GENERATED_BODY()

public:
	AMRObsrtuctBase();

protected:
	virtual void BeginPlay() override;

	// ✨ 모든 장애물이 공통으로 사용할 충돌 처리 함수입니다.
	UFUNCTION()
	virtual void OnObstacleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	class USceneComponent* SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstruct")
	class UStaticMeshComponent* MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstruct")
	float Damage = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Obstruct")
	float KnockbackStrength = 700.0f;

	// 💥 부서질 때 재생할 파티클 이펙트입니다.
	UPROPERTY(EditAnywhere, Category = "Effects")
	class UParticleSystem* BreakEffectVFX;

	// 🔊 부서질 때 재생할 사운드 이펙트입니다.
	UPROPERTY(EditAnywhere, Category = "Effects")
	class USoundBase* BreakEffectSFX;
};