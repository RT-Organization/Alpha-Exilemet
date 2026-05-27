// Fill out your copyright notice in the Description page of Project Settings.


#include "SkeletalToolBase.h"

ASkeletalToolBase::ASkeletalToolBase()
{
    SkeletalMeshComp = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
    RootComponent = SkeletalMeshComp;
    Mesh = SkeletalMeshComp;

    // Null out the static one — it was never created in this subclass
    StaticMeshComp = nullptr;

    SkeletalMeshComp->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    SkeletalMeshComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}