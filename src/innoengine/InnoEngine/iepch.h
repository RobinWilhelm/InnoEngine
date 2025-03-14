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
#include <iostream>
#include <fstream>

#include "SDL3/SDL.h"
#include "InnoEngine/IE_Assert.h"
#include "InnoEngine/Log.h"
#include "InnoEngine/CoreAPI.h"

#include "imgui.h"

#include "DirectXMath.h"
#include "Directxtk/SimpleMath.h"
namespace DXSM = DirectX::SimpleMath;