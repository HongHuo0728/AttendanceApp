#include "settings/state.h"

namespace attendance {

SettingsState& Settings() {
    static SettingsState state;
    return state;
}

} // namespace attendance
