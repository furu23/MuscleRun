// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/MRPlayerCharacter.h"
#include "Blueprint/UserWidget.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFrameWork/SpringArmComponent.h"
#include "GameFrameWork/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimInstance.h"
#include "Component/MRItemEffectManagerComponent.h"
#include "Object/Item/ItemBaseActor.h"
#include "Component/MRHealthComponent.h"
#include <Sys/GameState/MRGameState.h>
#include "Kismet/GameplayStatics.h"
#include "Sys/WidgetSubSystem/MRUIManager.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

// Sets default values
AMRPlayerCharacter::AMRPlayerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// 기본 상태를 '달리기'로 초기화합니다.
	CharacterState = ECharacterState::ECS_Running;

	// --- 컴포넌트 계층 구조 설정 ---

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	TriggerVolume->SetupAttachment(RootComponent);
	HealthComp = CreateDefaultSubobject<UMRHealthComponent>(TEXT("HealthComp"));
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

	// --- Bool 프로퍼티 설정 ---
	
	// 캐릭터는 컨트롤러의 회전을 따르지 않는다 (이동 방향을 바라봐야 하므로).
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	// 스프링 암도 컨트롤러의 회전을 따르지 않고, 캐릭터의 등 뒤에 고정되어야 한다.
	SpringArm->bUsePawnControlRotation = false;
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritRoll = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bDoCollisionTest = false; // 카메라가 벽에 부딪혀 줌인되는 것을 방지

	// 카메라는 스프링 암에 붙어있으므로, 당연히 컨트롤러 회전을 따르지 않는다.
	Camera->bUsePawnControlRotation = false;


	// --- CMC 기본 인자 설정 ---
	GetCharacterMovement()->bOrientRotationToMovement = true; // 이동 방향으로 캐릭터를 회전시킨다.
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = BASE_SPEED_MAX;
	GetCharacterMovement()->JumpZVelocity = BASE_JUMP_VELOCITY;
	GetCharacterMovement()->AirControl = 1.0f;
	GetCharacterMovement()->GravityScale = BASE_GRAVITY_SCALE;

	// --- 추가 설정 및 초기화 ---
	EffectComponent = CreateDefaultSubobject<UMRItemEffectManagerComponent>(TEXT("EffectComp"));
}

// Called when the game starts or when spawned
void AMRPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// HealthComponent가 유효한지 확인하고, OnHealthBecomeToZero 이벤트가 발생하면
	// 이 클래스의 OnDeath 함수를 호출하도록 서로 연결(바인딩)합니다.
	if (HealthComp)
	{
		HealthComp->OnHealthBecomeToZero.AddDynamic(this, &AMRPlayerCharacter::OnDeath);
	}

	// 이 액터(플레이어)가 데미지를 입는 이벤트(OnTakeAnyDamage)가 발생하면,
	// HandleTakeDamage 함수를 호출하도록 연결(바인딩)합니다.
	OnTakeAnyDamage.AddDynamic(this, &AMRPlayerCharacter::HandleTakeDamage);

	// 기본 입력 초기화를 진행합니다.
	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(IMC_MRPlayerInput, 0);
		}
	}

	// GameState를 가져오고 디버그 위젯을 켭니다.
	CachedGameState = Cast<AMRGameState>(UGameplayStatics::GetGameState(this));
	GetWorld()->GetSubsystem<UMRUIManager>()->ToggleDebugWidget();

	// 캡슐 높이 저장
	DefualtCapsuleHalfSize = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	// 타일 매니저와의 종속관계로 인해, Tick()을 항상 타일 매니저 뒤로 실행합니다.
	if (ATileManager* TileManager = Cast<ATileManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATileManager::StaticClass())))
	{
		// "나(캐릭터)의 Tick은, 반드시 TileManager의 Tick이 끝난 후에 실행되어야 한다"
		// 라고 엔진에게 명시적으로 알려줍니다.
		AddTickPrerequisiteActor(TileManager);
	}
}

void AMRPlayerCharacter::Slide()
{
	// 땅에 있을 때만 슬라이딩 가능
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		CharacterState = ECharacterState::ECS_Sliding;
		// 여기에 실제 슬라이딩 로직 추가 (예: 콜리전 크기 변경, 속도 변경 등)
		// 디버그 메시지 추가
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("SLIDING STATE"));
	}
}

