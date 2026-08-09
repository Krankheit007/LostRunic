#include "Core/LRGameplayTags.h"

namespace LRGameplayTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateSourceInputCloseEyes, "State.Source.Input.CloseEyes", "Close-eyes input requested a state change.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateSourceInputOpenEyes, "State.Source.Input.OpenEyes", "Open-eyes input requested a state change.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateSourceDeath, "State.Source.Death", "Death requested entry to Memory.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateSourceNarrative, "State.Source.Narrative", "Narrative requested a state change.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateBlockerHidden, "State.Blocker.Hidden", "State input is blocked while hiding.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateBlockerDialogue, "State.Blocker.Dialogue", "State input is blocked by dialogue.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateBlockerMenu, "State.Blocker.Menu", "State input is blocked by a menu.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateBlockerTransition, "State.Blocker.Transition", "State input is blocked by level transition.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateBlockerPresentation, "State.Blocker.Presentation", "State input is blocked until presentation completes.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateBlockerDeath, "State.Blocker.Death", "State input is blocked by death processing.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateRejectInvalidTransition, "State.Reject.InvalidTransition", "The requested transition is not legal.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateRejectBlocked, "State.Reject.Blocked", "A gameplay blocker rejected the request.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateRejectConcurrentInput, "State.Reject.ConcurrentInput", "Another eye input owns the current press.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateRejectPresentationLocked, "State.Reject.PresentationLocked", "Presentation has not completed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(StateRejectAlreadyCurrent, "State.Reject.AlreadyCurrent", "The target mode is already active.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionActionInteract, "Interaction.Action.Interact", "Generic interaction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionActionPickup, "Interaction.Action.Pickup", "Pickup interaction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionActionRead, "Interaction.Action.Read", "Reading interaction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionActionTalk, "Interaction.Action.Talk", "Dialogue interaction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionActionUse, "Interaction.Action.Use", "Item-use interaction.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionActionHide, "Interaction.Action.Hide", "Enter or exit a hide spot.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionRejectNoTarget, "Interaction.Reject.NoTarget", "No valid target exists.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionRejectTooFar, "Interaction.Reject.TooFar", "The target is outside execution distance.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionRejectWrongFacing, "Interaction.Reject.WrongFacing", "The target is outside the facing cone.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionRejectOccluded, "Interaction.Reject.Occluded", "World geometry occludes the target.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionRejectState, "Interaction.Reject.State", "The current player state is incompatible.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionRejectItem, "Interaction.Reject.Item", "The selected item is incompatible.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(InteractionRejectCompleted, "Interaction.Reject.Completed", "The one-shot interaction already completed.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemCategoryKey, "Item.Category.Key", "Key item category.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemCategoryCourageWeapon, "Item.Category.CourageWeapon", "Non-lethal Courage weapon category.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemUseRejectNotOwned, "Item.Use.Reject.NotOwned", "The inventory does not own the requested item.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemUseRejectInvalidSlot, "Item.Use.Reject.InvalidSlot", "The source quick slot is invalid or empty.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemUseRejectCooldown, "Item.Use.Reject.Cooldown", "The item use is on cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemUseRejectImmune, "Item.Use.Reject.Immune", "The target is immune to this item.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemUseRejectTarget, "Item.Use.Reject.Target", "The target cannot receive item use.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(ItemUseRejectExecution, "Item.Use.Reject.Execution", "The target rejected item execution.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseFootstepWalk, "Noise.Footstep.Walk", "Walking footstep stimulus.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseFootstepRun, "Noise.Footstep.Run", "Running footstep stimulus.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NoiseInteraction, "Noise.Interaction", "Interaction-created noise stimulus.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SightPlayer, "Sight.Player", "A guard saw the player.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SightPlayerLost, "Sight.Player.Lost", "A guard lost confirmed sight of the player.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SearchReached, "Search.Reached", "A guard reached the latest disturbance location.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SearchAlertDecay, "Search.AlertDecay", "Alert decayed after its observation delay.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SearchTimeout, "Search.Timeout", "A guard search timed out.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(AIEventAlertChanged, "AI.Event.AlertChanged", "Alert state changed and StateTree should reselect.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(NarrativeEventCompleted, "Narrative.Event.Completed", "A stable narrative event completed.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SavePolicyAutoOnComplete, "Save.Policy.AutoOnComplete", "Completion requests a debounced autosave.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SavePolicyCritical, "Save.Policy.Critical", "Completion requests an ordered critical save.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TargetDoorHomeKey, "Target.Door.HomeKey", "The Home key door target.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TargetGuardCourageVulnerable, "Target.Guard.CourageVulnerable", "Guard accepts Courage knockback.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TargetGuardCourageImmune, "Target.Guard.CourageImmune", "Guard rejects Courage knockback.");
}
