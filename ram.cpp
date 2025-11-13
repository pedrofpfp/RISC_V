#include "ram.h"
#include <iostream>
#include <iomanip>

// ============================================================
//  MAIN RAM
// ============================================================
MainRAM::MainRAM() {
    memory.resize(MAIN_RAM_SIZE, 0);
    std::cout << "[RAM] Módulo MainRAM (" << (MAIN_RAM_SIZE / 1024)
              << " KB) criado.\n";
}

uint8_t MainRAM::readByte(uint32_t local_addr) {
    if (local_addr < memory.size())
        return memory[local_addr];
    std::cerr << "[MainRAM] ERRO: Leitura fora dos limites (0x"
              << std::hex << local_addr << ")\n";
    return 0;
}

void MainRAM::writeByte(uint32_t local_addr, uint8_t data) {
    if (local_addr < memory.size())
        memory[local_addr] = data;
    else
        std::cerr << "[MainRAM] ERRO: Escrita fora dos limites (0x"
                  << std::hex << local_addr << ")\n";
}

// ============================================================
//  VRAM
// ============================================================
//
// --- CORREÇÃO ---
// Todas as implementações de VRAM foram removidas daqui.
// O arquivo ram.h já define VRAM() e dumpHexToTerminal()
// como funções "inline" vazias, que é o correto, pois a VRAM
// não é usada neste projeto.
//
// ============================================================


// ============================================================
//  PERIFÉRICOS
// ============================================================
Peripherals::Peripherals()
    : simulation_should_halt(false),
      test_result(0),
      tohost_word(0)
{
    std::cout << "[E/S] Módulo de Periféricos (1 KB) criado.\n";
}

uint8_t Peripherals::readByte(uint32_t local_addr) {
    if (local_addr >= LOCAL_TOHOST_ADDR && local_addr <= LOCAL_TOHOST_ADDR + 3) {
        uint32_t byte_offset = local_addr - LOCAL_TOHOST_ADDR;
        uint32_t shift = byte_offset * 8;
        return static_cast<uint8_t>((tohost_word >> shift) & 0xFF);
    }
    return 0;
}

void Peripherals::writeByte(uint32_t local_addr, uint8_t data) {
    if (local_addr >= LOCAL_TOHOST_ADDR && local_addr <= LOCAL_TOHOST_ADDR + 3) {

        uint32_t byte_offset = local_addr - LOCAL_TOHOST_ADDR;
        uint32_t shift = byte_offset * 8;

        tohost_word = (tohost_word & ~(0xFF << shift)) | (static_cast<uint32_t>(data) << shift);

        simulation_should_halt = true;
        test_result = tohost_word;
    }
}