void AMRPlayerCharacter::StopSliding()
{
	// 현재 슬라이딩 상태일 때만 달리기 상태로 변경
	if (CharacterState == ECharacterState::ECS_Sliding)
	{
		CharacterState = ECharacterState::ECS_Running;
		// 슬라이딩 종료 로직 추가 (예: 콜리전 크기 복구 등)
	}
}

void AMRPlayerCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	// 이동 모드가 'Falling'(낙하 중)으로 바뀌었다면 점프 상태로 간주
	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		CharacterState = ECharacterState::ECS_Jumping;
		// 디버그 메시지 추가
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("JUMPING STATE"));
	}
	// 다시 땅으로 돌아오면(Walking) 달리기 상태로 변경
	else if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Walking)
	{
		CharacterState = ECharacterState::ECS_Running;
	}
}

void AMRPlayerCharacter::HandleTakeDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	// GetDamaged 함수를 호출하여 HealthComponent에 데미지를 전달합니다.
	GetDamaged(Damage);
}

void AMRPlayerCharacter::OnDeath()
{
	// 1. 죽음 이펙트(VFX)를 재생합니다.
	if (DeathEffectVFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeathEffectVFX, GetActorLocation(), GetActorRotation());
	}

	// 2. 죽음 사운드(SFX)를 재생합니다.
	if (DeathEffectSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DeathEffectSFX, GetActorLocation());
	}

	// 3. 플레이어의 입력을 비활성화하여 더 이상 움직이지 못하게 합니다.
	DisableInput(nullptr);

	// 3. 캐릭터의 기본 충돌체(캡슐)의 기능을 끕니다.
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 4. 캐릭터의 이동 컴포넌트를 비활성화합니다.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	// 5. 스켈레탈 메시에 물리 시뮬레이션을 적용하여 래그돌로 만듭니다.
	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->SetCollisionProfileName(TEXT("Ragdoll")); // 래그돌용 콜리전 프로파일로 변경
		SkelMesh->SetSimulatePhysics(true); // 물리 시뮬레이션 활성화!
	}

	// 6. 카메라를 분리하여 래그돌이 된 캐릭터를 계속 비추게 합니다.
	if (SpringArm)
	{
		SpringArm->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}

	if (ResultWidgetClass)
	{
		// 플레이어 컨트롤러를 가져옵니다.
		APlayerController* PC = GetController<APlayerController>();
		if (PC)
		{
			// 위젯을 생성합니다.
			UUserWidget* ResultWidget = CreateWidget<UUserWidget>(PC, ResultWidgetClass);
			if (ResultWidget)
			{
				// 화면에 위젯을 추가합니다.
				ResultWidget->AddToViewport();

				// 마우스 커서를 보이게 하고, UI에만 입력이 가능하도록 모드를 변경합니다.
				PC->bShowMouseCursor = true;
				PC->SetInputMode(FInputModeUIOnly());
			}
		}
	}

	// 6. 약간의 딜레이 후 캐릭터 액터를 파괴합니다. (이펙트가 보일 시간 확보)
	SetLifeSpan(200.0f);
}

