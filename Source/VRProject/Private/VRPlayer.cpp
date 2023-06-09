// Fill out your copyright notice in the Description page of Project Settings.


#include "VRPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "VRGameModeBase.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include <Camera/CameraComponent.h>
#include <MotionControllerComponent.h>
#include <DrawDebugHelpers.h>
#include <HeadMountedDisplayFunctionLibrary.h>
#include <Components/CapsuleComponent.h>
#include <NiagaraComponent.h>
#include <NiagaraDataInterfaceArrayFunctionLibrary.h>
#include <Haptics/HapticFeedbackEffect_Curve.h>
#include <UMG/Public/Components/WidgetInteractionComponent.h>

// Sets default values 생성자 :생성하고싶은거 여기 다가.
AVRPlayer::AVRPlayer()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("VRcamera"));
	VRCamera->SetupAttachment(RootComponent);
	//이부분 언리얼에서 체크 해제해준 그부분.
	VRCamera->bUsePawnControlRotation = false;
	

	//손추가
	LeftHand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftHand"));
	LeftHand->SetupAttachment(RootComponent); 
	LeftHand->SetTrackingMotionSource(FName("Left"));
	
	RightHand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightHand"));
	RightHand->SetupAttachment(RootComponent);
	RightHand->SetTrackingMotionSource(FName("Right"));

	//스켈레탈메시컴포넌트 만들기
	LeftHandMesh= CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LeftHandMesh"));

	//스켈레탈메시 로드해서 할당
	ConstructorHelpers::FObjectFinder<USkeletalMesh>TempMesh(TEXT("/Script/Engine.SkeletalMesh'/Game/Characters/MannequinsXR/Meshes/SKM_MannyXR_left.SKM_MannyXR_left'"));
	if (TempMesh.Succeeded())
	{
		LeftHandMesh->SetSkeletalMesh(TempMesh.Object);
		LeftHandMesh->SetRelativeLocation(FVector(-2.9f, -3.5f, 4.5f));
		LeftHandMesh->SetRelativeRotation(FRotator(-25, -180, 90));
		LeftHandMesh->SetupAttachment(LeftHand);

	}

	//오른손 메쉬
	RightHandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RightHandMesh"));
	ConstructorHelpers::FObjectFinder<USkeletalMesh> TempMesh2(TEXT("/Script/Engine.SkeletalMesh'/Game/Characters/MannequinsXR/Meshes/SKM_MannyXR_right.SKM_MannyXR_right'"));
		if (TempMesh2.Succeeded())
		{
			RightHandMesh->SetSkeletalMesh(TempMesh.Object);
			RightHandMesh->SetRelativeLocation(FVector(-2.9f, 3.5f, 4.5f));
			RightHandMesh->SetRelativeRotation(FRotator(25, 0, 90));
			RightHandMesh->SetupAttachment(RightHand);

		}
	// teleport
		// Teleport
		TeleportCircle = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TeleportCircle"));
		TeleportCircle->SetupAttachment(RootComponent);
		TeleportCircle->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		TeleportCurveComp = CreateDefaultSubobject < UNiagaraComponent>(TEXT("TeleportCurveComp"));
		TeleportCurveComp->SetupAttachment(RootComponent);

		// 집게손가락
		RightAim = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightAim"));
		RightAim->SetupAttachment(RootComponent);
		RightAim->SetTrackingMotionSource(FName("RightAim"));

		//
		WidgetInteractionComp = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("WidgetInteractionComp"));
		WidgetInteractionComp->SetupAttachment(RightAim);
}

// Called when the game starts or when spawned
void AVRPlayer::BeginPlay()
{
	Super::BeginPlay();
// Enhanced Input 사용처리
	//주황글씨 다 함수임. 함수는 인풋이 필요함
	//함수를 호출하면 변수(그릇)에다가 담아서 써야함
	auto PC = Cast<APlayerController>(GetWorld()->GetFirstPlayerController());

	if (PC)
	{//LocalPlayer
		auto localPlayer = PC->GetLocalPlayer();
		auto subSystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(localPlayer);
		if (subSystem)
		{
			subSystem->AddMappingContext(IMC_VRInput, 0);
			subSystem->AddMappingContext(IMC_Hand, 0);
		}
	}

	ResetTeleport();

	//크로스헤어 객체 만들기
	if (CrosshairFactory)
	{
		Crosshair = GetWorld()->SpawnActor<AActor>(CrosshairFactory);
	}

	//HMD 가 연결되어 있지 않다면
	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled() == false)
	{
	//Hand를 테스트 할 수 있는 위치로 이동시키자
	RightHand->SetRelativeLocation(FVector(20, 20, 0));
	RightAim->SetRelativeLocation(FVector(20, 20, 0));
	//카메라의 UsePawn Control Rotation 을 활성화 시키자
	VRCamera->bUsePawnControlRotation = true;
	}

	//만약 HMD가 연결되어 있다면
	//->기본 트랙킹 offset 설정
	UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Eye);

	
}

