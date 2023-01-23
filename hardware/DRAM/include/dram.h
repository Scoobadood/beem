#ifndef M6502_INCLUDE_MEMORY_H_
#define M6502_INCLUDE_MEMORY_H_

#include "bus.h"

#include <vector>

class DRAM {
public:
    DRAM(uint32_t sz, uint16_t bus_address);

    /**
     * Load the memory from binary data in the specified file.
     * @return true if it's successful or else logs an error message and returns false.
     */
    bool load( const std::string& file_name );

    void tick(const std::shared_ptr<Bus> &bus);

    /**
     * Peek memory: Convenience method; used for tooling, not emulation.
     * addr is assumed to be a bus address.
     */
    uint8_t at_bus(uint16_t addr) const;

    /**
     * Return const ref to underlying memory
     */
    inline std::shared_ptr<std::vector<uint8_t>> data() {
        return memory_;
    }

private:
    uint16_t bus_address_;
    std::shared_ptr<std::vector<uint8_t>> memory_;
};

#endif //M6502_INCLUDE_MEMORY_H_
