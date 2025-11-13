#include "cpu.h"
#include <iostream>
#include <iomanip>
#include <string> // Pode manter, mas não será usado

// ============================================================
//  CONSTRUTOR – Inicializa todos os registradores
// ============================================================
CPU::CPU()
{
    for (int i = 0; i < 32; ++i) regs[i] = 0;
    regs[0] = 0;

    // MUDANÇA: Voltar para o endereço de compliance
    pc = 0x80000000;

    // Inicializa todos os CSRs
    mtvec = 0;
    mcause = 0;
    mstatus = 0;
    mepc = 0;
    mie = 0;
    medeleg = 0;
    mideleg = 0;
    pmpaddr0 = 0;
    pmpcfg0 = 0;
    satp = 0;
    mhartid = 0; // Somos sempre o core 0

    running = true;
    cycle_count = 0;
    std::cout << "[CPU] CPU inicializada (Modo Compliance, PC=0x80000000)\n";
}

void CPU::setPC(uint32_t new_pc) {
    pc = new_pc;
    std::cout << "[CPU] PC ajustado manualmente para 0x"
              << std::hex << std::setw(8) << std::setfill('0') << pc << std::dec << std::endl;
}

void CPU::run(Bus& bus, int max_cycles)
{
    running = true;
    cycle_count = 0;
    std::cout << "---[ INÍCIO DA EXECUÇÃO RISC-V (Compliance) ]---\n";

    // MUDANÇA: O loop agora checa o 'simulation_should_halt'
    // que é controlado pelo periférico 'tohost'
    while (running && cycle_count < max_cycles && !bus.peripherals->simulation_should_halt)
    {
        uint32_t instr = fetch(bus);
        execute(instr, bus);
        cycle_count++;
        regs[0] = 0;
    }

    if (cycle_count >= max_cycles)
        std::cout << ">>> RESULTADO: TIMEOUT! (Limite de " << max_cycles << " ciclos atingido)\n";

    std::cout << "---[ FIM DA EXECUÇÃO RISC-V ]---\n";
}

// ============================================================
//  FUNÇÕES AUXILIARES DE CSR (NOVAS)
// ============================================================
uint32_t CPU::read_csr(uint32_t addr) {
    switch(addr) {
        case 0x300: return mstatus;
        case 0x302: return medeleg;
        case 0x303: return mideleg;
        case 0x304: return mie;
        case 0x305: return mtvec;
        case 0x341: return mepc;
        case 0x342: return mcause;
        case 0x180: return satp;
        case 0x3a0: return pmpcfg0;
        case 0x3b0: return pmpaddr0;
        case 0xF14: return mhartid;
        default: return 0; // Ignora outros
    }
}

void CPU::write_csr(uint32_t addr, uint32_t value) {
     switch(addr) {
        case 0x300: mstatus = value; break;
        case 0x302: medeleg = value; break;
        case 0x303: mideleg = value; break;
        case 0x304: mie = value; break;
        case 0x305: mtvec = value; break;
        case 0x341: mepc = value; break;
        case 0x342: mcause = value; break;
        case 0x180: satp = value; break;
        case 0x3a0: pmpcfg0 = value; break;
        case 0x3b0: pmpaddr0 = value; break;
        default: break; // Ignora outros
    }
}


// ... (Funções Fetch, print_registers, get_abi_name não mudam) ...
// ... (Copie-as do seu arquivo existente) ...
uint32_t CPU::fetch(Bus& bus)
{
    uint32_t instr = bus.readWord(pc);
    pc += 4;
    return instr;
}