// Called every frame
void AMRPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 시간에 따른 난이도 곡선을 적용합니다.
	if (CachedGameState)
	{
		double NewMultiplier = CachedGameState->GetGameSpeedMultiplier();

		//if (b~~~)
		GetCharacterMovement()->MaxWalkSpeed = (BASE_SPEED_MAX + SpeedBonus) * NewMultiplier;

		//GetCharacterMovement()->MaxWalkSpeed = BASE_SPEED_MAX * NewMultiplier + 300.f;

		GetCharacterMovement()->GravityScale = BASE_GRAVITY_SCALE * NewMultiplier * NewMultiplier;
		GetCharacterMovement()->JumpZVelocity = BASE_JUMP_VELOCITY * NewMultiplier;
		GetCharacterMovement()->MaxAcceleration = BASE_MAX_ACCELERATION * NewMultiplier;
	}

	// 슬라이딩 로직
	/*
	if (CurrentCharacterState = ECharacterState::Slide)
	{
		GetCharacterMovement()->GroundFriction = 0.f;
		GetCharacterMovement()->Velocity *= 0.98f;
	}
	else
	{
		GetCharacterMovement()->GroundFriction = 8.f;
	}
	*/


	if (bIsTurningNow)
	{
		// Alpha 값을 시간에 따라 0에서 1로 증가시킨다.
		TurnAlpha += DeltaTime / TurnDuration;
		TurnAlpha = FMath::Min(TurnAlpha, 1.0f);

		// 시작 Transform과 끝 Transform 사이를 부드럽게 보간(Lerp/Slerp)한다.
		const FVector NewLocation = FMath::Lerp(TurnStartTransform.GetLocation(), TurnEndTransform.GetLocation(), TurnAlpha);
		const FQuat NewRotation = FQuat::Slerp(TurnStartTransform.GetRotation(), TurnEndTransform.GetRotation(), TurnAlpha);

		SetActorLocationAndRotation(NewLocation, NewRotation);

		// --- 4. 회전 완료 처리 ---
		if (TurnAlpha >= 1.0f)
		{
			bIsTurningNow = false;

			FVector NewForwardVector = GetActorForwardVector();
			GetCharacterMovement()->Velocity = NewForwardVector * ForwardSpeedBeforeTurn;
		}

		// 자동 회전 중에는 아래의 일반 이동 로직을 실행하지 않고 즉시 종료한다.
		return;
	}

	// 방향을 정의하고, 현재 앞방향으로 이동합니다.
	FVector ForwardDirection = FVector::ZeroVector;
	switch (CurrentTrackDirection)
	{
	case ETrackDirection::North: ForwardDirection = FVector::ForwardVector; break; // +X
	case ETrackDirection::East:  ForwardDirection = FVector::RightVector;   break; // +Y
	case ETrackDirection::South: ForwardDirection = -FVector::ForwardVector; break; // -X
	case ETrackDirection::West:  ForwardDirection = -FVector::RightVector;  break; // -Y
	}
	AddMovementInput(ForwardDirection, 1.0f);

	// 레인을 변경하는 로직입니다. bIsSwitchingLane 참일때만 동작합니다.
	if (bIsSwitchingLane)
	{
		LaneSwitchAlpha += DeltaTime / LaneSwitchDuration;
		LaneSwitchAlpha = FMath::Min(LaneSwitchAlpha, 1.0f);

		// 이동할 값을 시간에 따른 LaneSwitchAlpha에 따라 증가시키며 위치를 보정시킵니다.
		const float NewLateralOffset = FMath::Lerp(LaneSwitchStartLateralOffset, LaneSwitchEndLateralOffset, LaneSwitchAlpha);

		FVector NewLocation = GetActorLocation();

		// 현재 트랙 방향을 switch로 분기하여 고정 오프셋에 Lerp 값을 더합니다.
		switch (CurrentTrackDirection)
		{
		case ETrackDirection::North:
			NewLocation.Y = FixedLaneOffset + NewLateralOffset;
			break;
		case ETrackDirection::South:
			NewLocation.Y = FixedLaneOffset - NewLateralOffset;
			break;
		case ETrackDirection::East:
			NewLocation.X = FixedLaneOffset - NewLateralOffset;
			break;
		case ETrackDirection::West:
			NewLocation.X = FixedLaneOffset + NewLateralOffset;
			break;
		}
		UE_LOG(LogTemp, Log, TEXT("Movement Is Here, NewLateral : %.2f, Current Start Offset : %.2f, Current End Offset : %.2f"), NewLateralOffset, LaneSwitchStartLateralOffset, LaneSwitchEndLateralOffset);
		SetActorLocation(NewLocation);

		// 레인 변경 완료 시 넘었을지도 모르는 값을 보정해줍니다. (Min 함수 때문에 실행될 일은 매우 없습니다)
		if (LaneSwitchAlpha >= 1.0f)
		{
			bIsSwitchingLane = false;
			CurrentLane = TargetLane;

			// 최종 위치 보정
			NewLocation = GetActorLocation();

			switch (CurrentTrackDirection)
			{
			case ETrackDirection::North:
				NewLocation.Y = FixedLaneOffset + LaneSwitchEndLateralOffset;
				break;
			case ETrackDirection::South:
				NewLocation.Y = FixedLaneOffset - LaneSwitchEndLateralOffset;
				break;
			case ETrackDirection::East:
				NewLocation.X = FixedLaneOffset - LaneSwitchEndLateralOffset;
				break;
			case ETrackDirection::West:
				NewLocation.X = FixedLaneOffset + LaneSwitchEndLateralOffset;
				break;
			}
			SetActorLocation(NewLocation);
		}
	}
}

