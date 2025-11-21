#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <iostream>
#include "ram.h" // Contém MainRAM, VRAM e Peripherals

/**
 * @class Bus
 * @brief Gerencia o roteamento de leitura/escrita entre a CPU e os
 * diferentes componentes de memória (MainRAM, VRAM, Periféricos).
 */
class Bus {
public:
    Bus(MainRAM* ram, VRAM* vram, Peripherals* peripherals);

    uint8_t   readByte(uint32_t addr);
    void      writeByte(uint32_t addr, uint8_t data);
    uint32_t  readWord(uint32_t addr);
    void      writeWord(uint32_t addr, uint32_t data);

    // ====================================================================
    //  Permite que a CPU acesse o módulo de Periféricos para checar 'tohost'
    // ====================================================================
    Peripherals* peripherals;

private:
    MainRAM* ram;
    VRAM* vram;
    // Peripherals* peripherals; // Mantido em 'public'
};

#endif // BUS_H