// ============================================================
//  EXECUTE – Decodifica e executa (via Barramento)
// ============================================================
void CPU::execute(uint32_t instr, Bus& bus)
{
    uint32_t opcode = instr & 0x7F;
    uint32_t rd     = (instr >> 7)  & 0x1F;
    uint32_t funct3 = (instr >> 12) & 0x7;
    uint32_t rs1    = (instr >> 15) & 0x1F;
    uint32_t rs2    = (instr >> 20) & 0x1F;
    uint32_t funct7 = (instr >> 25) & 0x7F;

    switch (opcode)
    {

    // ... (Cases 0x13, 0x33, 0x37, 0x17, 0x03, 0x23, 0x63, 0x6F, 0x67, 0x0F)
    // ... (Copie todos os cases de instruções normais do seu cpu.cpp existente)
    // ... (Eles estão corretos para este teste) ...

    // ========================================================
    //  TIPO I – Operações imediatas (ADDI, ANDI, ORI, etc.)
    // ========================================================
    case 0x13:
    {
        int32_t imm = (int32_t)(instr & 0xFFF00000) >> 20;
        switch (funct3) {
        case 0x0: regs[rd] = regs[rs1] + imm; break; // ADDI
        case 0x2: regs[rd] = ((int32_t)regs[rs1] < imm); break; // SLTI
        case 0x3: regs[rd] = (regs[rs1] < (uint32_t)imm); break; // SLTIU
        case 0x4: regs[rd] = regs[rs1] ^ imm; break; // XORI
        case 0x6: regs[rd] = regs[rs1] | imm; break; // ORI
        case 0x7: regs[rd] = regs[rs1] & imm; break; // ANDI
        case 0x1: regs[rd] = regs[rs1] << (imm & 0x1F); break; // SLLI
        case 0x5: // SRLI / SRAI
            if (funct7 == 0x00) regs[rd] = regs[rs1] >> (imm & 0x1F); // SRLI
            else if (funct7 == 0x20) regs[rd] = (int32_t)regs[rs1] >> (imm & 0x1F); // SRAI
            break;
        default: running = false; break;
        }
        break;
    }
    // ========================================================
    //  TIPO R – Operações entre registradores (ADD, SUB, AND…)
    // ========================================================
    case 0x33:
    {
        switch (funct3) {
        case 0x0: // ADD / SUB
            if (funct7 == 0x20) regs[rd] = regs[rs1] - regs[rs2]; // SUB
            else regs[rd] = regs[rs1] + regs[rs2]; // ADD
            break;
        case 0x1: regs[rd] = regs[rs1] << (regs[rs2] & 0x1F); break; // SLL
        case 0x2: regs[rd] = ((int32_t)regs[rs1] < (int32_t)regs[rs2]); break; // SLT
        case 0x3: regs[rd] = (regs[rs1] < regs[rs2]); break; // SLTU
        case 0x4: regs[rd] = regs[rs1] ^ regs[rs2]; break; // XOR
        case 0x5: // SRL / SRA
            if (funct7 == 0x00) regs[rd] = regs[rs1] >> (regs[rs2] & 0x1F); // SRL
            else if (funct7 == 0x20) regs[rd] = (int32_t)regs[rs1] >> (regs[rs2] & 0x1F); // SRA
            break;
        case 0x6: regs[rd] = regs[rs1] | regs[rs2]; break; // OR
        case 0x7: regs[rd] = regs[rs1] & regs[rs2]; break; // AND
        default: running = false; break;
        }
        break;
    }
    // ========================================================
    //  TIPO U – LUI / AUIPC
    // ========================================================
    case 0x37: regs[rd] = (instr & 0xFFFFF000); break; // LUI
    case 0x17: regs[rd] = (pc - 4) + (instr & 0xFFFFF000); break; // AUIPC

    // ========================================================
    //  TIPO I (LOAD)
    // ========================================================
    case 0x03:
    {
        int32_t imm = (int32_t)instr >> 20;
        uint32_t addr = regs[rs1] + imm;
        switch (funct3) {
        case 0x0: regs[rd] = (int8_t)bus.readByte(addr); break; // LB
        case 0x1: regs[rd] = (int16_t)(bus.readByte(addr) | (bus.readByte(addr+1) << 8)); break; // LH
        case 0x2: regs[rd] = bus.readWord(addr); break; // LW
        case 0x4: regs[rd] = (uint8_t)bus.readByte(addr); break; // LBU
        case 0x5: regs[rd] = (uint16_t)(bus.readByte(addr) | (bus.readByte(addr+1) << 8)); break; // LHU
        default: running = false; break;
        }
        break;
    }
    // ========================================================
    //  TIPO S (STORE)
    // ========================================================
// ========================================================
    //  TIPO S (STORE)
    // ========================================================
// ========================================================
    //  TIPO S (STORE)
    // ========================================================
// ========================================================
    //  TIPO S (STORE)
    // ========================================================
    case 0x23:
    {
        // ========================================================
        //  <<<--- ADICIONE ESTA LINHA DE DEBUG AQUI ---<<<
        // ========================================================
        std::cout << "DEBUG: STORE instr\n";

        // --- CORREÇÃO DO BUG ---
        // O cálculo do imediato S-type estava incorreto no seu arquivo original.
        // A lógica correta é extrair os bits, combiná-los e DEPOIS estender o sinal.

        uint32_t imm_11_5 = (instr >> 25) & 0x7F; // Extrai bits 11:5 (instr[31:25])
        uint32_t imm_4_0  = (instr >> 7)  & 0x1F; // Extrai bits 4:0 (instr[11:7])
        int32_t imm = (imm_11_5 << 5) | imm_4_0;

        // Estende o sinal manualmente do bit 11 (0x800)
        if (imm & 0x800)
            imm |= 0xFFFFF000;

        // O resto do seu código (switch/case) estava correto
        uint32_t addr = regs[rs1] + imm;
        switch (funct3) {
        case 0x0: bus.writeByte(addr, regs[rs2] & 0xFF); break; // SB
        case 0x1: bus.writeByte(addr, (regs[rs2] >> 0) & 0xFF); bus.writeByte(addr+1, (regs[rs2] >> 8) & 0xFF); break; // SH
        case 0x2: bus.writeWord(addr, regs[rs2]); break; // SW
        default: running = false; break;
        }
        break;
    }
    //  TIPO B – Desvios condicionais
    // ========================================================
    case 0x63:
    {
        int32_t imm = (((int32_t)instr >> 31) << 12) |
                      (((instr >> 7) & 0x1) << 11) |
                      (((instr >> 25) & 0x3F) << 5) |
                      (((instr >> 8) & 0xF) << 1);
        if (imm & 0x1000) imm |= 0xFFFFE000;
        bool take = false;
        switch (funct3) {
        case 0x0: take = (regs[rs1] == regs[rs2]); break; // BEQ
        case 0x1: take = (regs[rs1] != regs[rs2]); break; // BNE
        case 0x4: take = ((int32_t)regs[rs1] < (int32_t)regs[rs2]); break; // BLT
        case 0x5: take = ((int32_t)regs[rs1] >= (int32_t)regs[rs2]); break; // BGE
        case 0x6: take = (regs[rs1] < regs[rs2]); break; // BLTU
        case 0x7: take = (regs[rs1] >= regs[rs2]); break; // BGEU
        default: running = false; break;
        }
        if (take) pc = (pc - 4) + imm;
        break;
    }
    // ========================================================
    //  JAL / JALR
    // ========================================================
    case 0x6F: // JAL
    {
        int32_t imm = (((int32_t)instr >> 31) << 20) |
                      (((instr >> 12) & 0xFF) << 12) |
                      (((instr >> 20) & 0x1) << 11) |
                      (((instr >> 21) & 0x3FF) << 1);
        if (imm & 0x100000) imm |= 0xFFE00000;
        regs[rd] = pc;
        pc = (pc - 4) + imm;
        break;
    }
    case 0x67: // JALR
    {
        int32_t imm = (int32_t)instr >> 20;
        uint32_t target = (regs[rs1] + imm) & ~1;
        regs[rd] = pc;
        pc = target;
        break;
    }
    // ========================================================
    //  FENCE
    // ========================================================
    case 0x0F:
        break; // NOP para nosso simulador

    // ========================================================
    //  SISTEMA (ECALL / EBREAK / CSR) – (MUDANÇA RADICAL)
    // ========================================================
    case 0x73:
    {
        uint32_t csr_addr = (instr >> 20) & 0xFFF;

        if (funct3 == 0) {
            // ECALL / EBREAK / MRET
            if (instr == 0x00000073) { // ECALL
                mcause = 11; // Causa: Environment call from M-mode
                mepc = pc - 4; // Salva o PC da instrução ECALL
                pc = mtvec;    // Pula para o trap handler
            } else if (instr == 0x30200073) { // MRET
                // (Simplificado: não restaura mstatus.MIE)
                pc = mepc; // Retorna do trap
            } else {
                running = false; // EBREAK ou outro
            }
        }
        else {
            // Instruções CSR
            uint32_t imm = (instr >> 15) & 0x1F;
            uint32_t old_val = read_csr(csr_addr);

            switch (funct3) {
                case 0x1: // CSRRW (Atomic Read/Write CSR)
                    regs[rd] = old_val;
                    write_csr(csr_addr, regs[rs1]);
                    break;
                case 0x2: // CSRRS (Atomic Read and Set bits)
                    regs[rd] = old_val;
                    write_csr(csr_addr, old_val | regs[rs1]);
                    break;
                case 0x3: // CSRRC (Atomic Read and Clear bits)
                    regs[rd] = old_val;
                    write_csr(csr_addr, old_val & ~regs[rs1]);
                    break;
                case 0x5: // CSRRWI (Atomic Read/Write CSR Immediate)
                    regs[rd] = old_val;
                    write_csr(csr_addr, imm);
                    break;
                case 0x6: // CSRRSI (Atomic Read and Set bits Immediate)
                    regs[rd] = old_val;
                    write_csr(csr_addr, old_val | imm);
                    break;
                case 0x7: // CSRRCI (Atomic Read and Clear bits Immediate)
                    regs[rd] = old_val;
                    write_csr(csr_addr, old_val & ~imm);
                    break;
            }
        }
        break;
    }

    // --------------------------------------------------------
    //  OPCODE DESCONHECIDO
    // --------------------------------------------------------
    default:
        std::cerr << "Opcode desconhecido: 0x" << std::hex << opcode
                  << " (em PC=0x" << (pc-4) << ")" << std::dec << "\n";
        running = false;
        break;
    }
}

// ============================================================
//  IMPRIME O ESTADO DOS REGISTRADORES
// ============================================================
void CPU::print_registers()
{
    std::cout << "--- Estado dos Registradores ---\n";
    std::cout << "PC: 0x" << std::hex << std::setw(8) << std::setfill('0') << pc << "\n";
    for (int i = 0; i < 32; ++i) {
        std::cout << "x" << i << " (" << get_abi_name(i) << "):\t0x"
                  << std::hex << std::setw(8) << std::setfill('0') << regs[i]
                  << std::dec << " (" << static_cast<int32_t>(regs[i]) << ")\n";
    }
    std::cout << "---------------------------------\n";
}

const char* CPU::get_abi_name(int i)
{
    static const char* names[] = {
        "zero","ra","sp","gp","tp","t0","t1","t2",
        "s0/fp","s1","a0","a1","a2","a3","a4","a5",
        "a6","a7","s2","s3","s4","s5","s6","s7",
        "s8","s9","s10","s11","t3","t4","t5","t6"
    };
    if (i >= 0 && i < 32) return names[i];
    return "unk";
}
