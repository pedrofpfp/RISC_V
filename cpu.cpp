#include "cpu.h"
#include <iostream>
#include <iomanip>
#include <string>

// ============================================================
//  CONSTRUTOR – Inicializa todos os registradores e PC
// ============================================================
CPU::CPU()
{
    for (int i = 0; i < 32; ++i) regs[i] = 0;
    regs[0] = 0;

    // Endereço inicial padrão para testes de compliance
    pc = 0x80000000;

    // Inicializa todos os CSRs (Simplificado para M-Mode Básico)
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
    mhartid = 0;

    running = true;
    cycle_count = 0;
    std::cout << "[CPU] CPU inicializada (Modo Compliance, PC=0x80000000)\n";
}

void CPU::setPC(uint32_t new_pc)
{
    pc = new_pc;
    std::cout << "[CPU] PC ajustado manualmente para 0x"
              << std::hex << std::setw(8) << std::setfill('0') << pc << std::dec << std::endl;
}

void CPU::run(Bus& bus, int max_cycles)
{
    running = true;
    cycle_count = 0;
    std::cout << "---[ INÍCIO DA EXECUÇÃO RISC-V (Compliance) ]---\n";

    // O loop checa se a simulação deve parar via periférico 'tohost'
    while (running && cycle_count < max_cycles && !bus.peripherals->simulation_should_halt)
    {
        uint32_t instr = fetch(bus);
        execute(instr, bus);
        cycle_count++;
        regs[0] = 0; // x0 deve ser sempre 0
    }

    if (cycle_count >= max_cycles)
        std::cout << ">>> RESULTADO: TIMEOUT! (Limite de " << max_cycles << " ciclos atingido)\n";

    std::cout << "---[ FIM DA EXECUÇÃO RISC-V ]---\n";
}

// ============================================================
//  FUNÇÕES AUXILIARES DE CSR (CSRs Mínimos)
// ============================================================
uint32_t CPU::read_csr(uint32_t addr)
{
    switch(addr)
    {
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
    default: return 0;
    }
}

void CPU::write_csr(uint32_t addr, uint32_t value)
{
    switch(addr)
    {
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
    default: break;
    }
}


// ============================================================
//  FETCH (com Debug)
// ============================================================
uint32_t CPU::fetch(Bus& bus)
{
    // DEBUG: Informa o PC antes da leitura
    std::cout << "[FETCH] PC: 0x" << std::hex << pc << " (Lendo instrução)\n";

    uint32_t instr = bus.readWord(pc);

    // DEBUG: Informa a instrução lida
    std::cout << "[FETCH] Instrução lida: 0x" << std::hex << instr << "\n";

    pc += 4;
    return instr;
}

