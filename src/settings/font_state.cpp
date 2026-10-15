#include "settings/font_state.h"

namespace attendance {

FontsState& Fonts() {
    static FontsState state;
    return state;
}

} // namespace attendance
