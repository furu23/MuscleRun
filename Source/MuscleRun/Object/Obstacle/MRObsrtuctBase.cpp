// Fill out your copyright notice in the Description page of Project Settings.

#include "MRObsrtuctBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
// UParticleSystem과 USoundBase를 사용하기 위해 헤더를 추가할 수 있습니다.
#include "Particles/ParticleSystem.h"
#include "Sound/SoundBase.h"


AMRObsrtuctBase::AMRObsrtuctBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);

	// ✨ 모든 장애물이 기본적으로 플레이어와 부딪히도록(Block) 설정합니다.
	MeshComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	MeshComponent->SetNotifyRigidBodyCollision(true);
}

void AMRObsrtuctBase::BeginPlay()
{
	Super::BeginPlay();
	// ✨ OnObstacleHit 함수를 이벤트에 연결합니다.
	MeshComponent->OnComponentHit.AddDynamic(this, &AMRObsrtuctBase::OnObstacleHit);
}

void AMRObsrtuctBase::OnObstacleHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	// ✨ 부딪힌 대상이 플레이어 캐릭터라면, 데미지와 넉백을 적용합니다.
	ACharacter* MRPlayerCharacter = Cast<ACharacter>(OtherActor);
	if (MRPlayerCharacter)
	{
		// [기존 기능 유지] 데미지를 적용합니다.
		UGameplayStatics::ApplyDamage(MRPlayerCharacter, Damage, UGameplayStatics::GetPlayerController(this, 0), this, UDamageType::StaticClass());

		// --- 변경된 부분 시작: 넉백 로직 수정 ---

		// 1. 넉백의 세기를 설정합니다. 이 값들을 조절하여 넉백 느낌을 바꿀 수 있습니다.
		const float KnockbackXYStrength = 1200.0f; // 뒤로 밀려나는 힘
		const float KnockbackZStrength = 600.0f;  // 위로 살짝 뜨는 힘

		// 2. '충돌 방향(Hit.ImpactNormal)' 대신 '캐릭터의 앞 방향'을 기준으로 뒤쪽 방향을 계산합니다.
		const FVector BackwardDirection = -MRPlayerCharacter->GetActorForwardVector();

		// 3. 최종 넉백 속도를 계산합니다. (뒤로 밀려나면서 살짝 뜨도록)
		const FVector LaunchVelocity = (BackwardDirection * KnockbackXYStrength) + (FVector::UpVector * KnockbackZStrength);

		// 4. 계산된 속도로 캐릭터를 튕겨냅니다.
		MRPlayerCharacter->LaunchCharacter(LaunchVelocity, true, true);

		// --- 변경된 부분 끝 ---

		// [기존 기능 유지] 부서지는 이펙트(VFX)를 재생합니다. (변수에 이펙트가 할당된 경우에만)
		if (BreakEffectVFX)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BreakEffectVFX, GetActorLocation(), GetActorRotation());
		}

		// [기존 기능 유지] 부서지는 사운드(SFX)를 재생합니다. (변수에 사운드가 할당된 경우에만)
		if (BreakEffectSFX)
		{
			UGameplayStatics::PlaySoundAtLocation(this, BreakEffectSFX, GetActorLocation());
		}

		// [기존 기능 유지] 모든 효과를 재생한 뒤, 장애물 액터 자신을 파괴하여 맵에서 제거합니다.
		Destroy();
	}
}