// ============================================================
//  EXECUTE – Decodifica e executa (com Logs de Debug)
// ============================================================
void CPU::execute(uint32_t instr, Bus& bus)
{
    uint32_t opcode = instr & 0x7F;
    uint32_t rd     = (instr >> 7)  & 0x1F;
    uint32_t funct3 = (instr >> 12) & 0x7;
    uint32_t rs1    = (instr >> 15) & 0x1F;
    uint32_t rs2    = (instr >> 20) & 0x1F;
    uint32_t funct7 = (instr >> 25) & 0x7F;

    // DEBUG GERAL DE INSTRUÇÃO (Em vez do log detalhado aqui, usamos o log do FETCH)
    // std::cout << "[CYCLE " << std::dec << cycle_count << "] PC: 0x" << std::hex << (pc - 4)
    //           << " | INSTR: 0x" << instr << " | Op: 0x" << opcode << std::dec;

    switch (opcode)
    {

    // ========================================================
    //  TIPO I – Operações imediatas (ADDI, SLLI, SRLI, etc.)
    // ========================================================
    case 0x13:
    {
        int32_t imm = (int32_t)(instr & 0xFFF00000) >> 20;
        std::cout << " [EXEC] ADDI/SLLI/SRLI... | rd:" << rd << ", rs1:" << rs1 << ", imm:" << imm << "\n";
        switch (funct3)
        {
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
        default:
            std::cerr << "ERRO: Funct3 desconhecido para Opcode 0x13\n";
            running = false;
            break;
        }
        break;
    }
    // ========================================================
    //  TIPO R – Operações entre registradores (ADD, SUB, AND…)
    // ========================================================
    case 0x33:
    {
        std::cout << " [EXEC] ADD/SUB/SLL... | rd:" << rd << ", rs1:" << rs1 << ", rs2:" << rs2 << ", F7:" << funct7 << "\n";
        switch (funct3)
        {
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
        default:
            std::cerr << "ERRO: Funct3 desconhecido para Opcode 0x33\n";
            running = false;
            break;
        }
        break;
    }
    // ========================================================
    //  TIPO U – LUI / AUIPC
    // ========================================================
    case 0x37:
        std::cout << " [EXEC] LUI | rd:" << rd << "\n";
        regs[rd] = (instr & 0xFFFFF000);
        break; // LUI
    case 0x17:
        std::cout << " [EXEC] AUIPC | rd:" << rd << "\n";
        regs[rd] = (pc - 4) + (instr & 0xFFFFF000);
        break; // AUIPC

// ========================================================
//  TIPO I (LOAD) - Opcode 0x03
// ========================================================
    case 0x03:
    {
        int32_t imm = (int32_t)instr >> 20;
        uint32_t addr = regs[rs1] + imm;

        // DEBUG DE LOAD
        std::cout << " [EXEC] LOAD (LB/LH/LW/LBU/LHU) | F3: 0x" << funct3 << " | Dest Addr: 0x" << std::hex << addr << "\n";

        if (rd == 0) break;

        switch (funct3)
        {
        // LB (Load Byte) - Extensão de Sinal
        case 0x0:
            regs[rd] = (int32_t)(int8_t)bus.readByte(addr);
            std::cout << "    -> LB (Sinal) | Valor lido: 0x" << std::hex << regs[rd] << "\n";
            break;

        // LH (Load Half-word) - Extensão de Sinal
        case 0x1:
        {
            uint16_t half_word_signed = (uint16_t)bus.readByte(addr) |
                                        ((uint16_t)bus.readByte(addr + 1) << 8);
            regs[rd] = (int32_t)(int16_t)half_word_signed;
            std::cout << "    -> LH (Sinal) | Valor lido: 0x" << std::hex << regs[rd] << "\n";
            break;
        }

        // LW (Load Word)
        case 0x2:
            regs[rd] = bus.readWord(addr);
            std::cout << "    -> LW | Valor lido: 0x" << std::hex << regs[rd] << "\n";
            break;

        // LBU (Load Byte Unsigned) - Extensão Zero
        case 0x4:
            regs[rd] = (uint32_t)bus.readByte(addr);
            std::cout << "    -> LBU (Zero) | Valor lido: 0x" << std::hex << regs[rd] << "\n";
            break;

        // LHU (Load Half-word Unsigned) - Extensão Zero
        case 0x5:
        {
            uint16_t half_word_unsigned = (uint16_t)bus.readByte(addr) |
                                          ((uint16_t)bus.readByte(addr + 1) << 8);
            regs[rd] = (uint32_t)half_word_unsigned;
            std::cout << "    -> LHU (Zero) | Valor lido: 0x" << std::hex << regs[rd] << "\n";
            break;
        }

        default:
            std::cerr << "ERRO: Funct3 desconhecido para Opcode 0x03\n";
            running = false;
            break;
        }
        break;
    }


// ========================================================
//  TIPO S (STORE) - Opcode 0x23
// ========================================================
    case 0x23:
    {
        // Cálculo do Imediato S-Type
        uint32_t imm_11_5 = (instr >> 25) & 0x7F;
        uint32_t imm_4_0  = (instr >> 7)  & 0x1F;
        int32_t imm = (imm_11_5 << 5) | imm_4_0;
        if (imm & 0x800) imm |= 0xFFFFF000;

        uint32_t addr = regs[rs1] + imm;

        // DEBUG DE STORE
        std::cout << " [EXEC] STORE (SB/SH/SW) | F3: 0x" << funct3 << " | Dest Addr: 0x" << std::hex << addr
                  << " | Valor RS2: 0x" << regs[rs2] << std::dec << "\n";

        switch (funct3)
        {

        // SB (Store Byte)
        case 0x0:
            bus.writeByte(addr, regs[rs2] & 0xFF);
            std::cout << "    -> SB | Escrito byte: 0x" << std::hex << (regs[rs2] & 0xFF) << "\n";
            break;

        // SH (Store Half-word) - Little-Endian
        case 0x1:
            bus.writeByte(addr, (regs[rs2] >> 0) & 0xFF);
            bus.writeByte(addr+1, (regs[rs2] >> 8) & 0xFF);
            std::cout << "    -> SH | Escrito half-word: 0x" << std::hex << (regs[rs2] & 0xFFFF) << "\n";
            break;

        // SW (Store Word)
        case 0x2:
            bus.writeWord(addr, regs[rs2]);
            std::cout << "    -> SW | Escrito word: 0x" << std::hex << regs[rs2] << "\n";
            break;

        default:
            std::cerr << "ERRO: Funct3 desconhecido para Opcode 0x23\n";
            running = false;
            break;
        }
        break;
    }


    // ========================================================
    //  TIPO B – Desvios condicionais
    // ========================================================
    case 0x63:
    {
        // Cálculo do imediato B-Type
        int32_t imm = (((int32_t)instr >> 31) << 12) |
                      (((instr >> 7) & 0x1) << 11) |
                      (((instr >> 25) & 0x3F) << 5) |
                      (((instr >> 8) & 0xF) << 1);
        if (imm & 0x1000) imm |= 0xFFFFE000;
        bool take = false;

        std::cout << " [EXEC] BRANCH (BEQ/BNE/...) | F3: 0x" << funct3 << " | RS1: 0x" << regs[rs1]
                  << " | RS2: 0x" << regs[rs2] << "\n";

        switch (funct3)
        {
        case 0x0: take = (regs[rs1] == regs[rs2]); break; // BEQ
        case 0x1: take = (regs[rs1] != regs[rs2]); break; // BNE
        case 0x4: take = ((int32_t)regs[rs1] < (int32_t)regs[rs2]); break; // BLT
        case 0x5: take = ((int32_t)regs[rs1] >= (int32_t)regs[rs2]); break; // BGE
        case 0x6: take = (regs[rs1] < regs[rs2]); break; // BLTU
        case 0x7: take = (regs[rs1] >= regs[rs2]); break; // BGEU
        default:
            std::cerr << "ERRO: Funct3 desconhecido para Opcode 0x63\n";
            running = false;
            break;
        }
        if (take) {
            pc = (pc - 4) + imm;
            std::cout << "    -> BRANCH TAKE | PC target: 0x" << std::hex << pc << "\n";
        }
        break;
    }
    // ========================================================
    //  JAL / JALR
    // ========================================================
    case 0x6F: // JAL
    {
        // Cálculo do imediato J-Type
        int32_t imm = (((int32_t)instr >> 31) << 20) |
                      (((instr >> 12) & 0xFF) << 12) |
                      (((instr >> 20) & 0x1) << 11) |
                      (((instr >> 21) & 0x3FF) << 1);
        if (imm & 0x100000) imm |= 0xFFE00000;

        std::cout << " [EXEC] JAL | rd:" << rd << " | PC target: 0x" << std::hex << ((pc - 4) + imm) << "\n";

        regs[rd] = pc;
        pc = (pc - 4) + imm;
        break;
    }
    case 0x67: // JALR
    {
        int32_t imm = (int32_t)instr >> 20;
        uint32_t target = (regs[rs1] + imm) & ~1;

        std::cout << " [EXEC] JALR | rd:" << rd << " | PC target: 0x" << std::hex << target << "\n";

        regs[rd] = pc;
        pc = target;
        break;
    }
    // ========================================================
    //  FENCE
    // ========================================================
    case 0x0F:
        std::cout << " [EXEC] FENCE | NOP\n";
        break;

    // ========================================================
    //  SISTEMA (ECALL / EBREAK / CSR)
    // ========================================================
    case 0x73:
    {
        uint32_t csr_addr = (instr >> 20) & 0xFFF;
        std::cout << " [EXEC] SYSTEM (ECALL/CSR) | F3: 0x" << funct3 << " | CSR: 0x" << csr_addr << "\n";

        if (funct3 == 0)
        {
            // ECALL / EBREAK / MRET
            if (instr == 0x00000073)   // ECALL
            {
                std::cout << "    -> ECALL | Trap para 0x" << std::hex << mtvec << "\n";
                mcause = 11;
                mepc = pc - 4;
                pc = mtvec;
            }
            else if (instr == 0x30200073)     // MRET
            {
                std::cout << "    -> MRET | Retornando para 0x" << std::hex << mepc << "\n";
                pc = mepc;
            }
            else
            {
                std::cerr << "    -> EBREAK ou instrução SYSTEM desconhecida: 0x" << std::hex << instr << "\n";
                running = false;
            }
        }
        else // Instruções CSR
        {
            uint32_t imm = (instr >> 15) & 0x1F;
            uint32_t old_val = read_csr(csr_addr);
            switch (funct3)
            {
            case 0x1: regs[rd] = old_val; write_csr(csr_addr, regs[rs1]); break;
            case 0x2: regs[rd] = old_val; write_csr(csr_addr, old_val | regs[rs1]); break;
            case 0x3: regs[rd] = old_val; write_csr(csr_addr, old_val & ~regs[rs1]); break;
            case 0x5: regs[rd] = old_val; write_csr(csr_addr, imm); break;
            case 0x6: regs[rd] = old_val; write_csr(csr_addr, old_val | imm); break;
            case 0x7: regs[rd] = old_val; write_csr(csr_addr, old_val & ~imm); break;
            }
        }
        break;
    }

    // --------------------------------------------------------
    //  OPCODE DESCONHECIDO (Captura de Erro)
    // --------------------------------------------------------
    default:
        std::cerr << ">>> ERRO FATAL: Opcode desconhecido: 0x" << std::hex << opcode
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
    for (int i = 0; i < 32; ++i)
    {
        std::cout << "x" << i << " (" << get_abi_name(i) << "):\t0x"
                  << std::hex << std::setw(8) << std::setfill('0') << regs[i]
                  << std::dec << " (" << static_cast<int32_t>(regs[i]) << ")\n";
    }
    std::cout << "---------------------------------\n";
}

const char* CPU::get_abi_name(int i) const
{
    // Mapeamento padrão RISC-V ABI
    static const char* names[] =
    {
        "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
        "s0",   "s1", "a0", "a1", "a2", "a3", "a4", "a5",
        "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7",
        "s8",   "s9", "s10","s11", "t3", "t4", "t5", "t6"
    };

    // Você pode adicionar a lógica para 'fp' se precisar do apelido
    if (i == 8) return "fp/s0"; // Ou apenas "s0"

    if (i >= 0 && i < 32) return names[i];
    return "unk";
}
