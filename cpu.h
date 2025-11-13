#ifndef CPU_H
#define CPU_H

#include <vector>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include "bus.h"
class CPU {
public:
    uint32_t regs[32];
    uint32_t pc;
    bool running;
    int cycle_count;

    // --- NOVOS CSRs ---
    uint32_t mtvec;    // Endereço do Trap Handler
    uint32_t mcause;   // Causa da Trap
    uint32_t mstatus;  // Status da Máquina
    uint32_t mepc;     // PC da exceção
    uint32_t mie;      // Interrupções Habilitadas (Machine)
    uint32_t medeleg;  // Delegação de Exceção
    uint32_t mideleg;  // Delegação de Interrupção
    uint32_t pmpaddr0; // Configuração de Proteção de Memória
    uint32_t pmpcfg0;  // Configuração de Proteção de Memória
    uint32_t satp;     // Page Table Base Address
    uint32_t mhartid;  // ID do Core (sempre 0 para nós)
    // --- FIM DOS NOVOS CSRs ---


    CPU();
    uint32_t fetch(Bus& bus);
    void execute(uint32_t instr, Bus& bus);
    void print_registers();
    void run(Bus& bus, int max_cycles = 50000);
    void setPC(uint32_t new_pc);

private:
    const char* get_abi_name(int i);

    // Funções auxiliares para CSR
    uint32_t read_csr(uint32_t addr);
    void write_csr(uint32_t addr, uint32_t value);
};
#endif
