#pragma once
#include <chrono>
#include "json.hpp"
#include "logging.h"

namespace carj {
	using json = nlohmann::json;

	class ScopedTimer {
	public:
		ScopedTimer(json::reference _target) : target{_target}{
			start = std::chrono::high_resolution_clock::now();
		}

		~ScopedTimer(){
			auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::high_resolution_clock::now() - start);
			target = duration.count();
			LOG(INFO) << duration.count() << "s";
		}

	private:
		json::reference target;
		std::chrono::time_point<std::chrono::high_resolution_clock> start;
	};
}