// Fill out your copyright notice in the Description page of Project Settings.


#include "ToolBase.h"

// Sets default values
AToolBase::AToolBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AToolBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void AToolBase::StartUsing_Implementation()
{
	// default empty
}

void AToolBase::StopUsing_Implementation()
{
	// default empty
}