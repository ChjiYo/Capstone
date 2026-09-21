#include "EquipmentComponent.h"
#include "InventoryComponent.h"
#include "CharBase.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "CapstoneSFX.h"

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEquipmentComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (int32 i = 0; i < SlotCount; ++i)
	{
		if (AttachedMeshes[i])
		{
			AttachedMeshes[i]->DestroyComponent();
			AttachedMeshes[i] = nullptr;
		}
	}

	Super::EndPlay(EndPlayReason);
}

UInventoryComponent* UEquipmentComponent::GetInventory() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<UInventoryComponent>() : nullptr;
}

ACharBase* UEquipmentComponent::GetOwnerCharacter() const
{
	return Cast<ACharBase>(GetOwner());
}

bool UEquipmentComponent::GetEquipInfo(FName ItemID, FEquipInfo& OutInfo) const
{
	if (ItemID.IsNone())
		return false;

	const UInventoryComponent* Inventory = GetInventory();
	const FItemData* Row = Inventory ? Inventory->FindTableRow(ItemID) : nullptr;
	const FEquipInfo* Def = EquipDefs::FindDefinition(ItemID);

	// DataTable 행에 장비 정보가 채워져 있으면 그것이 원본이다
	if (Row && Row->EquipSlot != EEquipSlot::None)
	{
		OutInfo = FEquipInfo();
		OutInfo.Slot = Row->EquipSlot;
		OutInfo.Name = Row->DisplayName;
		OutInfo.AttackBonus = Row->AttackBonus;
		OutInfo.MaxHpBonus = Row->MaxHpBonus;
		OutInfo.Mesh = Row->EquipMesh;
		OutInfo.Socket = Row->EquipSocket;
		OutInfo.Offset = Row->EquipOffset;
		OutInfo.bBuiltIn = Def && Def->bBuiltIn;

		if (Def)
			OutInfo.Description = Def->Description;

		return true;
	}

	if (!Def)
		return false;

	OutInfo = *Def;

	// 이름은 표에 있는 것을 우선한다 (번역/수정을 에디터에서 할 수 있게)
	if (Row && !Row->DisplayName.IsEmpty())
		OutInfo.Name = Row->DisplayName;

	return true;
}

UTexture2D* UEquipmentComponent::ResolveIcon(FName ItemID) const
{
	const UInventoryComponent* Inventory = GetInventory();
	const FItemData* Data = Inventory ? Inventory->GetItemData(ItemID) : nullptr;

	if (Data && !Data->Icon.IsNull())
	{
		if (UTexture2D* Icon = Data->Icon.LoadSynchronous())
			return Icon;
	}

	return EquipDefs::GetGeneratedIcon(ItemID);
}

FText UEquipmentComponent::MakeTooltip(FName ItemID, bool bEquipped) const
{
	FEquipInfo Info;
	if (!GetEquipInfo(ItemID, Info))
		return FText::GetEmpty();

	FString Text = FString::Printf(TEXT("%s\n[%s]"), *Info.Name.ToString(), *EquipDefs::GetSlotLabel(Info.Slot).ToString());

	if (!Info.Description.IsEmpty())
		Text += TEXT("\n") + Info.Description.ToString();

	if (Info.AttackBonus != 0)
		Text += FString::Printf(TEXT("\n공격력 %+d"), Info.AttackBonus);

	if (Info.MaxHpBonus != 0)
		Text += FString::Printf(TEXT("\n최대 체력 %+d"), Info.MaxHpBonus);

	Text += bEquipped ? TEXT("\n\n클릭하면 장비를 벗습니다") : TEXT("\n\n클릭하거나 장비창으로 끌어다 놓으면 장착합니다");

	return FText::FromString(Text);
}

FName UEquipmentComponent::GetEquippedItem(EEquipSlot Slot) const
{
	const int32 Index = SlotIndex(Slot);

	return Index >= 0 && Index < SlotCount ? Equipped[Index] : NAME_None;
}

int32 UEquipmentComponent::GetTotalAttackBonus() const
{
	int32 Total = 0;

	for (int32 i = 0; i < SlotCount; ++i)
	{
		FEquipInfo Info;
		if (GetEquipInfo(Equipped[i], Info))
			Total += Info.AttackBonus;
	}

	return Total;
}

int32 UEquipmentComponent::GetTotalMaxHpBonus() const
{
	int32 Total = 0;

	for (int32 i = 0; i < SlotCount; ++i)
	{
		FEquipInfo Info;
		if (GetEquipInfo(Equipped[i], Info))
			Total += Info.MaxHpBonus;
	}

	return Total;
}

TArray<FName> UEquipmentComponent::GetEquippedItems() const
{
	TArray<FName> Result;

	for (int32 i = 0; i < SlotCount; ++i)
		Result.Add(Equipped[i]);

	return Result;
}

void UEquipmentComponent::ApplyBonus(const FEquipInfo& Info, int32 Sign)
{
	ACharBase* Character = GetOwnerCharacter();

	if (!Character)
		return;

	if (Info.AttackBonus != 0)
		Character->AddAttackPower(Info.AttackBonus * Sign);

	if (Info.MaxHpBonus != 0)
		Character->AddMaxHp(Info.MaxHpBonus * Sign);
}