// Called every frame
void AVRPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// HMD가 연결돼 있지 않으면
	if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled() == false)
	{
		//->손이 카메라 방향과 일치하도록 하자
		RightHand->SetRelativeRotation(VRCamera->GetRelativeRotation());
		RightAim->SetRelativeRotation(VRCamera->GetRelativeRotation());
	}

// 	
// 	FVector StartPos = RightAim->GetComponentLocation();
// 		//종료점
// 		FVector EndPos = StartPos + RightAim->GetForwardVector() * 10000;
// 		DrawDebugLine(GetWorld(), StartPos, EndPos, FColor::Blue, false, -1, 0, 1);
		//텔레포트 확인 처리
	if (bTeleporting)
	{
		//만약 직선을 그린다면
		if (bTeleportCurve == false)
		{
		DrawteleportStright();
		}
		//그렇지 않으면
		else
		{
			//곡선그리기
			DrawTeleportCurve();
		}
		// 	나이아가라를 이용해 선그리기
		if (TeleportCurveComp)
		{
			UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(TeleportCurveComp, FName("User.PointArray"), Lines);
		}
	}

	//크로스헤어
	DrawCrosshair();
	Grabbing();
	DrawDebugRemoteGrab();


}

// Called to bind functionality to input
void AVRPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	auto InputSystem = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	if (InputSystem)
	{
		//Binding for moving 움직임에대한 바인딩
		InputSystem->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AVRPlayer::Move);
		
		InputSystem->BindAction(IA_Mouse, ETriggerEvent::Triggered, this, &AVRPlayer::Turn);

		InputSystem->BindAction(IA_Teleport, ETriggerEvent::Started, this, &AVRPlayer::TeleportStart);

		InputSystem->BindAction(IA_Teleport, ETriggerEvent::Completed, this, &AVRPlayer::TeleportEnd);

		InputSystem->BindAction(IA_Fire, ETriggerEvent::Started, this, &AVRPlayer::FireInput);
		InputSystem->BindAction(IA_Fire, ETriggerEvent::Completed, this, &AVRPlayer::ReleaseUIInput);

		// 잡기 시작과 마무리
		InputSystem->BindAction(IA_Grab, ETriggerEvent::Started, this, &AVRPlayer::TryGrab);
		InputSystem->BindAction(IA_Grab, ETriggerEvent::Completed, this, &AVRPlayer::UnTryGrab);
	}

}

void AVRPlayer::Move(const FInputActionValue& Values)
{
	//1.앞뒤좌우는어디서 가져올까? 사용자의 입력에 따라
	FVector2D Axis = Values.Get<FVector2D>();
	AddMovementInput(GetActorForwardVector(), Axis.X);
	AddMovementInput(GetActorRightVector(), Axis.Y);
// 	UE_LOG(LogTemp, Warning, TEXT("Move!!"));
// 	//사용자의 입력에 따라 앞뒤 좌우로 이동하고 싶다.
// 	//2.앞뒤좌우라는 방향이 필요.
// 	FVector Dir(Axis.X, Axis.Y, 0);
// 	//3.이동하고싶다.(하고싶은걸 먼저찾기)
// 	//P=P0+VT
// 	FVector P0 = GetActorLocation();
// 	FVector vt = Dir * moveSpeed * GetWorld()->DeltaTimeSeconds;
// 	FVector P = P0 + vt;
// 	SetActorLocation(P);
// 	//내가쓴거 = GetActorLocation() + Axis * Deltatime;
}

void AVRPlayer::Turn(const FInputActionValue& Values)
{
	FVector2D Axis = Values.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}


