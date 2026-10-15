#include "animation/state.h"

namespace attendance {

AnimationsState& Animations() {
    static AnimationsState state;
    return state;
}

} // namespace attendance