bool UEquipmentComponent::EquipFromInventory(FName ItemID)
{
	UInventoryComponent* Inventory = GetInventory();

	FEquipInfo NewInfo;
	if (!Inventory || !GetEquipInfo(ItemID, NewInfo) || NewInfo.Slot == EEquipSlot::None)
		return false;

	if (Inventory->GetItemCount(ItemID) < 1)
		return false;

	const int32 Index = SlotIndex(NewInfo.Slot);
	const FName OldItemID = Equipped[Index];

	// 새 장비를 먼저 꺼내야 그 자리에 벗은 장비가 들어갈 수 있다
	Inventory->RemoveItem(ItemID, 1);

	if (!OldItemID.IsNone() && !Inventory->CanAddItem(OldItemID, 1))
	{
		Inventory->AddItem(ItemID, 1);
		CapstoneSFX::Play2D(this, ESFX::UI_Deny);
		return false;
	}

	if (!OldItemID.IsNone())
	{
		FEquipInfo OldInfo;
		if (GetEquipInfo(OldItemID, OldInfo))
			ApplyBonus(OldInfo, -1);

		Inventory->AddItem(OldItemID, 1);
	}

	Equipped[Index] = ItemID;
	ApplyBonus(NewInfo, +1);
	RefreshVisual(NewInfo.Slot);

	CapstoneSFX::Play2D(this, ESFX::UI_ItemPickup);
	LOG(TEXT("장착 : %s (%s)"), *ItemID.ToString(), *NewInfo.Name.ToString());

	OnEquipmentChanged.Broadcast();
	return true;
}

bool UEquipmentComponent::Unequip(EEquipSlot Slot)
{
	UInventoryComponent* Inventory = GetInventory();
	const int32 Index = SlotIndex(Slot);

	if (!Inventory || Index < 0 || Index >= SlotCount || Equipped[Index].IsNone())
		return false;

	const FName ItemID = Equipped[Index];

	if (!Inventory->CanAddItem(ItemID, 1))
	{
		CapstoneSFX::Play2D(this, ESFX::UI_Deny);
		LOG(TEXT("장착 해제 실패 : 인벤토리가 가득 참 (%s)"), *ItemID.ToString());
		return false;
	}

	FEquipInfo Info;
	if (GetEquipInfo(ItemID, Info))
		ApplyBonus(Info, -1);

	Equipped[Index] = NAME_None;
	RefreshVisual(Slot);

	Inventory->AddItem(ItemID, 1);

	CapstoneSFX::Play2D(this, ESFX::UI_ItemDrop);
	LOG(TEXT("장착 해제 : %s"), *ItemID.ToString());

	OnEquipmentChanged.Broadcast();
	return true;
}

void UEquipmentComponent::EquipDefaults()
{
	for (EEquipSlot Slot : { EEquipSlot::Weapon, EEquipSlot::Shield })
	{
		const int32 Index = SlotIndex(Slot);
		const FName ItemID = EquipDefs::GetDefaultItem(Slot);

		FEquipInfo Info;
		if (!Equipped[Index].IsNone() || !GetEquipInfo(ItemID, Info))
			continue;

		Equipped[Index] = ItemID;
		ApplyBonus(Info, +1);
		RefreshVisual(Slot);
	}

	OnEquipmentChanged.Broadcast();
}

void UEquipmentComponent::RestoreEquipped(const TArray<FName>& Items)
{
	for (int32 i = 0; i < SlotCount; ++i)
	{
		Equipped[i] = Items.IsValidIndex(i) ? Items[i] : NAME_None;
		RefreshVisual(static_cast<EEquipSlot>(i + 1));
	}

	OnEquipmentChanged.Broadcast();
}

void UEquipmentComponent::RefreshVisual(EEquipSlot Slot)
{
	const int32 Index = SlotIndex(Slot);

	if (Index < 0 || Index >= SlotCount)
		return;

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;

	if (!Body)
		return;

	if (AttachedMeshes[Index])
	{
		AttachedMeshes[Index]->DestroyComponent();
		AttachedMeshes[Index] = nullptr;
	}

	FEquipInfo Info;
	const bool bHasItem = GetEquipInfo(Equipped[Index], Info);

	// 기본 검/방패는 캐릭터 메시에 구워져 있어서, 그 칸이 기본 장비가 아니면(비었거나 다른 장비) 해당 본을 숨겨 모델을 지운다
	const bool bShowBuiltIn = bHasItem && Info.bBuiltIn;

	for (const FName& Bone : EquipDefs::GetBuiltInBones(Slot))
	{
		if (bShowBuiltIn)
			Body->UnHideBoneByName(Bone);
		else
			Body->HideBoneByName(Bone, EPhysBodyOp::PBO_None);
	}

	if (!bHasItem || Info.bBuiltIn || Info.Mesh.IsNull())
		return;

	UStaticMesh* LoadedMesh = Info.Mesh.LoadSynchronous();

	if (!LoadedMesh)
	{
		LOG(TEXT("장비 모델을 불러오지 못함 : %s"), *Info.Mesh.ToString());
		return;
	}

	UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(GetOwner(), NAME_None, RF_Transient);
	MeshComp->SetStaticMesh(LoadedMesh);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetGenerateOverlapEvents(false);
	MeshComp->SetCanEverAffectNavigation(false);
	MeshComp->SetupAttachment(Body, Info.Socket);
	MeshComp->SetRelativeTransform(Info.Offset);
	MeshComp->RegisterComponent();

	AttachedMeshes[Index] = MeshComp;
}
