#include "FMGameMode.h"
#include "FMStructure.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FMCombatHUD.h"
#include "GameFramework/GameStateBase.h"
#include "FMPlayerState.h"
#include "FMCharacter.h"
#include "FMPlayerController.h"
#include "FMGameState.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"

AFMGameMode::AFMGameMode()
{
	HUDClass = AFMCombatHUD::StaticClass();
	DefaultPawnClass = AFMCharacter::StaticClass();
	PlayerStateClass = AFMPlayerState::StaticClass();
	PlayerControllerClass = AFMPlayerController::StaticClass();
	GameStateClass = AFMGameState::StaticClass();
}

void AFMGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
	bPlayerRoom = UGameplayStatics::HasOption(Options, TEXT("FMRoom"));
	if (bPlayerRoom) bStartPlayersAsSpectators = true;
}

void AFMGameMode::InitGameState()
{
	Super::InitGameState();
	if (auto* State = GetGameState<AFMGameState>(); bPlayerRoom && State)
	{
		State->InitializePlayerRoom(UGameplayStatics::HasOption(OptionsString, TEXT("FMSteamRoom")));
	}
}

FString AFMGameMode::GetRoomAdmissionError(APlayerController* PendingPlayer)
{
	if (!bPlayerRoom) return {};
	if (bRoomStarted) return TEXT("FIELDMAKER_ROOM_STARTED");
	int32 Occupied = 0;
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		if (*It != PendingPlayer && It->PlayerState) ++Occupied;
	}
	return Occupied >= AFMGameState::RoomCapacity ? TEXT("FIELDMAKER_ROOM_FULL") : FString();
}

void AFMGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId,
	FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
	if (ErrorMessage.IsEmpty()) ErrorMessage = GetRoomAdmissionError();
}

FString AFMGameMode::InitNewPlayer(APlayerController* Player, const FUniqueNetIdRepl& UniqueId,
	const FString& Options, const FString& Portal)
{
	// A connection can finish its handshake after the room filled.
	const FString AdmissionError = GetRoomAdmissionError(Player);
	if (!AdmissionError.IsEmpty()) return AdmissionError;
	// Super::InitNewPlayer selects the start spot, so team assignment must precede it.
	AssignTeam(Player);
	return Super::InitNewPlayer(Player, UniqueId, Options, Portal);
}

void AFMGameMode::AssignTeam(APlayerController* Player)
{
	auto* NewState = Player ? Player->GetPlayerState<AFMPlayerState>() : nullptr;
	if (!NewState || !GameState)
	{
		return;
	}

	int32 Counts[2] = {};
	for (const APlayerState* Member : GameState->PlayerArray)
	{
		const auto* State = Cast<AFMPlayerState>(Member);
		if (State && State != NewState && State->GetTeam() < AFMPlayerState::UnassignedTeam)
		{
			++Counts[State->GetTeam()];
		}
	}
	NewState->SetTeam(Counts[0] <= Counts[1] ? 0 : 1);
}

void AFMGameMode::StartPlay()
{
	Super::StartPlay();
	RefreshLobby();
}

void AFMGameMode::PostLogin(APlayerController* Player)
{
	Super::PostLogin(Player);
	RefreshLobby();
}

void AFMGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	RefreshLobby(Exiting);
}

void AFMGameMode::RefreshLobby(AController* Excluding)
{
	auto* State = GetGameState<AFMGameState>();
	if ((!bTeamMatch && !bPlayerRoom) || !State || bRoomStarted) return;
	int32 Counts[2] = {};
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		const auto* Player = It->GetPlayerState<AFMPlayerState>();
		if (*It != Excluding && Player && Player->GetTeam() < 2) ++Counts[Player->GetTeam()];
	}
	State->SetLobbyReady(Counts[0] > 0 && Counts[1] > 0);
	if (bCombatSandbox && !bPlayerRoom) State->SetPhase(EFMMatchPhase::CombatTest);
}

bool AFMGameMode::StartPlayerRoom(APlayerController* Requester)
{
	auto* State = GetGameState<AFMGameState>();
	if (!bPlayerRoom || bRoomStarted || !State || !Requester || !Requester->IsLocalController() ||
		State->GetMatchSnapshot().Phase != EFMMatchPhase::Ready || !GetTeamStart(GetWorld(), 0) || !GetTeamStart(GetWorld(), 1))
	{
		return false;
	}

	bRoomStarted = true;
	bStartPlayersAsSpectators = false;
	ReturnPlayersToSpawn(true, true);
	SetRoundPhase(EFMMatchPhase::Deploying, 2.f);
	GetWorldTimerManager().SetTimer(RoundTimer, this, &AFMGameMode::UpdateRound, 0.1f, true);
	return true;
}

