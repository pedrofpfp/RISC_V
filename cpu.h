#ifndef CPU_H
#define CPU_H

#include <vector>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include "bus.h" // Necessário para a função run e fetch

class CPU {
public:
    uint32_t regs[32]; // Registradores de propósito geral (x0 a x31)
    uint32_t pc;       // Program Counter
    bool running;
    int cycle_count;

    // --- CSRs (Control and Status Registers) ---
    // Devem ser públicos para que a função de dump externa possa acessá-los.
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
    // --- FIM DOS CSRs ---


    CPU();
    uint32_t fetch(Bus& bus);
    void execute(uint32_t instr, Bus& bus);
    void print_registers();
    void run(Bus& bus, int max_cycles = 50000);
    void setPC(uint32_t new_pc);

    // Funções auxiliares:
    // **Corrigido:** Marcada como 'const' e movida para 'public' para o dump externo.
    const char* get_abi_name(int i) const;

    // Funções auxiliares para CSR
    uint32_t read_csr(uint32_t addr);
    void write_csr(uint32_t addr, uint32_t value);

private:
    // Opcional: Se houver funções privadas, elas permanecem aqui.
};

#endif // CPU_H
