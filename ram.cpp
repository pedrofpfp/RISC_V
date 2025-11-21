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
//  VRAM (Não usada)
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

// --- Leitura de Byte ---
// (Lê o byte correspondente da palavra 'tohost' interna - Little-Endian)
uint8_t Peripherals::readByte(uint32_t local_addr) {
    if (local_addr >= LOCAL_TOHOST_ADDR && local_addr <= LOCAL_TOHOST_ADDR + 3) {
        uint32_t byte_offset = local_addr - LOCAL_TOHOST_ADDR;
        uint32_t shift = byte_offset * 8;
        return static_cast<uint8_t>((tohost_word >> shift) & 0xFF);
    }
    return 0;
}

// --- Escrita de Byte ---
// (Monta a palavra 'tohost' interna - Little-Endian)
void Peripherals::writeByte(uint32_t local_addr, uint8_t data) {
    if (local_addr >= LOCAL_TOHOST_ADDR && local_addr <= LOCAL_TOHOST_ADDR + 3) {

        uint32_t byte_offset = local_addr - LOCAL_TOHOST_ADDR;
        uint32_t shift = byte_offset * 8;

        // Limpa o byte antigo e insere o novo byte na posição correta
        tohost_word = (tohost_word & ~(0xFF << shift)) | (static_cast<uint32_t>(data) << shift);

        // A simulação NÃO para em uma escrita de byte.
    }
}

// --- Leitura de Palavra ---
uint32_t Peripherals::readWord(uint32_t local_addr) {
    // Acesso rápido à palavra tohost
    if (local_addr == LOCAL_TOHOST_ADDR) {
        return tohost_word;
    }
    return 0;
}

// --- Escrita de Palavra ---
void Peripherals::writeWord(uint32_t local_addr, uint32_t data) {
    // Só reage se for uma escrita no endereço base do 'tohost'
    if (local_addr == LOCAL_TOHOST_ADDR) {
        tohost_word = data; // Armazena a palavra inteira

        // A LÓGICA DE PARADA (HALT) ocorre aqui (escrita de palavra).
        simulation_should_halt = true;
        test_result = tohost_word;
    }
}
