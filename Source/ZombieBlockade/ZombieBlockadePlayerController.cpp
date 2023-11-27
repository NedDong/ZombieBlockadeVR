// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZombieBlockadePlayerController.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "ZombieBlockadeCharacter.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "MouseRaycast.h"
#include "GridManager.h"
#include "Building.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AZombieBlockadePlayerController::AZombieBlockadePlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
}

void AZombieBlockadePlayerController::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void AZombieBlockadePlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		// Setup mouse input events
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this, &AZombieBlockadePlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this, &AZombieBlockadePlayerController::OnSetDestinationTriggered);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this, &AZombieBlockadePlayerController::OnSetDestinationReleased);
		EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &AZombieBlockadePlayerController::OnSetDestinationReleased);

		// Setup touch input events
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Started, this, &AZombieBlockadePlayerController::OnInputStarted);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Triggered, this, &AZombieBlockadePlayerController::OnTouchTriggered);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Completed, this, &AZombieBlockadePlayerController::OnTouchReleased);
		EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Canceled, this, &AZombieBlockadePlayerController::OnTouchReleased);

		//ZombieBlockade Code
		// New binding for right mouse button click
		EnhancedInputComponent->BindAction(SetBuildStructurehAction, ETriggerEvent::Started, this, &AZombieBlockadePlayerController::OnBuildStructureTriggered);
		
		// New binding for mouse button scroll
		EnhancedInputComponent->BindAction(SetSwitchSelectedForwardAction, ETriggerEvent::Started, this, &AZombieBlockadePlayerController::OnScrollForward);
		EnhancedInputComponent->BindAction(SetSwitchSelectedBackwardAction, ETriggerEvent::Started, this, &AZombieBlockadePlayerController::OnScrollBackward);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AZombieBlockadePlayerController::OnInputStarted()
{
	StopMovement();
}

// Triggered every frame when the input is held down
void AZombieBlockadePlayerController::OnSetDestinationTriggered()
{
	// We flag that the input is being pressed
	FollowTime += GetWorld()->GetDeltaSeconds();
	
	// We look for the location in the world where the player has pressed the input
	FHitResult Hit;
	bool bHitSuccessful = false;
	if (bIsTouch)
	{
		bHitSuccessful = GetHitResultUnderFinger(ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit);
	}
	else
	{
		bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
	}

	// If we hit a surface, cache the location
	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
	}
	
	// Move towards mouse pointer or touch
	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn != nullptr)
	{
		FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
	}
}

void AZombieBlockadePlayerController::OnSetDestinationReleased()
{
	// If it was a short press
	if (FollowTime <= ShortPressThreshold)
	{
		// We move there and spawn some particles
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination, FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}

	FollowTime = 0.f;
}

// Triggered every frame when the input is held down
void AZombieBlockadePlayerController::OnTouchTriggered()
{
	bIsTouch = true;
	OnSetDestinationTriggered();
}

void AZombieBlockadePlayerController::OnTouchReleased()
{
	bIsTouch = false;
	OnSetDestinationReleased();
}

void AZombieBlockadePlayerController::OnScrollForward()
{
	UGridManager::Instance()->TempSwitchSelectedBuilding(true, this);
}

void AZombieBlockadePlayerController::OnScrollBackward()
{
	UGridManager::Instance()->TempSwitchSelectedBuilding(false, this);
}

void AZombieBlockadePlayerController::OnBuildStructureTriggered()
{
	UGridManager::Instance()->DeploySelectedBuilding(this);
}

/*
void AZombieBlockadePlayerController::_OnBuildStructureTriggered()
{
	const BuildingInfo& info = GridManager::Instance().GetSelectedBuilding();

	// Return if nothing selected
	if (info.name.empty())
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("No building selected")));
		return;
	}

	AMouseRaycast::OnMouseClick(this, EKeys::RightMouseButton);
	FVector hitLocation = AMouseRaycast::GetMouseRaycast(this);
	GridCoord coord = GridManager::Instance().GetGridFromCoord(
		hitLocation.X - (info.sizeX - 1) * GridManager::Instance().GetGridSize() * 0.5,
		hitLocation.Y - (info.sizeY - 1) * GridManager::Instance().GetGridSize() * 0.5).coord;
	GridCoord exactCoord = GridManager::Instance().GetGridFromCoord(hitLocation.X, hitLocation.Y).coord;
	float gridSize = GridManager::Instance().GetGridSize();
	if (GridManager::Instance().CheckEmpty(coord, info.sizeX, info.sizeY))
	{
		// Add building
		std::wstring path = L"Blueprint'/Game/Blueprints/" + info.name + L"." + info.name + L"_C'";
		UClass* buildingClass = StaticLoadClass(AActor::StaticClass(), nullptr,
			reinterpret_cast<const TCHAR*>(path.c_str()));
		UWorld* world = GetWorld();
		FVector location = FVector(coord.first * gridSize, coord.second * gridSize, 0);

		if (world && buildingClass)
		{
			ABuilding* newBuilding = world->SpawnActor<ABuilding>(buildingClass, location, FRotator(0, 0, 0), {});
			newBuilding->coord = coord;
			GridManager::Instance().AddBuilding(newBuilding, true);
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(
				TEXT("Add building: <%d, %d>, <%d, %d>"), coord.first, coord.second, info.sizeX, info.sizeY));
		}
		else
		{
			// Display error message
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Cannot find BP of Building")));
		}
	}
	else if (GridManager::Instance().gridToBuilding.contains(exactCoord))
	{
		// Remove building
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(
			TEXT("Remove building: <%d, %d>"), exactCoord.first, exactCoord.second));
		ABuilding* OldBuilding = GridManager::Instance().gridToBuilding.at(exactCoord);
		GridManager::Instance().RemoveBuilding(OldBuilding);
		OldBuilding->Destroy();
	}
}
*/