//텔레포트
void AVRPlayer::TeleportStart(const FInputActionValue& Values)
{
//누르고 있는 중에는 사용자가 어디를 가리키는지 주시하고 싶다.
	bTeleporting = true;
	//라인이 보이도록 활성화
	TeleportCurveComp->SetVisibility(true);
}


//텔레포트
void AVRPlayer::TeleportEnd(const FInputActionValue& Values)
{
	//텔레포트 기능 리셋시키기.(계속 떠있을수 없으니까)
	//만약 텔레포트가 불가능하다면
	if (ResetTeleport() == false)
	{

	//다음처리를 하지않는다.
	return;
	}
	//워프 사용시 워프처리
	if (IsWarp)
	{
		DoWarp();
		return;
	}
	// 그렇지 않을경우 텔레포으트
	//텔레포트 위치로 이동하고 싶다.
	SetActorLocation(TeleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	
}


bool AVRPlayer::ResetTeleport()
{
	//텔레포트써클이 활성화 되어 있을 때만 텔레포트 가능하다.
	bool bCanTeleport = TeleportCircle->GetVisibleFlag();
	//써클 안보이게 처리
	TeleportCircle->SetVisibility(false);
	bTeleporting = false;
	TeleportCurveComp->SetVisibility(false);
	return bCanTeleport;

}

void AVRPlayer::DrawteleportStright()
{
	Lines.RemoveAt(0, Lines.Num());
//직선을 그리고 싶다.
// 필요정보 : 시작점, 종료점
	FVector StartPos = RightHand->GetComponentLocation();
	FVector EndPos = StartPos + RightHand->GetForwardVector() * 1000;

	// 두 점 사이에 충돌체가 있는지 체크하자
	CheckHitTeleport(StartPos, EndPos);
	Lines.Add(StartPos);
	Lines.Add(EndPos);
	//DrawDebugLine(GetWorld(), StartPos, EndPos, FColor::Red, false, -1, 0, 1);
	}



bool AVRPlayer::CheckHitTeleport(FVector LastPos, FVector& CurPos)
{
	FHitResult HitInfo;
	bool bHit = HitTest(LastPos, CurPos, HitInfo);
	//만약 부딪힌 대상이 바닥이라면
	if (bHit && HitInfo.GetActor()->GetName().Contains(TEXT("Floor")))
	{
		//마지막 점을 (EndPos) 최종 점으로 수정하고 싶다.
		CurPos = HitInfo.Location;

		//써클 활성화
		TeleportCircle->SetVisibility(true);
		//텔레포트써클을 위치 시키고싶다.
		TeleportLocation = CurPos;
		TeleportCircle->SetWorldLocation(TeleportLocation);
	}
	else
	{
		TeleportCircle->SetVisibility(false);
	}
	return bHit;
}


bool AVRPlayer::HitTest(FVector LastPos, FVector CurPos, FHitResult& HitInfo)
{
	FCollisionQueryParams Params;
		// 자기자신은 무시해라
		Params.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(HitInfo, LastPos, CurPos, ECC_Visibility, Params);

	return bHit;
}

//주어진 속도로 투사체를 날려보내고 투사체의 지나간점을 기록하자.
void AVRPlayer::DrawTeleportCurve()
{
	//Lines 초기화
	Lines.RemoveAt(0, Lines.Num());
	//1.시작점, 방향, 힘 -이것으로 투사체를 던진다.
	FVector Pos = RightHand->GetComponentLocation();
	FVector Dir = RightHand->GetForwardVector() * CurvedPower;

	//시작점을 가장먼저 기록하고 그뒤로 기록하는 For문이 시작되도록
	Lines.Add(Pos);
	for(int i = 0; i < LineSmooth; i++)
	{

	//이전 점 기억
		FVector LastPos = Pos;
	//2.투사체가 이동했으니까 반복적으로
	//V = v0+at
		Dir += FVector::UpVector * Gravity * SimulatedTime;
	//p=p0+vt
		Pos += Dir * SimulatedTime;
	//3.투사체의 위치에서
	// ->점과 점 사이에 물체가 가로막고 있다면
		if (CheckHitTeleport(LastPos, Pos))
		{
			//그점을 마지막점으로 하자
			Lines.Add(Pos);
			break;
		}
	
	//4.점을 기록하자
		Lines.Add(Pos);
	}

// 	//곡선 그리기
// 	for (int i = 0; i < Lines.Num()-1; i++)
// 	{
// 		DrawDebugLine(GetWorld(), Lines[i], Lines[i + 1], FColor::Red, false, -1, 0, 1);
// 	}


}

void AVRPlayer::DoWarp()
{
	//워프기능이 활성화 되어 있을 때
	if (IsWarp == false)
	{
		return;
	}
	//워프 처리 하고싶다.
	// =>일정시간동안 빠르게 이동하는거야
	//경과시간 초기화
	CurTime = 0;
	//충돌체 비활성화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//1 .시간이 흘러야한다.
	//2. 일정시간동안
	//[캡처]()->{body}
	GetWorld()->GetTimerManager().SetTimer(WarpHandle, FTimerDelegate::CreateLambda(
		[this]()->void
		{
			//body
			//일정시간안에 목적지에 도착하고 싶다.
			//1. 시간이 흘러야 한다.
			CurTime += GetWorld()->DeltaTimeSeconds;
			// 현재
			FVector CurPos = GetActorLocation();
			// 도착
			FVector EndPos = TeleportLocation + FVector::UpVector * GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			// 2. 이동해야한다.
			CurPos = FMath::Lerp<FVector>(CurPos, EndPos, CurTime / WarpTime);
			// 3. 목적지에 도착
			SetActorLocation(CurPos);
			// 시간이 다 흘렀다면 
			if (CurTime >= WarpTime)
			{
				// -> 그 위치로 할당하고
				SetActorLocation(EndPos);
				// -> 타이머 종료해주기
				GetWorld()->GetTimerManager().ClearTimer(WarpHandle);
				GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}
			/*거리가 거의가까워졌다면 그 위치로 할당해주기.
			//->타이머 종료하기(워프 종료)
						float Distance = FVector::Dist(CurPos, EndPos);
			if (Distance < 0.1f)
			{
				CurPos = EndPos;
			}
			*/
		}
	), 0.02f, true);
}

void AVRPlayer::FireInput(const FInputActionValue& Values)
{


	//UI에 이벤트 전달하고 싶다.
	if (WidgetInteractionComp)
	{
		WidgetInteractionComp->PressPointerKey(FKey(FName("LeftMouseButton")));
	}

	// 진동처리 하고 싶다.
	auto PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->PlayHapticEffect(HF_Fire, EControllerHand::Right);
	}

	//LineTrace 이용해서 총을 쏘고 싶다.
	//시작점
	FVector StartPos = RightAim->GetComponentLocation();
	//종료점
	FVector EndPos = StartPos + RightAim->GetForwardVector() * 10000;
	//총쏘기(LineTrace 동작
	FHitResult HitInfo;
	bool bHit = HitTest(StartPos, EndPos, HitInfo);
	//만약 부딪치면 녀석이 있으면 날려보내자
	if (bHit)
	{
		auto HitComp = HitInfo.GetComponent();
		if (HitComp && HitComp->IsSimulatingPhysics())
		{
			//날려보내자
			//F=ma
			HitComp->AddForceAtLocation
				((EndPos - StartPos).GetSafeNormal() * HitComp->GetMass() * 100000, HitInfo.Location);
		}
	}
}