void AFMGameMode::ReturnPlayersToSpawn(bool bResetRound, bool bResetEquipment)
{
	constexpr float SpawnSpacing = 135.f;
	int32 SpawnedPerTeam[2] = {};
	for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
	{
		auto* Player = *It;
		const auto* Member = Player->GetPlayerState<AFMPlayerState>();
		if (!Member || Member->GetTeam() >= 2) continue;
		const auto* State = GetGameState<AFMGameState>();
		const uint8 Side = Member->GetTeam() ^ uint8(State->GetMatchSnapshot().bSidesSwapped);
		if (const AActor* Start = GetTeamStart(GetWorld(), Side))
		{
			const int32 Slot = SpawnedPerTeam[Member->GetTeam()]++;
			// Fill the center first and alternate left/right so short-handed teams stay centered.
			const int32 Lane = (Slot + 1) / 2 * (Slot % 2 == 0 ? 1 : -1);
			FTransform Spawn = Start->GetActorTransform();
			Spawn.AddToTranslation(Start->GetActorRightVector() * (Lane * SpawnSpacing));
			if (auto* Pawn = Cast<AFMCharacter>(Player->GetPawn()))
			{
				Pawn->GetCharacterMovement()->StopMovementImmediately();
				Pawn->SetActorLocationAndRotation(Spawn.GetLocation(), Spawn.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
				Player->SetControlRotation(Spawn.Rotator());
			}
			else RestartPlayerAtTransform(Player, Spawn);
		}
		else
		{
			RestartPlayer(Player);
		}
		if (auto* Pawn = Cast<AFMCharacter>(Player->GetPawn()))
		{
			if (bResetRound) Pawn->ResetForRound(bResetEquipment);
			Player->SetViewTarget(Pawn);
		}
	}
}

bool AFMGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	return (!bPlayerRoom || bRoomStarted) && Super::PlayerCanRestart_Implementation(Player);
}

AActor* AFMGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	const auto* State = Player ? Player->GetPlayerState<AFMPlayerState>() : nullptr;
	if ((bTeamMatch || bPlayerRoom) && State && State->GetTeam() < 2)
	{
		const auto* Match = GetGameState<AFMGameState>();
		const bool bSwap = Match && Match->GetMatchSnapshot().bSidesSwapped;
		const int32 SpawnSide = State->GetTeam() ^ int32(bSwap);
		const FName Tag(*FString::Printf(TEXT("FM_Team%d"), SpawnSide));
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			if (It->PlayerStartTag == Tag) return *It;
		}
		UE_LOG(LogTemp, Warning, TEXT("FIELDMAKER: Missing team start %s"), *Tag.ToString());
	}
	return Super::ChoosePlayerStart_Implementation(Player);
}

const AActor* AFMGameMode::GetTeamStart(const UWorld* World, uint8 Side)
{
	const FName Tag(*FString::Printf(TEXT("FM_Team%d"), Side));
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		if (It->PlayerStartTag == Tag) return *It;
	}
	return nullptr;
}

bool AFMGameMode::IsInPreparationZone(const AController* Player, const FVector& Location, bool bSpawnOnly)
{
	const auto* Member = Player ? Player->GetPlayerState<AFMPlayerState>() : nullptr;
	const auto* State = Player ? Player->GetWorld()->GetGameState<AFMGameState>() : nullptr;
	if (!Member || Member->GetTeam() >= 2 || !State) return false;
	const uint8 Side = Member->GetTeam() ^ uint8(State->GetMatchSnapshot().bSidesSwapped);
	const auto* Start = GetTeamStart(Player->GetWorld(), Side);
	const auto* Other = GetTeamStart(Player->GetWorld(), Side ^ 1);
	if (!Start || !Other) return false;
	const FVector Origin = Start->GetActorLocation();
	if (bSpawnOnly) return FVector::DistSquared2D(Location, Origin) <= FMath::Square(550.f);
	const FVector Axis = (Other->GetActorLocation() - Origin).GetSafeNormal2D();
	return FVector::DotProduct(Location - (Origin + Other->GetActorLocation()) * 0.5f, Axis) < -70.f;
}

