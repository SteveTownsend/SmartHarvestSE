#pragma once

#include "RecursiveLock.h"

#include "CommonLibSSE/include/SKSE/Impl/PCH.h"

#include "CommonLibSSE/include/SKSE/API.h"
#include "CommonLibSSE/include/SKSE/Interfaces.h"
#include "CommonLibSSE/include/SKSE/Logger.h"
#include "CommonLibSSE/include/SKSE/RegistrationSet.h"

#include "CommonLibSSE/include/RE/Skyrim.h"
#include "LogWrapper.h"

#include "nlohmann/json.hpp"
#include "nlohmann/json-schema.hpp"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "version.h"
#include "ObjectType.h"
#include "iniSettings.h"
#include "utils.h"
#include "dataCase.h"
#include "IHasValueWeight.h"
#include "objects.h"
#include "FormHelper.h"
#include "papyrus.h"

#include "Exception.h"
#include "LoadOrder.h"
#include "Condition.h"
#include "Collection.h"
#include "CollectionFactory.h"
#include "CollectionManager.h"
