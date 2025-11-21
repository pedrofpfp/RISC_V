#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <filesystem>
#include "cpu.h"
#include "bus.h"
#include "ram.h"

// ============================================================
// CONSTANTES GLOBAIS
// ============================================================
// Define o limite de ciclos para a CPU (Ajuste conforme necessário)
const int MAX_CYCLES = 500000;
// Define a pasta onde os relatórios serão salvos
const std::string DUMP_DIR = "FAILURE_REPORTS";

namespace fs = std::filesystem;

// ============================================================
// FUNÇÃO AUXILIAR: GERA RELATÓRIO DE FALHA (FORA DA CPU)
// ============================================================
// Recebe a CPU como 'const' e pode acessar 'get_abi_name' porque ela é 'const' agora.
void generate_failure_report(const CPU& cpu, const std::string& filename, uint32_t tohost_result, int max_cycles)
{
    std::ofstream outfile(filename);

    outfile << "================================================\n";
    outfile << "          RELATÓRIO DE FALHA RISC-V\n";
    outfile << "================================================\n";
    outfile << "Teste: " << fs::path(filename).stem().string() << ".hex\n";
    outfile << "Ciclos Executados: " << std::dec << cpu.cycle_count << " (Max: " << max_cycles << ")\n";

    // ----------------------------------------------------
    // RESULTADO FINAL E CÓDIGO DE FALHA
    // ----------------------------------------------------
    if (tohost_result == 0xFFFFFFFF) {
        outfile << "Resultado: FALHA (TIMEOUT)\n";
    } else if (tohost_result != 1) {
        outfile << "Resultado: FALHA (tohost = 0x" << std::hex << tohost_result << ")\n";
    }

    // ----------------------------------------------------
    // DIAGNÓSTICO SIMPLIFICADO
    // ----------------------------------------------------
    outfile << "\n--- DIAGNÓSTICO ---\n";
    if (tohost_result == 0xFFFFFFFF) {
        outfile << "Causa: A CPU atingiu o limite de ciclos. Indica loop infinito ou instrução que não termina.\n";
    } else if (cpu.mcause == 0xB) {
        // mcause 11 (0xB) é ECALL (Environment Call from M-mode), que é usado pelo Test Harness para reportar falha.
        outfile << "Causa: O teste executou uma instrução ECALL (Trap 0xB) para reportar uma FALHA.\n";
        outfile << "  O erro ocorreu ANTES do PC 0x" << std::hex << cpu.mepc << " (PC salvo).\n";
        outfile << "  FALHA PROVÁVEL: Falha na instrução LOAD/STORE/Cálculo que precede o trap.\n";
    } else if (tohost_result != 1) {
        outfile << "Causa: O código de erro 0x" << std::hex << tohost_result << " foi escrito diretamente no periférico 'tohost'.\n";
        outfile << "  PC de Parada: 0x" << std::hex << cpu.pc << ".\n";
    } else {
        outfile << "Causa: Desconhecida ou erro de lógica não coberto.\n";
    }

    outfile << "---------------------------------\n";

    // --- Estado da CPU (Acessa membros públicos) ---
    outfile << "\n--- ESTADO FINAL DA CPU ---\n";
    outfile << "PC: 0x" << std::hex << std::setw(8) << std::setfill('0') << cpu.pc << "\n";

    for (int i = 0; i < 32; ++i)
    {
        outfile << "x" << i << " (" << cpu.get_abi_name(i) << "):\t0x"
                  << std::hex << std::setw(8) << std::setfill('0') << cpu.regs[i]
                  << std::dec << " (" << static_cast<int32_t>(cpu.regs[i]) << ")\n";
    }

    // --- CSRs ---
    outfile << "\n--- CSRs (Control and Status Registers) ---\n";
    outfile << "mstatus: 0x" << std::hex << cpu.mstatus << "\n";
    outfile << "mepc:    0x" << std::hex << cpu.mepc << "\n";
    outfile << "mcause:  0x" << std::hex << cpu.mcause << "\n";
    outfile << "mtvec:   0x" << std::hex << cpu.mtvec << "\n";
    outfile << "mie:     0x" << std::hex << cpu.mie << "\n";
    outfile.close();

    std::cout << "\n[DUMP] Relatório de falha salvo em: " << filename << std::endl;
}


/**
 * @brief Carrega um programa na memória a partir de um arquivo .hex.
 */
