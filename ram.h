#ifndef RAM_H
#define RAM_H

#include <vector>
#include <cstdint>
#include <iostream>

// --- Definições do Mapa de Memória (ATUALIZADO PARA TESTES DE COMPLIANCE) ---
const uint32_t MAIN_RAM_START = 0x80000000;
const uint32_t MAIN_RAM_SIZE  = 0x80000;    // 512 KB
const uint32_t MAIN_RAM_END   = MAIN_RAM_START + MAIN_RAM_SIZE - 1;

// Não precisamos de VRAM para este teste
const uint32_t VRAM_START = 0x00000; // Irrelevante
const uint32_t VRAM_SIZE  = 0x0;
const uint32_t VRAM_END   = 0x0;

// Periféricos (tohost) agora em 0x80001000, como o teste espera
const uint32_t PERIPHERALS_START = 0x80001000;
const uint32_t PERIPHERALS_SIZE  = 0x1000;     // 4KB
const uint32_t PERIPHERALS_END   = PERIPHERALS_START + PERIPHERALS_SIZE - 1;
// -------------------------------------


/**
 * @class MainRAM
 */
class MainRAM {
public:
    MainRAM();
    uint8_t readByte(uint32_t local_addr);
    void writeByte(uint32_t local_addr, uint8_t data);
private:
    std::vector<uint8_t> memory;
};

/**
 * @class VRAM
 */
class VRAM {
public:
    VRAM() { /* Vazio */ }
    void dumpHexToTerminal(int i) { /* Vazio */ }
};

/**
 * @class Peripherals
 */
class Peripherals {
public:
    const static uint32_t LOCAL_TOHOST_ADDR = 0x0;

    bool simulation_should_halt;
    uint32_t test_result;

Peripherals();
    uint8_t readByte(uint32_t local_addr);
    void writeByte(uint32_t local_addr, uint8_t data);

    // ⬇️ ADICIONE ESTAS DUAS LINHAS ⬇️
    uint32_t readWord(uint32_t local_addr);
    void writeWord(uint32_t local_addr, uint32_t data);

private:
    uint32_t tohost_word;
};

#endif // RAM_H
