// Fill out your copyright notice in the Description page of Project Settings.


#include "VoiceCharacter.h"

// Sets default values
AVoiceCharacter::AVoiceCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AVoiceCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AVoiceCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AVoiceCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

