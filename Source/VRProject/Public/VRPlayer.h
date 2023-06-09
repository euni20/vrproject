// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputMappingContext.h"
#include "VRPlayer.generated.h"

//사용자의 입력에 따라 앞뒤 좌우로 이동하고 싶다.
//필요속성 : 이동속도, 입력액션, 입력매핑컨텍스트
//사용자가 텔로포트 버튼을 눌렀다 떼면 텔레포트 되도록 하고싶다.
//1. 텔레포트 버튼을 눌렀다 뗐으니까.
//2. 사용자가 그 지점을 가리켰으니까
//3. 텔레포트 목적지가 필요하다.
//4. 텔레포트이동하고싶다. 최종펫말

UCLASS()
class VRPROJECT_API AVRPlayer : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRPlayer();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality  to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	//필요속성 : 이동속도, 입력액션, 입력매핑컨텍스트
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	float moveSpeed = 500;
	
	//Input Mapping Context
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputMappingContext* IMC_VRInput; 

	//Input Action For Move
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_Move;

	//이동처리 함수 
	void Move(const FInputActionValue& Values);

public:
	
	//마우스 입력처리
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_Mouse;
	
	//회전처리 함수
	void Turn(const FInputActionValue& Values);

	//카메라 
protected:
	UPROPERTY(VisibleAnywhere, Category="VRCamera")
	class UCameraComponent* VRCamera;

	//컨트롤러 왼손오른손 붙일꺼양 어렵지않앙 영은아
	UPROPERTY(VisibleAnywhere, Category = "MotionController")
	class UMotionControllerComponent* LeftHand;
		
	UPROPERTY(VisibleAnywhere, Category = "MotionController")
	class UMotionControllerComponent* RightHand;
	
	//사용할 손 모델
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="MotionController")
	class USkeletalMeshComponent* LeftHandMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category="MotionController")
	class USkeletalMeshComponent* RightHandMesh;

public: // Teleport Straight
	UPROPERTY(VisibleAnywhere, Category = "Teleport")
	class UNiagaraComponent* TeleportCircle;

	//텔레포트 기능 활성화 여부
	bool bTeleporting = false;
	
	// 버튼 눌렀을 때 처리할 함수
	void TeleportStart(const FInputActionValue& Values);
	// 버튼 뗐을 때 처리할 함수
	void TeleportEnd(const FInputActionValue& Values);
	//텔레포트 리셋 기능
	bool ResetTeleport();
	
	//직선 텔레포트 처리하기
	void DrawteleportStright();
	
	//텔레포트 선과 충돌체크 함수
	bool CheckHitTeleport(FVector LastPos, FVector& CurPos);
	//충돌처리 함수
	bool HitTest(FVector LastPos, FVector CurPos, FHitResult& HitInfo);
	//텔레포트 입력액션
	UPROPERTY(EditDefaultsOnly, Category="Input")
	class UInputAction* IA_Teleport;

private:
	//곡선 텔레포트 
	//곡선 텔레포트 사용 여부
	UPROPERTY(EditAnywhere, Category="Teleport", meta=(AllowPrivateAccess =true))
	bool bTeleportCurve = true;
	// 던지는 힘
	UPROPERTY(EditAnywhere, Category="Teleport", meta=(AllowPrivateAccess =true))
	float CurvedPower = 1500;
	//중력
	UPROPERTY(EditAnywhere, Category="Teleport", meta=(AllowPrivateAccess =true))
	float Gravity = -5000;
	//시뮬레이션 시간
	UPROPERTY(EditAnywhere, Category="Teleport", meta=(AllowPrivateAccess =true))
	float SimulatedTime = 0.02f;
	//곡선을 이루는 점 개수
	UPROPERTY(EditAnywhere, Category="Teleport", meta=(AllowPrivateAccess =true))
	int32 LineSmooth = 40;
	//점을 기억할 배열
	UPROPERTY()
	TArray<FVector> Lines;
	
	void DrawTeleportCurve();

	//사용할 나이아가라 컴포넌트(LineTrace)
	UPROPERTY(VisibleAnywhere, Category="Teleport")
	class UNiagaraComponent* TeleportCurveComp;


	//워프 사용여부
	UPROPERTY(EditAnywhere, Category="Teleport", meta =(AllowPrivateAccess=true))
	bool IsWarp = true;

	//타이머
	UPROPERTY()
	FTimerHandle WarpHandle;

	//경과시간
	UPROPERTY()
	float CurTime = 0;

	//워프할 때 걸릴 시간
	UPROPERTY(EditAnywhere, Category = "Teleport", meta = (AllowPrivateAccess = true))
	float WarpTime = 0.2f;

	//워프 수행할 기능(함수)
	void DoWarp();

	
	// 텔레포트 위치
	FVector TeleportLocation;