void loadProgramFromHexFile(const std::string& filename, Bus& bus, uint32_t base_addr) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        std::cerr << "[Loader] ERRO: Não foi possível abrir " << filename << std::endl;
        return;
    }

    std::cout << "[Loader] Carregando " << fs::path(filename).filename().string() << "...\n";

    std::string line;
    uint32_t current_addr = base_addr;

    while (std::getline(infile, line)) {
        line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
        line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);
        if (line.empty()) continue;

        if (line[0] == '@') {
            std::stringstream ss;
            ss << std::hex << line.substr(1);
            ss >> current_addr;
            continue;
        }

        std::stringstream ss(line);
        uint32_t value;
        ss >> std::hex >> value;
        if (ss.fail()) continue;

        bus.writeWord(current_addr, value);
        current_addr += 4;
    }
    std::cout << "[Loader] Carregamento concluído.\n";
}

/**
 * @brief Executa um único teste, gera dump em caso de falha.
 */
bool run_single_test(const fs::path& hex_file_path) {
    std::cout << "--- EXECUTANDO: " << hex_file_path.filename().string() << " ---\n";

    // 1. Reinicializa todo o hardware.
    MainRAM ram;
    VRAM vram;
    Peripherals peripherals;
    Bus bus(&ram, &vram, &peripherals);
    CPU cpu;

    // 2. Carrega o programa
    loadProgramFromHexFile(hex_file_path.string(), bus, MAIN_RAM_START);

    // 3. Executa a simulação
    cpu.run(bus, MAX_CYCLES);

    // Variáveis para dump
    uint32_t final_result = 0;
    std::string dump_filename = DUMP_DIR + "/" + hex_file_path.stem().string() + ".txt";

    // 4. Verifica o resultado
    if (bus.peripherals->simulation_should_halt) {
        final_result = bus.peripherals->test_result;

        if (final_result == 1) {
            std::cout << ">>> RESULTADO: \033[1;32mPASS\033[0m\n";
            return true;
        } else {
            std::cout << ">>> RESULTADO: \033[1;31mFAIL\033[0m (tohost = 0x"
                      << std::hex << final_result << std::dec << ")\n";
            // CHAMA A FUNÇÃO EXTERNA DE DUMP EM CASO DE FALHA
            generate_failure_report(cpu, dump_filename, final_result, MAX_CYCLES);
            return false;
        }
    } else {
        final_result = 0xFFFFFFFF; // Código para indicar TIMEOUT
        std::cout << ">>> RESULTADO: \033[1;31mFAIL (TIMEOUT)\033[0m\n";
        // CHAMA A FUNÇÃO EXTERNA DE DUMP EM CASO DE TIMEOUT
        generate_failure_report(cpu, dump_filename, final_result, MAX_CYCLES);
        return false;
    }
}

// ============================================================
// Função principal
// ============================================================
int main() {

    // Define o caminho para a pasta de testes
    const std::string path_str = "TESTES HEX RISCV\\";
    fs::path test_directory(path_str);

    // Cria o diretório de dumps se não existir
    fs::create_directories(DUMP_DIR);

    int pass_count = 0;
    int fail_count = 0;

    std::cout << "================================================\n";
    std::cout << "--- Iniciando Bateria de Testes RISC-V ---\n";
    std::cout << "--- Diretório: " << path_str << " ---\n";
    std::cout << "================================================\n\n";

    // Verifica se o diretório de testes existe
    if (!fs::exists(test_directory) || !fs::is_directory(test_directory)) {
        std::cerr << "ERRO: Diretório de testes não encontrado ou inválido:\n"
                  << path_str << std::endl;
        return 1;
    }

    // Varre todos os arquivos no diretório
    for (const auto& entry : fs::directory_iterator(test_directory)) {

        // Verifica se é um arquivo regular com a extensão .hex
        if (entry.is_regular_file() && entry.path().extension() == ".hex") {

            // Executa o teste para este arquivo
            if (run_single_test(entry.path())) {
                pass_count++;
            } else {
                fail_count++;
            }
            std::cout << "---------------------------------\n\n";
        }
    }

    // Imprime o resumo final
    std::cout << "================================================\n";
    std::cout << "--- RESUMO DOS TESTES ---\n";
    std::cout << "Testes Passaram: \033[1;32m" << pass_count << "\033[0m\n";
    std::cout << "Testes Falharam: \033[1;31m" << fail_count << "\033[0m\n";
    std::cout << "Total de Testes: " << (pass_count + fail_count) << "\n";
    std::cout << "================================================\n";

    return 0;
}
