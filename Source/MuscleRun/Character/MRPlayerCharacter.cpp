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

	// 게임 시작 시 상태를 '달리기'로 강제 초기화하여 이전 상태에 갇히는 문제를 방지합니다.
	CharacterState = ECharacterState::ECS_Running;

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
	DefaultCapsuleHalfSize = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

	// 타일 매니저와의 종속관계로 인해, Tick()을 항상 타일 매니저 뒤로 실행합니다.
	if (ATileManager* TileManager = Cast<ATileManager>(UGameplayStatics::GetActorOfClass(GetWorld(), ATileManager::StaticClass())))
	{
		// "나(캐릭터)의 Tick은, 반드시 TileManager의 Tick이 끝난 후에 실행되어야 한다"
		// 라고 엔진에게 명시적으로 알려줍니다.
		AddTickPrerequisiteActor(TileManager);
	}
}

void AMRPlayerCharacter::EndSlideCooldown()
{
	// 쿨다운을 해제하는 새로운 함수
	bIsOnSlideCooldown = false;
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
// AMRPlayerCharacter.cpp

void AMRPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 시간에 따른 난이도 곡선을 적용합니다.
	if (CachedGameState)
	{
		double NewMultiplier = CachedGameState->GetGameSpeedMultiplier();

		// 1. 평소 달리기 속도를 계산합니다.
		const float NewMaxSpeed = (BASE_SPEED_MAX + SpeedBonus) * NewMultiplier;
		GetCharacterMovement()->MaxWalkSpeed = NewMaxSpeed;

		// 2. ✨ 웅크렸을 때의 속도도 똑같은 값으로 설정하여 속도 저하를 방지합니다.
		GetCharacterMovement()->MaxWalkSpeedCrouched = NewMaxSpeed;

		// 나머지 값들도 업데이트합니다.
		GetCharacterMovement()->GravityScale = BASE_GRAVITY_SCALE * NewMultiplier * NewMultiplier;
		GetCharacterMovement()->JumpZVelocity = BASE_JUMP_VELOCITY * NewMultiplier;
		GetCharacterMovement()->MaxAcceleration = BASE_MAX_ACCELERATION * NewMultiplier;
	}

	// 2. 코너링 로직을 실행합니다. (코너링 중에는 다른 이동 로직을 막기 위해 return 처리)
	if (bIsTurningNow)
	{
		TurnAlpha += DeltaTime / TurnDuration;
		TurnAlpha = FMath::Min(TurnAlpha, 1.0f);

		const FVector NewLocation = FMath::Lerp(TurnStartTransform.GetLocation(), TurnEndTransform.GetLocation(), TurnAlpha);
		const FQuat NewRotation = FQuat::Slerp(TurnStartTransform.GetRotation(), TurnEndTransform.GetRotation(), TurnAlpha);
		SetActorLocationAndRotation(NewLocation, NewRotation);

		if (TurnAlpha >= 1.0f)
		{
			bIsTurningNow = false;
			FVector NewForwardVector = GetActorForwardVector();
			GetCharacterMovement()->Velocity = NewForwardVector * ForwardSpeedBeforeTurn;
		}
		return; // 코너링 중에는 아래 로직을 실행하지 않음
	}

	// 3. 전진 이동 로직을 실행합니다. (슬라이딩 중에도 앞으로 나아가도록 수정됨)
	FVector ForwardDirection = FVector::ZeroVector;
	switch (CurrentTrackDirection)
	{
	case ETrackDirection::North: ForwardDirection = FVector::ForwardVector; break;
	case ETrackDirection::East:  ForwardDirection = FVector::RightVector;   break;
	case ETrackDirection::South: ForwardDirection = -FVector::ForwardVector; break;
	case ETrackDirection::West:  ForwardDirection = -FVector::RightVector;  break;
	}
	AddMovementInput(ForwardDirection, 1.0f);


	// --- ✨ 여기가 구조가 변경된 핵심 부분 ---

	// 4. 레인 '변경 중'일 때의 로직
	if (bIsSwitchingLane)
	{
		LaneSwitchAlpha += DeltaTime / LaneSwitchDuration;
		LaneSwitchAlpha = FMath::Min(LaneSwitchAlpha, 1.0f);

		const float NewLateralOffset = FMath::Lerp(LaneSwitchStartLateralOffset, LaneSwitchEndLateralOffset, LaneSwitchAlpha);
		FVector NewLocation = GetActorLocation();

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
		SetActorLocation(NewLocation);

		if (LaneSwitchAlpha >= 1.0f)
		{
			bIsSwitchingLane = false;
			CurrentLane = TargetLane;
			// 최종 위치 보정
			// (위치를 한번 더 설정하는 대신, 아래의 '레인 수호' 로직이 처리하도록 맡겨도 좋습니다)
		}
	}
	// 5. 레인 변경 중이 '아닐' 때의 "레인 절대 수호" 로직
	else
	{
		const int32 LogicalLaneIndex = static_cast<int32>(CurrentLane) - 1;
		const float TargetLateralOffset = LogicalLaneIndex * LaneWidth;
		FVector CorrectedLocation = GetActorLocation();
		bool bNeedsCorrection = false;

		switch (CurrentTrackDirection)
		{
		case ETrackDirection::North:
			if (!FMath::IsNearlyEqual(CorrectedLocation.Y, FixedLaneOffset + TargetLateralOffset, 1.0f))
			{
				CorrectedLocation.Y = FixedLaneOffset + TargetLateralOffset;
				bNeedsCorrection = true;
			}
			break;
		case ETrackDirection::South:
			if (!FMath::IsNearlyEqual(CorrectedLocation.Y, FixedLaneOffset - TargetLateralOffset, 1.0f))
			{
				CorrectedLocation.Y = FixedLaneOffset - TargetLateralOffset;
				bNeedsCorrection = true;
			}
			break;
		case ETrackDirection::East:
			if (!FMath::IsNearlyEqual(CorrectedLocation.X, FixedLaneOffset - TargetLateralOffset, 1.0f))
			{
				CorrectedLocation.X = FixedLaneOffset - TargetLateralOffset;
				bNeedsCorrection = true;
			}
			break;
		case ETrackDirection::West:
			if (!FMath::IsNearlyEqual(CorrectedLocation.X, FixedLaneOffset + TargetLateralOffset, 1.0f))
			{
				CorrectedLocation.X = FixedLaneOffset + TargetLateralOffset;
				bNeedsCorrection = true;
			}
			break;
		}

		if (bNeedsCorrection)
		{
			SetActorLocation(CorrectedLocation, false, nullptr, ETeleportType::TeleportPhysics);
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
		bWantsToJump = false;
		GetWorld()->GetTimerManager().ClearTimer(JumpBufferTimerHandler);

		if (JumpMontage)
		{
			UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				AnimInstance->Montage_Play(JumpMontage, 1.0f);
			}
		}
	}
	else
	{
		bWantsToJump = true;
		GetWorld()->GetTimerManager().SetTimer(JumpBufferTimerHandler, this, &AMRPlayerCharacter::ClearJumpBuffer, JumpBufferDuration, false);
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
		FixedLaneOffset = AlignmentTransform.GetLocation().Y;
		break;
	case ETrackDirection::West:
	case ETrackDirection::East:
		AlignedLocation = FVector(TargetXYLocation.X, CurrentLocation.Y, CurrentLocation.Z);
		FixedLaneOffset = AlignmentTransform.GetLocation().X;
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

	UE_LOG(LogTemp, Warning, TEXT("Control Axis Rotated. New direction: %s, Current Offsets : (%.2f, %2f)"), *UEnum::GetValueAsString(NewDirection), LaneSwitchEndLateralOffset, LaneSwitchStartLateralOffset);
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
// AMRPlayerCharacter.cpp

// AMRPlayerCharacter.cpp

// AMRPlayerCharacter.cpp

// StartSlide: 모든 문제 해결 로직이 적용된 최종 버전
void AMRPlayerCharacter::StartSlide()
{
	// 슬라이드가 불가능한 상태이거나, 쿨다운 중일 때는 함수를 즉시 종료합니다.
	if (bIsOnSlideCooldown ||
		CharacterState == ECharacterState::ECS_Jumping ||
		CharacterState == ECharacterState::ECS_Sliding ||
		bIsTurningNow)
	{
		return;
	}

	// 이전에 실행 중이던 타이머가 있다면 확실하게 제거합니다. (안전장치)
	GetWorld()->GetTimerManager().ClearTimer(SlideTimeHandler);
	GetWorld()->GetTimerManager().ClearTimer(SlideCooldownTimerHandle);

	ForwardSpeedBeforeSlide = GetVelocity().Size();
	CharacterState = ECharacterState::ECS_Sliding;

	// --- ✨ 여기가 추가된 디버그 코드 ---
	// 1. Crouch() 호출 전 캡슐 높이를 측정하고 흰색으로 출력합니다.
	float BeforeHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	FString BeforeString = FString::Printf(TEXT("Capsule Half Height BEFORE Crouch: %.1f"), BeforeHeight);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, BeforeString);

	Crouch(); // ✨ 우리가 테스트하려는 바로 그 함수입니다.

	// 2. Crouch() 호출 후 캡슐 높이를 다시 측정하고 노란색으로 출력합니다.
	float AfterHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	FString AfterString = FString::Printf(TEXT("Capsule Half Height AFTER Crouch: %.1f"), AfterHeight);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, AfterString);
	// --- 디버그 코드 끝 ---

	if (SlideMontage)
	{
		PlayAnimMontage(SlideMontage, 0.7f);
	}

	// 정해진 시간 후 자동으로 일어서도록 StopSlide 함수를 예약합니다.
	GetWorld()->GetTimerManager().SetTimer(SlideTimeHandler, this, &AMRPlayerCharacter::StopSlide, SlideDuration, false);
}