void AVRPlayer::ReleaseUIInput()
{
	if (WidgetInteractionComp)
	{
		WidgetInteractionComp->ReleasePointerKey(FKey(FName("LeftMouseButton")));
	}
}

// 거리에 따라서 크로스헤어 크기가 같게 보이도록하자
void AVRPlayer::DrawCrosshair()
{
	// 시작점
	FVector StartPos = RightAim->GetComponentLocation();
	// 끝점
	FVector EndPos = StartPos + RightAim->GetForwardVector() * 10000;
	// 충돌정보를 저장
	FHitResult HitInfo;
	// 충돌체크
	bool bHit = HitTest(StartPos, EndPos, HitInfo);

	float Distance = 0;
	// -> 충돌이 발생하면 
	if (bHit)
	{
		//      -> 충돌한 지점에 크로스헤어 표시
		Crosshair->SetActorLocation(HitInfo.Location);
		Distance = HitInfo.Distance;
	}
	// -> 그렇지 않으면
	else
	{
		//      -> 그냥 끝점에 크로스헤어 표시
		Crosshair->SetActorLocation(EndPos);
		Distance = (EndPos - StartPos).Size();
	}

	Crosshair->SetActorScale3D(FVector(FMath::Max<float>(1, Distance)));

	// 빌보딩
	// -> 크로스헤어가 카메라를 바라보도록 처리
	FVector Direction = Crosshair->GetActorLocation() - VRCamera->GetComponentLocation();
	Crosshair->SetActorRotation(Direction.Rotation());
}

