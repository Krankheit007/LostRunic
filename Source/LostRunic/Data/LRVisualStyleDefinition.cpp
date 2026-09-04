/**
 * @file LRVisualStyleDefinition.cpp
 * @brief Validates authored Perception world appearance values.
 */
#include "Data/LRVisualStyleDefinition.h"

#include "Core/LRValidation.h"

bool ULRVisualStyleDefinition::Validate(FString& outError) const
{
	return LRValidation::RequireRange(TEXT("RevealedValueFloor"), RevealedValueFloor, 0.0f, 1.0f, outError)
		&& LRValidation::RequireRange(TEXT("ValueShoulder"), ValueShoulder, 0.001f, 100.0f, outError)
		&& LRValidation::RequireRange(TEXT("ValueScale"), ValueScale, 0.0f, 100.0f, outError)
		&& LRValidation::RequireRange(TEXT("ValueBias"), ValueBias, -100.0f, 100.0f, outError)
		&& LRValidation::RequireRange(TEXT("ValueGamma"), ValueGamma, 0.001f, 10.0f, outError)
		&& LRValidation::RequireRange(TEXT("NormalColorRetention"), NormalColorRetention, 0.0f, 1.0f, outError)
		&& LRValidation::RequireRange(TEXT("ShapeLiftStrength"), ShapeLiftStrength, 0.0f, 1.0f, outError);
}
