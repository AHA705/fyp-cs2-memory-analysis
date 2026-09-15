#pragma once
#include <cstdint>
#include <vector>

/**
 * @brief Base interface for reading memory from different sources (Virtual vs Physical).
 */
class MemoryInterface {
public:
    virtual ~MemoryInterface() = default;

    /**
     * @brief Reads a specific size of memory from a given address.
     * @param address The address to read from (could be virtual or physical depending on implementation).
     * @param buffer The buffer to store the read data.
     * @param size The number of bytes to read.
     * @return true if the read was successful, false otherwise.
     */
    virtual bool Read(uintptr_t address, void* buffer, size_t size) = 0;

    /**
     * @brief Helper template for reading objects.
     */
    template <typename T>
    bool ReadObject(uintptr_t address, T& out) {
        return Read(address, &out, sizeof(T));
    }
};