private: // 총쏘기
   UPROPERTY(EditDefaultsOnly, Category="Input", meta=(AllowPrivateAccess=true))
   class UInputAction* IA_Fire;

   // 총쏘기 처리할 함수
   void FireInput(const FInputActionValue& Values);

   // 집게손가락 표시할 모션컨트롤러
   UPROPERTY(VisibleAnywhere, Category="HandComp", meta = (AllowPrivateAccess = true))
   class UMotionControllerComponent* RightAim;

	//Crosshair
    UPROPERTY(EditAnywhere, Category="Crosshair", meta = (AllowPrivateAccess = true))
	TSubclassOf<AActor> CrosshairFactory;

	//인스턴스 크로스헤어-태어나는기능
	AActor* Crosshair;

	//크로스헤어를 그리는 기능
	void DrawCrosshair();


public://잡기 버튼을 누르면 물체를 잡고 싶다.
	//필요속성 : 입력액션, 잡을 범위
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	class UInputAction* IA_Grab;

	//잡을 범위
	UPROPERTY(EditDefaultsOnly, Category = "Grab")
	float GrabRange = 100;

	//잡은 물체 기억
	UPROPERTY()
	class UPrimitiveComponent* GrabbedObject;

	//잡은녀석이 있는지 여부 기억할 변수
	bool IsGrabbed = false;

	// 던지면 원하는 방향으로 날아가도록 하고싶다.
	// 던질 방향
	FVector ThrowDirection;
	// 던질 힘
	UPROPERTY(EditAnywhere, Category="Grab")
	float ThrowPower = 1000;

	// 직전 위치
	FVector PrevPos;
	// 이전 회전값
	FQuat PrevRot;
	// 회전 방향
	FQuat DeltaRotation;
	//회전빠르기
	UPROPERTY(EditAnywhere, Category="Grab")
	float ToquePower = 1000; 

	//잡기시도 기능
	void TryGrab();
	//놓기
	void UnTryGrab();
	//잡고있는중
	void Grabbing();

	UPROPERTY(EditDefaultsOnly, Category = "Haptic")
	class UHapticFeedbackEffect_Curve* HF_Fire;

	// 원격잡기 모드가 활성화 되면 원격잡기 모드 사용하도록 하고싶다.
   //필요속성 : 원격잡기 모드 여부, 거리, 이동속도
   UPROPERTY(EditDefaultsOnly, Category="Grab")
   bool IsRemoteGrab = true;
   UPROPERTY(EditDefaultsOnly, Category="Grab")
   float RemoteDistance = 2000;
   UPROPERTY(EditDefaultsOnly, Category="Grab")
   float RemoteMoveSpeed = 10;
   //검출범위
   UPROPERTY(EditDefaultsOnly, Category="Grab")
   float RemoteRadius = 20;
   //원거리 물체 이동을 위한 타이머
   FTimerHandle GrabTimer;

   void RemoteGrab();

   
	//Remote Grab 시각화 처리할지 여부
   UPROPERTY(EditDefaultsOnly, Category="Grab")
   bool bDrawDebugRemoteGrab = true;
   // 시각화 처리할 함수
   void DrawDebugRemoteGrab();

protected: //Widget 관련 속성
	UPROPERTY(VisibleAnywhere, Category = "Widget")
	class UWidgetInteractionComponent* WidgetInteractionComp;

	//release 상태로 되돌려놓기
	void ReleaseUIInput();

public:
	UPROPERTY(EditDefaultsOnly, Category = "input")
	class UInputMappingContext* IMC_Hand;

};
