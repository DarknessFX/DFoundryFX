#pragma once
#include "CoreMinimal.h"

enum class EDFXStatCategory : uint8 {
  All,
  None,
  Common,
  Performance,
  Favorites,
  MAX
};

struct DFOUNDRYFX_API FDFXStatCmd {
  EDFXStatCategory Category;
  bool bEnabled;
  FString Command;
};