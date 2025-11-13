#include "bus.h"
#include <iostream>
#include <iomanip>

Bus::Bus(MainRAM* ram, VRAM* vram, Peripherals* peripherals)
    : ram(ram), vram(vram), peripherals(peripherals)
{
    std::cout << "[Bus] Barramento conectado aos componentes de hardware.\n";
}

// ============================================================
//  LER UM BYTE DO ENDEREÇO GLOBAL
// ============================================================
uint8_t Bus::readByte(uint32_t addr) {

    // --- LÓGICA DE ROTEAMENTO CRÍTICA ---
    // Periféricos (o intervalo mais específico) devem ser checados PRIMEIRO.
    if (addr >= PERIPHERALS_START && addr <= PERIPHERALS_END) {
        uint32_t local = addr - PERIPHERALS_START;
        return peripherals->readByte(local);
    }
    // RAM Principal (o intervalo maior) é checado DEPOIS.
    else if (addr >= MAIN_RAM_START && addr <= MAIN_RAM_END) {
        uint32_t local = addr - MAIN_RAM_START;
        return ram->readByte(local);
    }
    // VRAM (não usada)
    else if (addr >= VRAM_START && addr <= VRAM_END) {
        return 0; // VRAM é um stub
    }
    else {
        std::cerr << "[Bus] ERRO: Leitura de endereço inválido 0x"
                  << std::hex << addr << std::dec << std::endl;
        return 0;
    }
}

// ============================================================
//  ESCREVER UM BYTE NO ENDEREÇO GLOBAL
// ============================================================
void Bus::writeByte(uint32_t addr, uint8_t data) {

    // --- LÓGICA DE ROTEAMENTO CRÍTICA ---
    // Periféricos (o intervalo mais específico) devem ser checados PRIMEIRO.
    if (addr >= PERIPHERALS_START && addr <= PERIPHERALS_END) {
        uint32_t local = addr - PERIPHERALS_START;
        peripherals->writeByte(local, data);
    }
    // RAM Principal (o intervalo maior) é checado DEPOIS.
    else if (addr >= MAIN_RAM_START && addr <= MAIN_RAM_END) {
        uint32_t local = addr - MAIN_RAM_START;
        ram->writeByte(local, data);
    }
    // VRAM (não usada)
    else if (addr >= VRAM_START && addr <= VRAM_END) {
        // VRAM é um stub, não faz nada
    }
    else {
        std::cerr << "[Bus] ERRO: Escrita em endereço inválido 0x"
                  << std::hex << addr << std::dec << std::endl;
    }
}

// ============================================================
//  LEITURA DE PALAVRA (32 bits) EM LITTLE-ENDIAN
// ============================================================
uint32_t Bus::readWord(uint32_t addr) {
    // Esta lógica está correta, pois chama readByte() 4x
    uint32_t value = 0;
    value |= readByte(addr);
    value |= readByte(addr + 1) << 8;
    value |= readByte(addr + 2) << 16;
    value |= readByte(addr + 3) << 24;
    return value;
}

// ============================================================
//  ESCRITA DE PALAVRA (32 bits) EM LITTLE-ENDIAN
// ============================================================
void Bus::writeWord(uint32_t addr, uint32_t data) {
    // Esta lógica está correta, pois chama writeByte() 4x
    writeByte(addr, data & 0xFF);
    writeByte(addr + 1, (data >> 8) & 0xFF);
    writeByte(addr + 2, (data >> 16) & 0xFF);
    writeByte(addr + 3, (data >> 24) & 0xFF);
}
