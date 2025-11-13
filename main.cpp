#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <filesystem> // ⬅️ NOVO INCLUDE (requer C++17)
#include "cpu.h"
#include "bus.h"
#include "ram.h"

// ⬅️ Define um atalho para o namespace filesystem
namespace fs = std::filesystem;

/**
 * @brief Carrega um programa na memória a partir de um arquivo .hex.
 * (Esta função está correta e não precisa de mudanças)
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
 * @brief ⬅️ NOVA FUNÇÃO: Executa um único teste.
 * Reinicializa o hardware, carrega, executa e verifica o resultado.
 * Retorna 'true' se o teste passou, 'false' se falhou.
 */
bool run_single_test(const fs::path& hex_file_path) {
    std::cout << "--- EXECUTANDO: " << hex_file_path.filename().string() << " ---\n";

    // 1. Reinicializa todo o hardware.
    // Isso é CRÍTICO para que um teste não afete o próximo.
    MainRAM ram;
    VRAM vram;
    Peripherals peripherals;
    Bus bus(&ram, &vram, &peripherals);
    CPU cpu; // CPU é resetada (PC=0x80000000)

    // 2. Carrega o programa
    // (Usa .string() para converter o 'path' em string)
    loadProgramFromHexFile(hex_file_path.string(), bus, MAIN_RAM_START);

    // 3. Executa a simulação
    cpu.run(bus);

    // 4. Verifica o resultado
    if (bus.peripherals->simulation_should_halt) {
        uint32_t result = bus.peripherals->test_result;
        if (result == 1) {
            std::cout << ">>> RESULTADO: \033[1;32mPASS\033[0m\n";
            return true;
        } else {
            std::cout << ">>> RESULTADO: \033[1;31mFAIL\033[0m (tohost = 0x"
                      << std::hex << result << std::dec << ")\n";
            return false;
        }
    } else {
        std::cout << ">>> RESULTADO: \033[1;31mFAIL (TIMEOUT)\033[0m\n";
        return false;
    }
}

// ============================================================
// Função principal (MODIFICADA)
// ============================================================
int main() {
    // ⬅️ Define o caminho para a pasta de testes
    const std::string path_str = "C:\\Users\\pedro\\Desktop\\TESTES HEX RISCV";
    fs::path test_directory(path_str);

    int pass_count = 0;
    int fail_count = 0;

    std::cout << "================================================\n";
    std::cout << "--- Iniciando Bateria de Testes RISC-V ---\n";
    std::cout << "--- Diretório: " << path_str << " ---\n";
    std::cout << "================================================\n\n";

    // Verifica se o diretório existe
    if (!fs::exists(test_directory) || !fs::is_directory(test_directory)) {
        std::cerr << "ERRO: Diretório de testes não encontrado ou inválido:\n"
                  << path_str << std::endl;
        return 1;
    }

    // ⬅️ Varre todos os arquivos no diretório
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

    // ⬅️ Imprime o resumo final
    std::cout << "================================================\n";
    std::cout << "--- RESUMO DOS TESTES ---\n";
    std::cout << "Testes Passaram: \033[1;32m" << pass_count << "\033[0m\n";
    std::cout << "Testes Falharam: \033[1;31m" << fail_count << "\033[0m\n";
    std::cout << "Total de Testes: " << (pass_count + fail_count) << "\n";
    std::cout << "================================================\n";

    return 0;
}