// StopSlide: 모든 문제 해결 로직이 적용된 최종 버전
void AMRPlayerCharacter::StopSlide()
{
	// 현재 슬라이딩 상태가 아니면, 중복 호출 방지를 위해 함수를 즉시 종료합니다.
	if (CharacterState != ECharacterState::ECS_Sliding)
	{
		return;
	}

	// --- 여기가 핵심 수정 부분 ---
	// 1. 슬라이드 재시작을 막기 위해 즉시 쿨다운 상태로 만듭니다.
	bIsOnSlideCooldown = true;

	// 2. 예약되어 있던 모든 관련 타이머를 깨끗하게 제거합니다.
	GetWorld()->GetTimerManager().ClearTimer(SlideTimeHandler);

	// 3. 아주 짧은 시간(0.2초) 후에 쿨다운을 해제하도록 새 타이머를 설정합니다.
	GetWorld()->GetTimerManager().SetTimer(SlideCooldownTimerHandle, this, &AMRPlayerCharacter::EndSlideCooldown, 0.2f, false);
	// --- 여기까지 ---

	UnCrouch();
	CharacterState = ECharacterState::ECS_Running;

	const FVector ForwardVector = GetActorForwardVector();
	GetCharacterMovement()->Velocity = ForwardVector * ForwardSpeedBeforeSlide;

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Stop(0.0f, SlideMontage);
	}
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