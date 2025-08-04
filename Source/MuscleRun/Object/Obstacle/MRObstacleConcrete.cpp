// Fill out your copyright notice in the Description page of Project Settings.

#include "MRObstacleConcrete.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"

AMRObstacleConcrete::AMRObstacleConcrete()
{
	PrimaryActorTick.bCanEverTick = true;

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = MeshComponent;
	ProjectileMovementComponent->InitialSpeed = 1000.0f;
	ProjectileMovementComponent->MaxSpeed = 1000.0f;
	ProjectileMovementComponent->bShouldBounce = true;
	ProjectileMovementComponent->Bounciness = 1.0f;
	ProjectileMovementComponent->ProjectileGravityScale = 0.f;
	ProjectileMovementComponent->bRotationFollowsVelocity = false;

	// 움직이는 장애물 하늘로 솓구치는 버그수정 로직
	// 1. 움직임을 특정 평면에 구속하는 기능을 활성화합니다.
	ProjectileMovementComponent->bConstrainToPlane = true;

	// 2. 그 평면을 Z축을 기준으로 하는 XY 평면으로 설정합니다.
	ProjectileMovementComponent->SetPlaneConstraintNormal(FVector(0.f, 0.f, 1.f));
}

void AMRObstacleConcrete::BeginPlay()
{
	Super::BeginPlay();
	StartLocation = GetActorLocation();
	ProjectileMovementComponent->Velocity = FVector(0.0f, 1.0f, 0.0f) * ProjectileMovementComponent->InitialSpeed;
}

void AMRObstacleConcrete::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const float DistanceMoved = FVector::Dist(StartLocation, GetActorLocation());
	if (MoveDistance > 0.f && DistanceMoved > MoveDistance)
	{
		ProjectileMovementComponent->Velocity *= -1;
		StartLocation = GetActorLocation();
	}
}

void AMRObstacleConcrete::OnObstacleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// ✨ 1. 부모 클래스의 OnObstacleHit 기능을 먼저 실행합니다 (플레이어에게 데미지/넉백).
	Super::OnObstacleHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);

	// ✨ 2. 부딪힌 대상이 플레이어가 아닐 경우에만 이 장애물 고유의 로직을 실행합니다.
	ACharacter* PlayerCharacter = Cast<ACharacter>(OtherActor);
	if (PlayerCharacter == nullptr)
	{
		// 대상이 플레이어가 아니면 (예: 벽), 왕복 운동을 위해 시작 위치를 리셋합니다.
		StartLocation = GetActorLocation();
	}
}