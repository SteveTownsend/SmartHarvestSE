#pragma once

#include "CommonLibSSE/include/SKSE/Impl/PCH.h"

#include "CommonLibSSE/include/SKSE/API.h"
#include "CommonLibSSE/include/SKSE/Interfaces.h"
#include "CommonLibSSE/include/SKSE/Logger.h"
#include "CommonLibSSE/include/SKSE/RegistrationSet.h"

#include "CommonLibSSE/include/RE/Skyrim.h"

#include "nlohmann/json.hpp"
#include "nlohmann/json-schema.hpp"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "Utilities/RecursiveLock.h"
#include "Utilities/LogWrapper.h"
#include "Looting/ObjectType.h"
#include "Utilities/Enums.h"