//물체를 잡고싶다.
void AVRPlayer::TryGrab()
{
	//원거리 잡기 활성화 되어 있으면
	if (IsRemoteGrab)
	{
		RemoteGrab();
		//아래는 처리하지 않는다.
		return;
	}
	// 중심점
	FVector Center = RightHand->GetComponentLocation();
	// 충돌체크(구충돌)
	// 충돌한 물체들 기록할 배열
	// 충돌 질의 작성
	FCollisionQueryParams Param;
	Param.AddIgnoredActor(this);
	Param.AddIgnoredComponent(RightHand);
	TArray<FOverlapResult> HitObjs;
	bool bHit = GetWorld()->OverlapMultiByChannel(HitObjs, Center, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(GrabRange), Param);

	//충돌하지 않았다면
	if (bHit == false)
	{
		return;
	}
	// -> 가장 가까운 물체 잡도록 하자(검출과정)
	
   // 가장 가까운 물체 인덱스
	int Closest = 0;
	for (int i = 0; i < HitObjs.Num(); i++)
	{
		// 1. 물리 기능이 활성화 되어 있는 녀석만 판단
		// -> 만약 부딪힌 컴포넌트가 물리기능이 비활성화 되어 있다면
		if (HitObjs[i].GetComponent()->IsSimulatingPhysics() == false)
		{
			// 검출하고 싶지 않다.
			continue;
		}
		//잡았다!
		IsGrabbed = true;

		// 2. 현재 손과 가장 가까운 녀석과 이번에 검출할 녀석과 더 가까운 녀석이 있다면
		// -> 필요속성 : 현재 가장가까운 녀석과 손과의 거리
		float ClosestDist = FVector::Dist(HitObjs[Closest].GetActor()->GetActorLocation(), Center);
		// -> 필요속성 : 이번에 검출할 녀석과 손과의 거리
		float NextDist = FVector::Dist(HitObjs[i].GetActor()->GetActorLocation(), Center);
		// 3. 만약 이번에가 현재꺼 보다 더 가깝다면
		if (NextDist < ClosestDist)
		{
			//  -> 가장 가까운 녀석으로 변경하기
			Closest = i;
		}
	}
	// 만약 잡았다면
	if (IsGrabbed)
	{
		GrabbedObject = HitObjs[Closest].GetComponent();
		// -> 물체 물리기능 비활성화
		GrabbedObject->SetSimulatePhysics(false);
		GrabbedObject->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// -> 손에 붙여주자
		GrabbedObject->AttachToComponent(RightHand, FAttachmentTransformRules::KeepWorldTransform);

	}
	
	
}


//잡은 녀석이 있으면 놓고싶다.
void AVRPlayer::UnTryGrab()
{
	if (IsGrabbed == false)
	{
		return;
	}
	//1.잡지않은 상태로 전환
	IsGrabbed = false;
	//2.손에서 떼어내기
	GrabbedObject->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	//3.물리기능 활성화
	GrabbedObject->SetSimulatePhysics(true);
	//4.충돌기능 활성화
	GrabbedObject->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	//던지기
	GrabbedObject->AddForce(ThrowDirection * ThrowPower * GrabbedObject->GetMass());

	//회전 시키기
	//각속도 = (1 / dt) * dTheta(특정 축 기준 변위 각도 Axis, angle)
	float Angle;
	FVector Axis; //Asis는 축
	DeltaRotation.ToAxisAndAngle(Axis, Angle);
	float dt = GetWorld()->DeltaTimeSeconds;
	FVector AngularVelocity = (1.0f / dt) * Angle * Axis;
	GrabbedObject->SetAllPhysicsAngularVelocityInRadians(AngularVelocity * ToquePower, true);

	GrabbedObject = nullptr;
}

//던질 정보를 업데이트 하기위한 함수