// Called to bind functionality to input
void AMRPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		Input->BindAction(IA_Left, ETriggerEvent::Triggered, this, &AMRPlayerCharacter::MoveLeft);
		Input->BindAction(IA_Right, ETriggerEvent::Triggered, this, &AMRPlayerCharacter::MoveRight);
		Input->BindAction(IA_MTJump, ETriggerEvent::Started, this, &AMRPlayerCharacter::Jump);
		Input->BindAction(IA_MTJump, ETriggerEvent::Completed, this, &AMRPlayerCharacter::StopJumping);
		Input->BindAction(IA_Slide, ETriggerEvent::Triggered, this, &AMRPlayerCharacter::StartSlide);
		Input->BindAction(IA_Slide, ETriggerEvent::Completed, this, &AMRPlayerCharacter::StopSlide);
		Input->BindAction(IA_Escape, ETriggerEvent::Started, this, &AMRPlayerCharacter::OnInputEscape);
	}
}

// 점프를 오버라이드한 함수입니다. 점프 입력을 받고 버퍼링 시간을 잽니다.
void AMRPlayerCharacter::Jump()
{
	if (bIsTurningNow) return;
	if (CanJump())
	{
		Super::Jump();  // 물리적으로 점프

		UE_LOG(LogTemp, Warning, TEXT("JumpState"));

		if (JumpMontage)
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				AnimInstance->Montage_Play(JumpMontage, 1.0f);
			}
		}
	}
}

// 땅에 닿았을 경우 호출합니다. 점프 버퍼링이 아직 Clear 되지 않았을 경우에 곧바로 다시 점프합니다.
void AMRPlayerCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (LandedEffectVFX)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), LandedEffectVFX, GetActorLocation(), GetActorRotation());
	}

	// 2. 착지 사운드(SFX)를 재생합니다.
	if (LandedEffectSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(this, LandedEffectSFX, GetActorLocation());
	}

	if (bWantsToJump)
	{
		bWantsToJump = false;
		Jump();
	}
}

// 주의! 타일 매니저와의 종속 관계가 주어진 함수입니다. 수정이 정말 필요하다면 말해주세요.
void AMRPlayerCharacter::ExecuteForceTurn(const FTransform& AlignmentTransform, ETrackDirection NewDirection)
{
	if (bIsTurningNow) return;

	// 회전 시의 중앙값을 가짐에 따라 Center로 상태를 강제 고정합니다.
	CurrentLane = ECharacterLane::Center;
	TargetLane = ECharacterLane::Center;

	// 트랙 방향도 마저 업데이트합니다.
	CurrentTrackDirection = NewDirection;
	FVector CurrentLocation = GetActorLocation();

	TurnAlpha = 0.0f;
	TurnStartTransform = GetActorTransform();

	ForwardSpeedBeforeTurn = FVector::DotProduct(GetVelocity(), GetActorForwardVector());


	// 목표 위치 계산 (Z축 높이는 현재 높이 유지)
	const FVector TargetXYLocation = AlignmentTransform.GetLocation();
	FVector AlignedLocation;


	switch (NewDirection)
	{
	case ETrackDirection::North:
	case ETrackDirection::South:
		AlignedLocation = FVector(CurrentLocation.X, TargetXYLocation.Y, CurrentLocation.Z);
		FixedLaneOffset = CurrentLocation.Y;
		break;
	case ETrackDirection::West:
	case ETrackDirection::East:
		AlignedLocation = FVector(TargetXYLocation.X, CurrentLocation.Y, CurrentLocation.Z);
		FixedLaneOffset = CurrentLocation.X;
		break;
	default:
		break;
	}

	// 목표 회전값 계산
	const FRotator NewRotation = AlignmentTransform.GetRotation().Rotator();

	TurnEndTransform = FTransform(NewRotation, AlignedLocation);

	// 이동 오프셋 값은 초기화해서 이동 정보를 초기화합니다. (강제 중앙 정렬)
	LaneSwitchEndLateralOffset = 0;
	LaneSwitchStartLateralOffset = 0;

	// 각종 상태 변화를 기록합니다.
	bIsTurningNow = true;
	bIsSwitchingLane = false;

	UE_LOG(LogTemp, Log, TEXT("Control Axis Rotated. New direction: %s"), *UEnum::GetValueAsString(NewDirection));
}
void AMRPlayerCharacter::OnInputJump(const FInputActionValue& Value)
{
	if (Value.Get<bool>())
	{
		Jump();
	}
	else
	{
		StopJumping();
	}
}

