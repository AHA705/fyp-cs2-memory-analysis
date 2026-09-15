#pragma once
#include <iostream>
#include <fstream>
#include <cstdint>
#include <string>
#include <vector>
#include <windows.h>
#include "MemoryInterface.hpp"
#include "logger.hpp"

/**
 * @brief Reads physical memory from a dump file and performs the manual x64
 * Page Table Walk to translate virtual addresses (VA) into physical addresses (PA).
 */
class PhysicalMemoryReader : public MemoryInterface {
public:
    PhysicalMemoryReader(uint64_t directoryTableBase, const std::string& memoryFilePath)
        : m_cr3(directoryTableBase), m_hDevice(INVALID_HANDLE_VALUE), m_memoryFilePath(memoryFilePath) {

        m_hDevice = CreateFileA(
            m_memoryFilePath.c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
            NULL
        );

        logger->info("Attempting to open physical memory file at {}", m_memoryFilePath);

        if (m_hDevice == INVALID_HANDLE_VALUE) {
            logger->critical("Failed to open physical memory file '{}' (GetLastError={})",
                             m_memoryFilePath,
                             GetLastError());
        }
    }

    ~PhysicalMemoryReader() override {
        if (m_hDevice != INVALID_HANDLE_VALUE) {
            CloseHandle(m_hDevice);
        }
    }

    /**
     * @brief Translates a Virtual Address (VA) into a Physical Address (PA)
     * using the current CR3 (Directory Table Base).
     */
    uint64_t Translate(uintptr_t virtualAddress) {
        // x64 Virtual Address structure (4-level paging):
        // [Sign Extension: 16 bits] [PML4: 9 bits] [PDPT: 9 bits] [PD: 9 bits] [PT: 9 bits] [Offset: 12 bits]

        uint64_t pml4_index = (virtualAddress >> 39) & 0x1FF;
        uint64_t pdpt_index = (virtualAddress >> 30) & 0x1FF;
        uint64_t pd_index   = (virtualAddress >> 21) & 0x1FF;
        uint64_t pt_index   = (virtualAddress >> 12) & 0x1FF;
        uint64_t offset     = virtualAddress & 0xFFF;

        uint64_t pml4e = 0, pdpte = 0, pde = 0, pte = 0;

        uint64_t cr3_base = m_cr3 & ~0xFFFULL;

        // 1. Read PML4 entry from CR3 base
        if (!ReadPhysical(cr3_base + (pml4_index * 8), &pml4e, sizeof(pml4e))) {
            logger->warn("Failed to read PML4E for VA 0x{:X}", virtualAddress);
            return 0;
        }
        if (!(pml4e & 1)) {
            // logger->debug("PML4 entry not present for VA 0x{:X}", virtualAddress);
            return 0;
        }

        // 2. Read PDPT entry from PML4 base (bits 12-51)
        uint64_t pdpt_base = pml4e & 0x000FFFFFFFFFF000ULL;
        if (!ReadPhysical(pdpt_base + (pdpt_index * 8), &pdpte, sizeof(pdpte))) {
            logger->warn("Failed to read PDPTE for VA 0x{:X}. Trying to read PhysAddr 0x{:X}", virtualAddress, pdpt_base + (pdpt_index * 8));
            return 0;
        }
        if (!(pdpte & 1)) {
            // logger->debug("PDPT entry not present for VA 0x{:X}", virtualAddress);
            return 0;
        }
        
        // 1GB huge page check
        if (pdpte & 0x80) {
            return (pdpte & 0x000FFFFFC0000000ULL) + (virtualAddress & 0x3FFFFFFFULL);
        }

        // 3. Read PD entry from PDPT base (bits 12-51)
        uint64_t pd_base = pdpte & 0x000FFFFFFFFFF000ULL;
        if (!ReadPhysical(pd_base + (pd_index * 8), &pde, sizeof(pde))) {
            logger->warn("Failed to read PDE for VA 0x{:X}", virtualAddress);
            return 0;
        }
        if (!(pde & 1)) {
            // logger->debug("PD entry not present for VA 0x{:X}", virtualAddress);
            return 0;
        }

        // 4. Handle PS bit (Huge Page - 2MB instead of 4KB)
        if (pde & 0x80) {
            return (pde & 0x000FFFFFFFE00000ULL) + (virtualAddress & 0x1FFFFFULL);
        }

        // 5. Read PT entry from PD base (bits 12-51)
        uint64_t pt_base = pde & 0x000FFFFFFFFFF000ULL;
        if (!ReadPhysical(pt_base + (pt_index * 8), &pte, sizeof(pte))) {
            logger->warn("Failed to read PTE for VA 0x{:X}", virtualAddress);
            return 0;
        }
        if (!(pte & 1)) {
            // logger->debug("PTE not present (paged out?) for VA 0x{:X}", virtualAddress);
            return 0;
        }

        // 6. Return physical frame + offset
        return (pte & 0x000FFFFFFFFFF000ULL) + offset;
    }

    /**
     * @brief Reads virtual memory by first translating it to a physical address.
     */
    bool Read(uintptr_t address, void* buffer, size_t size) override {
        uint64_t pa = Translate(address);
        if (!pa) return false;
        return ReadPhysical(pa, buffer, size);
    }

    /**
     * @brief Directly reads from the physical memory file via Win32 API.
     */
    bool ReadPhysical(uint64_t pa, void* buffer, size_t size) {
        if (m_hDevice == INVALID_HANDLE_VALUE) return false;

        LARGE_INTEGER liDistanceToMove;
        liDistanceToMove.QuadPart = pa;

        // Set the pointer in the device
        if (!SetFilePointerEx(m_hDevice, liDistanceToMove, NULL, FILE_BEGIN)) {
            return false;
        }

        DWORD bytesRead = 0;
        if (ReadFile(m_hDevice, buffer, static_cast<DWORD>(size), &bytesRead, NULL)) {
            return bytesRead == size;
        }

        return false;
    }

    void SetDirectoryTableBase(uint64_t cr3) { m_cr3 = cr3; }

private:
    HANDLE m_hDevice;
    uint64_t m_cr3;
    std::string m_memoryFilePath;
};
