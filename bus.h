#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include "ram.h" // Contém MainRAM, VRAM e Peripherals

/**
 * @class Bus
 * @brief Gerencia o roteamento de leitura/escrita entre a CPU e os
 * diferentes componentes de memória (MainRAM, VRAM, Periféricos).
 */
class Bus {
public:
    Bus(MainRAM* ram, VRAM* vram, Peripherals* peripherals);

    uint8_t  readByte(uint32_t addr);
    void     writeByte(uint32_t addr, uint8_t data);
    uint32_t readWord(uint32_t addr);
    void     writeWord(uint32_t addr, uint32_t data);

    // ====================================================================
    //  CORREÇÃO: Movido para 'public' para que a CPU::run() possa
    //  acessar bus.peripherals->simulation_should_halt
    // ====================================================================
    Peripherals* peripherals;

private:
    MainRAM* ram;
    VRAM* vram;
    // Peripherals* peripherals; // Movido para public
};

#endif // BUS_H