void AMRPlayerCharacter::OnInputEscape_Implementation(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("This Text Should Not To LOG!, Cheack Escape IA Binding"));
}

// 왼쪽 이동 함수입니다. 요청의 책임만 가지고 있습니다.
void AMRPlayerCharacter::MoveLeft()
{
	if (bIsSwitchingLane || bIsTurningNow) return;
	int32 NewLaneIndex = FMath::Max(0, static_cast<int32>(CurrentLane) - 1);
	TargetLane = static_cast<ECharacterLane>(NewLaneIndex);
	if (TargetLane != CurrentLane)
	{
		StartLaneSwitch();
	}
}

// 오른쪽 이동 함수입니다. 요청의 책임만 가지고 있습니다.
void AMRPlayerCharacter::MoveRight()
{
	if (bIsSwitchingLane || bIsTurningNow) return;
	int32 NewLaneIndex = FMath::Min(2, static_cast<int32>(CurrentLane) + 1);
	TargetLane = static_cast<ECharacterLane>(NewLaneIndex);
	if (TargetLane != CurrentLane)
	{
		StartLaneSwitch();
	}
}

// 상태를 바꿔 Tick()이 이동 로직을 실행하도록 하는 함수입니다. TargetLane을 바꾸는 작업도 겸합니다.
void AMRPlayerCharacter::StartLaneSwitch()
{
	bIsSwitchingLane = true;
	LaneSwitchAlpha = 0.0f;

	const FVector CurrentLocation = GetActorLocation();

	LaneSwitchStartLateralOffset = LaneSwitchEndLateralOffset;

	const int32 LogicalLaneIndex = static_cast<int32>(TargetLane) - 1; // 0,1,2 -> -1,0,1
	LaneSwitchEndLateralOffset = LogicalLaneIndex * LaneWidth;
}

// 슬라이딩을 시작하는 함수입니다. 키를 떼거나, 시간이 지났을 때 자동으로 StopSlide()를 호출합니다.
void AMRPlayerCharacter::StartSlide()
{
	if (GetCharacterMovement()->IsFalling())
		return;

	// CurrentCharacterState = ECharacterState::Slide;

	GetCapsuleComponent()->SetCapsuleHalfHeight(DefualtCapsuleHalfSize / 2.f);

	// PlayAnimMontage(...)

	GetWorld()->GetTimerManager().SetTimer(SlideTimeHandler, this, &AMRPlayerCharacter::StopSlide, SlideDuration, false);
}

void AMRPlayerCharacter::StopSlide()
{
	GetCapsuleComponent()->SetCapsuleHalfHeight(DefualtCapsuleHalfSize);

	//StopAnimMontage(...);

	GetWorld()->GetTimerManager().ClearTimer(SlideTimeHandler);
}

// 점프 버퍼링을 끝내는 함수입니다.
void AMRPlayerCharacter::ClearJumpBuffer()
{
	bWantsToJump = false;
}

void AMRPlayerCharacter::GetDamaged(float DamageAmount)
{

	if (HealthComp)
	{
		// Implement damage logic here
		HealthComp->GetDamage(DamageAmount);
	}

	if (bIsInvincible)
	{
		// 무적 상태이므로 피해 무시
		return;
	}
}

void AMRPlayerCharacter::ItemActivated(EItemEffectTypes ItemTypes)
{

	// Implement item activation logic here
	if (EffectComponent)
	{
		EffectComponent->ApplyEffect(ItemTypes);
	}
}