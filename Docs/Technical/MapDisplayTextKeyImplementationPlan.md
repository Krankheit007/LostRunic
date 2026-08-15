# Map Display TextKey Implementation Plan

**Goal:** Make `FLRMapRegistration.DisplayNameTextKey` the only authored source for localized map names.

**Architecture:** Remove the inline `FText DisplayName` fallback from map registration. `ULRGameContentSet::GetMapDisplayName` resolves the stable TextKey through `UIStringTable` and falls back only to `MapId` at runtime; editor validation reports missing TextKeys or missing StringTable references.

**Tech Stack:** Unreal Engine 5.8, C++, `UDataAsset`, `UStringTable`, Unreal Automation Tests.

## Tasks

1. Add a failing automation assertion that a registered map without `DisplayNameTextKey` is invalid.
2. Remove `DisplayName` from `FLRMapRegistration` and update `GetMapDisplayName` to use only `DisplayNameTextKey`.
3. Require `UIStringTable` and non-empty map TextKeys in editor validation.
4. Update the StringTable configuration guide and run `LostRunicEditor` build plus focused narrative/UI tests.

## Constraints

- Keep stable `MapId` and `DisplayNameTextKey` as `FName` values.
- Do not modify generated build output or user-authored WBP/map assets.
- Missing runtime TextKeys may fall back to `MapId` for diagnosis, but invalid authored data must be reported by Data Validation.