void AFMGameMode::SetRoundPhase(EFMMatchPhase Phase, float Duration)
{
	GetGameState<AFMGameState>()->SetPhase(Phase, Duration);
	const bool bLocked = Phase == EFMMatchPhase::Deploying || Phase == EFMMatchPhase::Countdown || Phase == EFMMatchPhase::Result
		|| Phase == EFMMatchPhase::SideSwap || Phase == EFMMatchPhase::Finished;
	for (TActorIterator<AFMCharacter> It(GetWorld()); It; ++It)
	{
		if (It->GetHealth() <= 0.f) continue;
		if (bLocked) It->GetCharacterMovement()->DisableMovement();
		else if (It->GetCharacterMovement()->MovementMode == MOVE_None)
			It->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void AFMGameMode::ClearStructures()
{
	for (TActorIterator<AFMStructure> It(GetWorld()); It; ++It) It->Destroy();
}

void AFMGameMode::EndRound(int32 Winner, EFMResultReason Reason)
{
	auto* State = GetGameState<AFMGameState>();
	if (!State) return;
	const EFMMatchPhase CurrentPhase = State->GetMatchSnapshot().Phase;
	if (CurrentPhase != EFMMatchPhase::KnifeCombat && CurrentPhase != EFMMatchPhase::Combat) return;
	const bool bKnife = CurrentPhase == EFMMatchPhase::KnifeCombat;
	if (!State->RecordRoundResult(Winner, Reason)) return;
	for (APlayerState* Player : State->PlayerArray)
	{
		if (auto* Member = Cast<AFMPlayerState>(Player))
		{
			if (bKnife) Member->ResetEconomy(5000);
			else Member->AddCredits(Member->GetTeam() == Winner ? 4000 : 2500);
		}
	}
	SetRoundPhase(EFMMatchPhase::Result, 10.f);
}

#if WITH_DEV_AUTOMATION_TESTS
void AFMGameMode::VerifyRoundResultIdempotency(UWorld* World)
{
	auto* Mode = World ? World->GetAuthGameMode<AFMGameMode>() : nullptr;
	auto* State = World ? World->GetGameState<AFMGameState>() : nullptr;
	if (!Mode || !State)
	{
		UE_LOG(LogTemp, Error, TEXT("FM round result idempotency verification failed: authoritative match classes unavailable"));
		return;
	}

	// Start from a fresh round even when this development command is repeated in one PIE session.
	State->AdvanceRound(false);
	const FFMMatchSnapshot BeforeFirstRound = State->GetMatchSnapshot();
	TMap<AFMPlayerState*, int32> CreditsBefore;
	for (APlayerState* Player : State->PlayerArray)
		if (auto* Member = Cast<AFMPlayerState>(Player)) CreditsBefore.Add(Member, Member->GetCredits());

	State->SetPhase(EFMMatchPhase::Combat, 30.f);
	Mode->EndRound(0, EFMResultReason::Elimination);
	const bool bDuplicateStateResultAccepted = State->RecordRoundResult(1, EFMResultReason::DoubleElimination);
	Mode->EndRound(1, EFMResultReason::DoubleElimination);
	const FFMMatchSnapshot AfterFirstRound = State->GetMatchSnapshot();
	bool bFirstRoundPassed = AfterFirstRound.Team0Score == BeforeFirstRound.Team0Score + 1 &&
		AfterFirstRound.Team1Score == BeforeFirstRound.Team1Score &&
		AfterFirstRound.RoundWinner == 0 && AfterFirstRound.ResultReason == EFMResultReason::Elimination &&
		AfterFirstRound.bRoundResultRecorded && AfterFirstRound.Phase == EFMMatchPhase::Result && !bDuplicateStateResultAccepted;
	for (const TPair<AFMPlayerState*, int32>& Pair : CreditsBefore)
	{
		const int32 Reward = Pair.Key->GetTeam() == 0 ? 4000 : 2500;
		bFirstRoundPassed &= Pair.Key->GetCredits() == FMath::Min(12000, Pair.Value + Reward);
	}
	if (!bFirstRoundPassed)
	{
		UE_LOG(LogTemp, Error, TEXT("FM round result idempotency verification failed: duplicate first-round result changed score, reason, phase, or credits"));
		return;
	}

	State->AdvanceRound(false);
	State->SetPhase(EFMMatchPhase::Combat, 30.f);
	const FFMMatchSnapshot BeforeSecondRound = State->GetMatchSnapshot();
	CreditsBefore.Reset();
	for (APlayerState* Player : State->PlayerArray)
		if (auto* Member = Cast<AFMPlayerState>(Player)) CreditsBefore.Add(Member, Member->GetCredits());
	Mode->EndRound(1, EFMResultReason::Elimination);
	const bool bSecondDuplicateStateResultAccepted = State->RecordRoundResult(0, EFMResultReason::DoubleElimination);
	Mode->EndRound(0, EFMResultReason::DoubleElimination);
	const FFMMatchSnapshot AfterSecondRound = State->GetMatchSnapshot();
	bool bSecondRoundPassed = AfterSecondRound.Team0Score == BeforeSecondRound.Team0Score &&
		AfterSecondRound.Team1Score == BeforeSecondRound.Team1Score + 1 &&
		AfterSecondRound.RoundWinner == 1 && AfterSecondRound.ResultReason == EFMResultReason::Elimination &&
		AfterSecondRound.bRoundResultRecorded && AfterSecondRound.Phase == EFMMatchPhase::Result && !bSecondDuplicateStateResultAccepted;
	for (const TPair<AFMPlayerState*, int32>& Pair : CreditsBefore)
	{
		const int32 Reward = Pair.Key->GetTeam() == 1 ? 4000 : 2500;
		bSecondRoundPassed &= Pair.Key->GetCredits() == FMath::Min(12000, Pair.Value + Reward);
	}
	if (bSecondRoundPassed)
	{
		UE_LOG(LogTemp, Display, TEXT("FM_ROUND_RESULT_IDEMPOTENCY_PASS: duplicate finalization blocked; next-round guard reset"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FM_ROUND_RESULT_IDEMPOTENCY_FAIL: next-round result was not independently recorded exactly once"));
	}
}

namespace
{
FAutoConsoleCommandWithWorld VerifyRoundResultIdempotencyCommand(
	TEXT("fm.Match.VerifyRoundResultIdempotency"),
	TEXT("Development verification for one-time server round result, score, reward, and round reset"),
	FConsoleCommandWithWorldDelegate::CreateStatic(&AFMGameMode::VerifyRoundResultIdempotency));
}
#endif

void AFMGameMode::UpdateRound()
{
	auto* State = GetGameState<AFMGameState>();
	const auto Match = State->GetMatchSnapshot();
	const double Now = State->GetServerWorldTimeSeconds();
	const double Delta = FMath::Max(0.0, Now - LastRoundUpdate);
	LastRoundUpdate = Now;
	const bool bExpired = State->GetPhaseTimeRemaining() <= 0.f;
	const bool bKnife = Match.Phase == EFMMatchPhase::KnifeCombat;
	const bool bFighting = bKnife || Match.Phase == EFMMatchPhase::Combat;
	int32 Members[2] = {}, Alive[2] = {};
	float Health[2] = {};
	AFMCharacter* Living[2] = {};
	for (APlayerState* Player : State->PlayerArray)
	{
		const auto* Member = Cast<AFMPlayerState>(Player);
		if (!Member || Member->GetTeam() >= 2) continue;
		const uint8 Team = Member->GetTeam();
		++Members[Team];
		auto* Pawn = Cast<AFMCharacter>(Member->GetPawn());
		if (!Pawn || Pawn->GetHealth() <= 0.f) continue;
		++Alive[Team];
		Health[Team] += Pawn->GetHealth();
		Living[Team] = Pawn;
		if (Match.Phase == EFMMatchPhase::KnifePreparation || Match.Phase == EFMMatchPhase::Purchase || Match.Phase == EFMMatchPhase::Building)
		{
			const bool bSpawnOnly = Match.Phase != EFMMatchPhase::Building;
			if (!IsInPreparationZone(Pawn->GetController(), Pawn->GetActorLocation(), bSpawnOnly))
			{
				const uint8 Side = Team ^ uint8(Match.bSidesSwapped);
				if (const auto* Start = GetTeamStart(GetWorld(), Side))
				{
					Pawn->GetCharacterMovement()->StopMovementImmediately();
					const int32 Lane = Alive[Team] - 1;
					Pawn->TeleportTo(Start->GetActorLocation() + Start->GetActorRightVector() * (Lane - 2) * 135.f, Pawn->GetActorRotation());
				}
			}
		}
	}
	if (Members[0] == 0 || Members[1] == 0)
	{
		if (!Match.bRoundResultRecorded)
			State->RecordRoundResult(INDEX_NONE, EFMResultReason::TeamLeft);
		ClearStructures();
		SetRoundPhase(EFMMatchPhase::Finished, 0.f);
		GetWorldTimerManager().ClearTimer(RoundTimer);
		return;
	}
	if (bFighting)
	{
		for (TActorIterator<APlayerController> It(GetWorld()); It; ++It)
		{
			const auto* Member = It->GetPlayerState<AFMPlayerState>();
			const auto* Pawn = Cast<AFMCharacter>(It->GetPawn());
			if (Member && Member->GetTeam() < 2 && Pawn && Pawn->GetHealth() <= 0.f && Living[Member->GetTeam()])
				It->SetViewTarget(Living[Member->GetTeam()]);
		}
		if (!Alive[0] || !Alive[1])
		{
			const int32 Winner = Alive[0] ? 0 : Alive[1] ? 1 : INDEX_NONE;
			EndRound(Winner, Winner == INDEX_NONE ? EFMResultReason::DoubleElimination : EFMResultReason::Elimination);
			return;
		}
		if (bExpired)
		{
			int32 Winner = INDEX_NONE;
			if (bKnife && Alive[0] != Alive[1]) Winner = Alive[0] > Alive[1] ? 0 : 1;
			else if (bKnife && !FMath::IsNearlyEqual(Health[0], Health[1])) Winner = Health[0] > Health[1] ? 0 : 1;
			EndRound(Winner, EFMResultReason::TimeExpired);
			return;
		}
		if (!bKnife && State->GetPhaseTimeRemaining() <= 60.f)
		{
			const auto* A = GetTeamStart(GetWorld(), 0);
			const auto* B = GetTeamStart(GetWorld(), 1);
			if (!A || !B) return;
			const FVector Center = (A->GetActorLocation() + B->GetActorLocation()) * 0.5f;
			bool Present[2] = {};
			for (TActorIterator<AFMCharacter> It(GetWorld()); It; ++It)
			{
				const auto* Member = It->GetPlayerState<AFMPlayerState>();
				if (Member && Member->GetTeam() < 2 && It->GetHealth() > 0.f && FVector::DistSquared2D(It->GetActorLocation(), Center) <= FMath::Square(350.f))
					Present[Member->GetTeam()] = true;
			}
			if (CaptureTeam != INDEX_NONE && !Present[CaptureTeam]) { CaptureTeam = INDEX_NONE; CaptureProgress = 0; }
			if (Present[0] != Present[1])
			{
				CaptureTeam = Present[0] ? 0 : 1;
				CaptureProgress += FMath::Min(Delta, 60.0 - State->GetPhaseTimeRemaining());
				if (CaptureProgress >= 10.0) { EndRound(CaptureTeam, EFMResultReason::Capture); return; }
			}
			State->UpdateCapture(Center, CaptureTeam, FMath::FloorToInt(CaptureProgress * 10.0), Present[0] && Present[1]);
		}
		return;
	}
	if (!bExpired) return;
	switch (Match.Phase)
	{
	case EFMMatchPhase::Deploying: SetRoundPhase(EFMMatchPhase::KnifePreparation, 10.f); break;
	case EFMMatchPhase::KnifePreparation: SetRoundPhase(EFMMatchPhase::KnifeCombat, 60.f); break;
	case EFMMatchPhase::Purchase: SetRoundPhase(EFMMatchPhase::Building, 30.f); break;
	case EFMMatchPhase::Building:
		ReturnPlayersToSpawn(false, false);
		SetRoundPhase(EFMMatchPhase::Countdown, 3.f);
		break;
	case EFMMatchPhase::Countdown:
		CaptureTeam = INDEX_NONE;
		CaptureProgress = 0;
		SetRoundPhase(EFMMatchPhase::Combat, 150.f);
		break;
	case EFMMatchPhase::Result:
		if (FMath::Max(Match.Team0Score, Match.Team1Score) >= 10)
		{
			ClearStructures();
			SetRoundPhase(EFMMatchPhase::Finished, 0.f);
			GetWorldTimerManager().ClearTimer(RoundTimer);
		}
		else if (!Match.bSidesSwapped && FMath::Max(Match.Team0Score, Match.Team1Score) >= 5)
			SetRoundPhase(EFMMatchPhase::SideSwap, 5.f);
		else
		{
			State->AdvanceRound(false);
			ReturnPlayersToSpawn(true, Match.RoundNumber == 1);
			SetRoundPhase(EFMMatchPhase::Purchase, 20.f);
		}
		break;
	case EFMMatchPhase::SideSwap:
		ClearStructures();
		for (APlayerState* Player : State->PlayerArray)
			if (auto* Member = Cast<AFMPlayerState>(Player)) Member->ResetEconomy(5000);
		State->AdvanceRound(true);
		ReturnPlayersToSpawn(true, true);
		SetRoundPhase(EFMMatchPhase::Purchase, 20.f);
		break;
	default: break;
	}
}
