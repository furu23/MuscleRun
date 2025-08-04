// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/Item/ItemBaseActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Character/MRPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

class USceneComponent;

// Sets default values
AItemBaseActor::AItemBaseActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));
	SetRootComponent(RootScene);
  
	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComp);

	MeshComp->SetMobility(EComponentMobility::Movable);

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(RootComponent);
	// TriggerVolume->SetBoxExtent()

	// 컴포넌트 연결!
}

// Called when the game starts or when spawned
void AItemBaseActor::BeginPlay()
{
	Super::BeginPlay();

	// 여기서 TriggerVolume의 오버래핑 이벤트를 바인딩!
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AItemBaseActor::OnOverlapBegin);
}

void AItemBaseActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 회전 속도 (도/초 단위 -> 라디안/초)
	float RotationSpeed = 120.0f; // 초당 30도

	// 회전 축 설정 ( 기울어진 축 예시: x=1, y=1, z=0)
	FVector TiltedAxis(0.f, 0.f, 1.f);
	TiltedAxis.Normalize(); // 정규화

	// 쿼티리언으로 회전 계산
	FQuat RotationQuat = FQuat(TiltedAxis, FMath::DegreesToRadians(RotationSpeed * DeltaTime));

	// 현재 회전에 회전 덧붙이기
	MeshComp->AddLocalRotation(RotationQuat);
}

void AItemBaseActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 여기서 OtherActor(AMRPlayerCharacter 의 ItemActivate 함수를 통해, 호출해주세요!
	// 끝냈다면, 에디터 세팅에서 Player의 Collision Preset에서 새 Coliision Channel, Item을 만들고, Ovelap 만 활성화!
	// 이 액터의 콜리전 채널을 Item으로 하고, Player의 것도 켜 주세요!
	AMRPlayerCharacter* PlayerCharacter = Cast<AMRPlayerCharacter>(OtherActor);
	if (PlayerCharacter && ItemType != EItemEffectTypes::None)
	{
		PlayPickupEffect(); // 줍는 사운드/이펙트 재생
		/*UE_LOG(LogTemp, Warning, TEXT("충돌: %s(%s) 와 %s(%s)"),
			*GetName(), *OverlappedComp->GetName(),
			*OtherActor->GetName(), *OtherComp->GetName());*/
		PlayerCharacter->ItemActivated(ItemType); // 아이템 효과 발동 (플레이어에게 전달)

		SetActorHiddenInGame(true);   // 시각적으로 숨김
		SetActorEnableCollision(false); // 충돌 비활성화

		// 0.8초 뒤에 DestroySelf() 호출
		FTimerHandle DestroyTimerHandle;
		GetWorldTimerManager().SetTimer(DestroyTimerHandle, this, &AItemBaseActor::DestroySelf, 0.8f, false);
	}
}

void AItemBaseActor::DestroySelf()
{
	PlayVanishEffect();

	Destroy();
}

void AItemBaseActor::PlayPickupEffect()
{
	if (PickupSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PickupSound, GetActorLocation());
	}
	
	if (PickupEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), PickupEffect, GetActorLocation());
	}
}

void AItemBaseActor::PlayVanishEffect()
{
	if (VanishSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, VanishSound, GetActorLocation());
	}

	if (VanishEffect)
	{
		UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(
			this, VanishEffect, GetActorLocation());
		if (PSC)
		{
			PSC->bAutoDestroy = true;  // 파티클 끝나면 자동으로 씬에서 제거됨
		}
	}
}

