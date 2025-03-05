#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <optional>
#include <source_location>
#include <condition_variable>
#include <memory>
#include <filesystem>
#include <thread>
#include <mutex>
#include <type_traits>
#include <typeindex>
#include <algorithm>
#include <cstdint>
#include <chrono>

#include "SDL3/SDL.h"
#include "IE_Assert.h"
#include "Log.h"

#include "imgui.h"

#include "SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;

using namespace std::chrono_literals;