void AVRPlayer::Grabbing()
{
	if (IsGrabbed == false)
	{
		return;
	}
	//던질방향 업데이트
	//타겟(현재나의손위치) - me(이전에 나의 손 위치) 
	ThrowDirection = RightHand->GetComponentLocation() - PrevPos;
	//회전방향 업데이트
	//쿼터니온 공식 (사원
	//Angle1 = Q1, Angle2 = Q2
	//Angle1 + Angle2 = Q1 * Q2
	//-Angle1 = Q1.Inverse()
	//Angle2 - Angle1 = Q2* Q1.Inverse()
	DeltaRotation = RightHand->GetComponentQuat() * PrevRot.Inverse();

	//이전위치 업데이트
	PrevPos = RightHand->GetComponentLocation();
	//이전회전값 업데이트
	PrevRot = RightHand->GetComponentQuat();
}

void AVRPlayer::RemoteGrab()
{
	// 충돌체크(구충돌)
	// 충돌한 물체들 기록할 배열
	// 충돌 질의 작성
	FCollisionQueryParams Param;
	Param.AddIgnoredActor(this);
	Param.AddIgnoredComponent(RightAim);
	FVector StartPos = RightHand->GetComponentLocation();
	FVector EndPos = StartPos + RightAim->GetForwardVector() * RemoteDistance;

	//SweepTestByChannel 을 사용하여 특정 부분만 적용되게
	FHitResult HitInfo;
	bool bHit = GetWorld()->SweepSingleByChannel(HitInfo, StartPos, EndPos, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(RemoteRadius), Param);

	//충돌이 되면 잡아당기기 애니메이션 실행
	//(거리가 필요하겠네?float)
	if (bHit && HitInfo.GetComponent()->IsSimulatingPhysics())//무버블로 되어있는 애들만 잡게 하겠다.
	{
		// 잡았다
		IsGrabbed = true;
		// 잡은 물체 할당
		GrabbedObject = HitInfo.GetComponent();
		// -> 물체 물리기능 비활성화
		GrabbedObject->SetSimulatePhysics(false);
		GrabbedObject->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// -> 손에 붙여주자
		GrabbedObject->AttachToComponent(RightHand, FAttachmentTransformRules::KeepWorldTransform);

		PrevPos = RightHand->GetComponentLocation();
		PrevRot = RightHand->GetComponentQuat();


		// 원거리 물체가 손으로 끌려오도록 처리
		GetWorld()->GetTimerManager().SetTimer(GrabTimer, FTimerDelegate::CreateLambda(
			[this]()->void
			{
				//물체가 -> 손 위치로 도착
				if (GrabbedObject == nullptr)
				{
					GetWorld()->GetTimerManager().ClearTimer(GrabTimer);
					return;
				}
				//  물체가 -> 손 위치로 도착
				FVector Pos = GrabbedObject->GetComponentLocation();
				FVector TargetPos = RightHand->GetComponentLocation();
				Pos = FMath::Lerp<FVector>(Pos, TargetPos, RemoteMoveSpeed * GetWorld()->DeltaTimeSeconds);
				GrabbedObject->SetWorldLocation(Pos);

				float Distance = FVector::Dist(Pos, TargetPos);
				//거의 가까워졌다면
				if (Distance < 10)
				{
					//이동 중단하기
					GrabbedObject->SetWorldLocation(TargetPos);

					//프리포즈 초기값_이전위치값
					//프리포즈 초기값
					PrevPos = RightHand->GetComponentLocation();
					PrevRot = RightHand->GetComponentQuat();
					GetWorld()->GetTimerManager().ClearTimer(GrabTimer);
				}
			}
		), 0.02f, true);
	}
}

void AVRPlayer::DrawDebugRemoteGrab()
{
	// 시각화 켜져있는지 여부 확인, 원거리물체 잡기 활성화 여부
	if (bDrawDebugRemoteGrab == false || IsRemoteGrab == false)
	{
		return;
	}

	FCollisionQueryParams Param;
	Param.AddIgnoredActor(this);
	Param.AddIgnoredComponent(RightAim);
	FVector StartPos = RightAim->GetComponentLocation();
	FVector EndPos = StartPos + RightAim->GetForwardVector() * RemoteDistance;

	FHitResult HitInfo;
	bool bHit = GetWorld()->SweepSingleByChannel(HitInfo, StartPos, EndPos, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(RemoteRadius), Param);

	// 그리기
	DrawDebugSphere(GetWorld(), StartPos, RemoteRadius, 10, FColor::Yellow);
	if (bHit)
	{
		DrawDebugSphere(GetWorld(), HitInfo.Location, RemoteRadius, 10, FColor::Yellow);
	}
}

