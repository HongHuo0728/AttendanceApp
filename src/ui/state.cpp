#include "ui/state.h"

namespace attendance {

UiState& Ui() {
    static UiState state;
    return state;
}

} // namespace attendance
