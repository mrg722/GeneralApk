#pragma once

namespace district_fury::core {

enum class ApplicationState {
    Running,
    ExitRequested,
};

[[nodiscard]] bool shouldContinue(ApplicationState state);

}  // namespace district_fury::core
