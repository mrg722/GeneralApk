#include "core/ApplicationState.h"

namespace district_fury::core {

bool shouldContinue(const ApplicationState state) {
    return state == ApplicationState::Running;
}

}  // namespace district_fury::core
