#include "filament_sensors_handler.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa/MMU2/mmu2_fsensor.h"

namespace MMU2 {

FilamentState WhereIsFilament() {
    return FSensors_instance().WhereIsFilament();
}

BlockRunoutRAII::BlockRunoutRAII() {
    FSensors_instance().IncEvLock();
}

BlockRunoutRAII::~BlockRunoutRAII() {
    FSensors_instance().DecEvLock();
}

} // namespace MMU2
