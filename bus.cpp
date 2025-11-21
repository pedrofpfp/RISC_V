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
    // Periféricos
    if (addr >= PERIPHERALS_START && addr <= PERIPHERALS_END) {
        uint32_t local = addr - PERIPHERALS_START;
        return peripherals->readByte(local);
    }
    // RAM Principal
    else if (addr >= MAIN_RAM_START && addr <= MAIN_RAM_END) {
        uint32_t local = addr - MAIN_RAM_START;
        return ram->readByte(local);
    }
    // VRAM
    else if (addr >= VRAM_START && addr <= VRAM_END) {
        return 0; // VRAM é um stub de leitura
    }
    // Endereço Inválido
    else {
        std::cerr << "[Bus] ERRO: Leitura de Byte em endereço inválido 0x"
                  << std::hex << addr << std::dec << std::endl;
        return 0;
    }
}

// ============================================================
//  ESCREVER UM BYTE NO ENDEREÇO GLOBAL
// ============================================================
void Bus::writeByte(uint32_t addr, uint8_t data) {
    // Periféricos
    if (addr >= PERIPHERALS_START && addr <= PERIPHERALS_END) {
        uint32_t local = addr - PERIPHERALS_START;
        peripherals->writeByte(local, data);
    }
    // RAM Principal
    else if (addr >= MAIN_RAM_START && addr <= MAIN_RAM_END) {
        uint32_t local = addr - MAIN_RAM_START;
        ram->writeByte(local, data);
    }
    // VRAM
    else if (addr >= VRAM_START && addr <= VRAM_END) {
        // VRAM é um stub de escrita
    }
    // Endereço Inválido
    else {
        std::cerr << "[Bus] ERRO: Escrita de Byte em endereço inválido 0x"
                  << std::hex << addr << std::dec << std::endl;
    }
}

// ============================================================
//  LEITURA DE PALAVRA (32 bits) EM LITTLE-ENDIAN (Corrigida)
// ============================================================
uint32_t Bus::readWord(uint32_t addr) {

    // Periféricos (Acesso rápido)
    if (addr >= PERIPHERALS_START && addr <= PERIPHERALS_END) {
        uint32_t local = addr - PERIPHERALS_START;
        return peripherals->readWord(local);
    }
    // RAM Principal (Montagem byte-a-byte, chamando readByte para roteamento)
    else if (addr >= MAIN_RAM_START && addr <= MAIN_RAM_END) {

        // 🛑 CRÍTICO: Verifica se o acesso de 4 bytes excede o final da RAM
        if (addr + 3 > MAIN_RAM_END) {
            std::cerr << "[Bus] ERRO DE LIMITE: Leitura de palavra 0x" << std::hex << addr
                      << " excede o fim da RAM (Max: 0x" << MAIN_RAM_END << ")!\n";
            return 0; // Retorna 0 (simulando falha de leitura ou trap)
        }

        // Little-Endian: LSB em addr
        uint32_t value = 0;
        value |= readByte(addr);
        value |= (uint32_t)readByte(addr + 1) << 8;
        value |= (uint32_t)readByte(addr + 2) << 16;
        value |= (uint32_t)readByte(addr + 3) << 24; // MSB
        return value;
    }
    // VRAM e Endereços Inválidos (Retorna 0 ou reporta erro)
    else {
        std::cerr << "[Bus] ERRO: Leitura de Palavra em endereço inválido 0x"
                  << std::hex << addr << std::dec << std::endl;
        return 0;
    }
}

// ============================================================
//  ESCRITA DE PALAVRA (32 bits) EM LITTLE-ENDIAN (Corrigida)
// ============================================================
void Bus::writeWord(uint32_t addr, uint32_t data) {

    // Periféricos (Acesso rápido)
    if (addr >= PERIPHERALS_START && addr <= PERIPHERALS_END) {
        uint32_t local = addr - PERIPHERALS_START;
        peripherals->writeWord(local, data);
    }
    // RAM Principal (Desmontagem byte-a-byte, chamando writeByte para roteamento)
    else if (addr >= MAIN_RAM_START && addr <= MAIN_RAM_END) {

        // 🛑 CRÍTICO: Verifica se o acesso de 4 bytes excede o final da RAM
        if (addr + 3 > MAIN_RAM_END) {
            std::cerr << "[Bus] ERRO DE LIMITE: Escrita de palavra 0x" << std::hex << addr
                      << " excede o fim da RAM (Max: 0x" << MAIN_RAM_END << ")!\n";
            return;
        }

        // Little-Endian: LSB em addr
        writeByte(addr, data & 0xFF);
        writeByte(addr + 1, (data >> 8) & 0xFF);
        writeByte(addr + 2, (data >> 16) & 0xFF);
        writeByte(addr + 3, (data >> 24) & 0xFF); // MSB
    }
    // VRAM e Endereços Inválidos (reporta erro)
    else {
        std::cerr << "[Bus] ERRO: Escrita de Palavra em endereço inválido 0x"
                  << std::hex << addr << std::dec << std::endl;
    }
}
