#ifndef BEEB_HARDWARE_ROM_INCLUDE_H_
#define BEEB_HARDWARE_ROM_INCLUDE_H_

#include "bus.h"
#include <vector>

class Rom {
public:
    explicit Rom(uint16_t sz, uint16_t bus_addr);

    explicit Rom(const std::vector<uint8_t> &data, uint16_t bus_addr);

    explicit Rom(const std::string &file_name, uint16_t bus_addr);

    void tick(const std::shared_ptr<Bus> &bus);

    /**
     * Peek memory
     */
    uint8_t at_bus(uint16_t addr) const;

    /**
     * Return const ref to underlying memory
     */
    inline std::vector<uint8_t> data() const {
        return memory_;
    }

private:
    uint16_t bus_address_;
    std::vector<uint8_t> memory_;
};

#endif // BEEB_HARDWARE_ROM_INCLUDE_H_
