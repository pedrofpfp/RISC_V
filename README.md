# Relatório de Arquitetura: Emulador RISC-V Completo

# O Fluxo de Execução (O Papel do `main.cpp`)

O `main.cpp` atua como o \"mundo real\" fora do chip. Ele orquestra a
simulação inteira através da função `run_single_test`. O fluxo é:

1.  **Instanciar Hardware:** Cria os objetos físicos: `MainRAM`, `VRAM`,
    `Peripherals`.

        // Em main.cpp -> run_single_test()
        MainRAM ram;
        VRAM vram;
        Peripherals peripherals;

2.  **Instanciar Barramento:** Cria o `Bus` e \"conecta\" os componentes
    de hardware a ele (passando os ponteiros).

        // Em main.cpp -> run_single_test()
        Bus bus(&ram, &vram, &peripherals);

3.  **Instanciar CPU:** Cria a `CPU`.

        // Em main.cpp -> run_single_test()
        CPU cpu; // CPU é resetada (PC=0x80000000)

4.  **Carregar Programa:** Chama `loadProgramFromHexFile()`. Esta função
    usa `bus.writeWord()` para escrever as instruções do arquivo `.hex`
    na `MainRAM`.

        // Em main.cpp -> run_single_test()
        loadProgramFromHexFile(hex_file_path.string(), bus, MAIN_RAM_START);

5.  **Iniciar CPU:** Chama `cpu.run(bus)`. Esta é a chamada que \"liga\"
    o processador. O controle do programa agora passa para a `CPU`.

        // Em main.cpp -> run_single_test()
        cpu.run(bus);

6.  **Verificar Resultado:** Quando `cpu.run()` retorna, significa que a
    simulação parou (seja por timeout ou por `tohost`). O `main` então
    verifica o resultado lendo `bus.peripherals->test_result`.

        // Em main.cpp -> run_single_test()
        if (bus.peripherals->simulation_should_halt) {
            uint32_t result = bus.peripherals->test_result;
            if (result == 1) {
                std::cout << ">>> RESULTADO: \033[1;32mPASS\033[0m\n";
                return true;
            } else {
                // ... FAIL
            }
        }

# O Mediador (O Papel do `bus.h/.cpp`)

O `Bus` é o componente mais crítico para a arquitetura. Ele não armazena
dados, mas atua como um **decodificador de endereço** (Address Decoder).
Sua única função é rotear as solicitações da `CPU` para o componente
correto.

-   A `CPU` não sabe o que é `MainRAM` ou `Peripherals`. Ela apenas sabe
    falar com o `Bus`.

-   Quando a `CPU` chama `bus.readWord(addr)`, o `Bus` implementa esta
    lógica (através de `readByte`):

    -   `if (addr >= PERIPHERALS_START ...)`: Envia a solicitação para
        `peripherals->readByte(local_addr)`.

    -   `else if (addr >= MAIN_RAM_START ...)`: Envia a solicitação para
        `ram->readByte(local_addr)`.

    -   `else`: Gera um erro de barramento.

    ```{=html}
    <!-- -->
    ```
        // Em bus.cpp
        uint8_t Bus::readByte(uint32_t addr) {
            // Periféricos ... devem ser checados PRIMEIRO.
            if (addr >= PERIPHERALS_START && addr <= PERIPHERALS_END) {
                uint32_t local = addr - PERIPHERALS_START;
                return peripherals->readByte(local);
            }
            // RAM Principal ... é checado DEPOIS.
            else if (addr >= MAIN_RAM_START && addr <= MAIN_RAM_END) {
                uint32_t local = addr - MAIN_RAM_START;
                return ram->readByte(local);
            }
            // ...
        }

-   Este design permite uma arquitetura limpa: a `CPU` lida com
    *lógica*, o `Bus` lida com *roteamento*, e a `RAM` lida com
    *armazenamento*.

# Os Componentes (O Papel do `ram.h/.cpp`)

Este arquivo define os *endpoints* do barramento.

-   **`MainRAM`**: É um componente \"burro\" (simple storage). É
    essencialmente um `std::vector`. Quando o `Bus` lhe pede para ler ou
    escrever num endereço *local*, ele fá-lo sem qualquer lógica
    adicional.

        // Em ram.h
        class MainRAM {
        public:
            // ...
        private:
            std::vector<uint8_t> memory;
        };

-   **`Peripherals`**: É um componente \"inteligente\" (I/O). Ele também
    armazena dados (`tohost_word`), mas tem *lógica de hardware*
    associada. A sua função `writeByte` é o mecanismo que termina a
    simulação:

    1.  O `Bus` envia uma escrita para o endereço `LOCAL_TOHOST_ADDR`
        (0x0).

    2.  `Peripherals::writeByte` recebe-a.

    3.  Ele armazena o valor em `test_result`.

    4.  **Crucialmente**, ele define `simulation_should_halt = true;`.

    ```{=html}
    <!-- -->
    ```
        // Em ram.cpp
        void Peripherals::writeByte(uint32_t local_addr, uint8_t data) {
            if (local_addr >= LOCAL_TOHOST_ADDR && local_addr <= LOCAL_TOHOST_ADDR + 3) {

                // ... (lógica para montar a palavra 'tohost_word') ...
                tohost_word = (tohost_word & ~(0xFF << shift)) | (static_cast<uint32_t>(data) << shift);

                simulation_should_halt = true;
                test_result = tohost_word;
            }
        }

# Análise Detalhada da CPU (`cpu.cpp`)

Este relatório detalha o funcionamento do seu emulador de CPU RISC-V,
explicando como cada função C++ mapeia para uma etapa lógica da operação
de um processador real.

## `CPU::CPU()` (O Construtor)

-   **Função:** Esta é a etapa de \"Power-On Reset\" (Ligar) da sua CPU.

-   `for (int i = 0; i < 32; ++i) regs[i] = 0;`: Inicializa todos os 32
    registradores (registos) de propósito geral (GPRs) para zero.

-   `regs[0] = 0;`: Embora o loop acima já o faça, esta linha (e a linha
    no final do `run()`) reforça a regra mais importante da ISA RISC-V:
    o registador `x0` (ou `zero`) é \"hardwired\" (ligado fisicamente) a
    zero.

-   `pc = 0x80000000;`: Define o *Program Counter (PC)* para o endereço
    de boot. Este endereço (0x80000000) é o local padrão onde os testes
    de compliance do RISC-V esperam que a memória do programa comece.

        // Em cpu.cpp
        CPU::CPU()
        {
            for (int i = 0; i < 32; ++i) regs[i] = 0;
            regs[0] = 0;

            // MUDANÇA: Voltar para o endereço de compliance
            pc = 0x80000000;
            // ... (inicializa CSRs) ...
            running = true;
        }

-   `mtvec = 0; ... mhartid = 0;`: Inicializa todos os *Control and
    Status Registers (CSRs)* para um estado conhecido (zero).

-   **Em Resumo:** O construtor prepara a CPU para um \"cold boot\".

## `CPU::run(Bus& bus, int max_cycles)`

-   **Função:** Este é o *Clock* principal da CPU. Ele implementa o
    ciclo *Fetch-Execute* (Busca-Execução).

-   `while (running && ... && !bus.peripherals->simulation_should_halt)`:
    Este é o \"clock tick\". O loop continua a rodar (\"tickando\")
    desde que:

    -   `running` seja `true` (nenhum erro interno fatal ocorreu).

    -   O `max_cycles` (um timeout) não tenha sido atingido.

    -   `!bus.peripherals->simulation_should_halt`: Esta é a parte mais
        importante. O seu barramento (Bus) deteta essa escrita e aciona
        essa flag. É assim que o programa de teste sinaliza \"Eu
        terminei, pode parar a simulação\".

    ```{=html}
    <!-- -->
    ```
        // Em cpu.cpp
        void CPU::run(Bus& bus, int max_cycles)
        {
            // ...
            while (running && cycle_count < max_cycles && !bus.peripherals->simulation_should_halt)
            {
                uint32_t instr = fetch(bus);
                execute(instr, bus);
                cycle_count++;
                regs[0] = 0;
            }
            // ...
        }

-   `uint32_t instr = fetch(bus);`: Esta é a *Etapa de Busca (Fetch)*.

-   `execute(instr, bus);`: Esta é a combinação das *Etapas de
    Decodificação (Decode) e Execução (Execute)*.

-   `regs[0] = 0;`: Mais uma vez, garante que `x0` permaneça zero.

## `CPU::fetch(Bus& bus)`

-   **Função:** A *Etapa de Busca (Fetch)* do pipeline.

-   `uint32_t instr = bus.readWord(pc);`: O coração do fetch. A CPU
    coloca o endereço atual do `pc` no barramento (Bus) e pede para ler
    (`readWord`) a instrução.

-   `pc += 4;`: A CPU *incrementa o PC* em 4 bytes (32 bits).

        // Em cpu.cpp
        uint32_t CPU::fetch(Bus& bus)
        {
            uint32_t instr = bus.readWord(pc);
            pc += 4;
            return instr;
        }

-   Se a instrução for um salto (branch, JAL, JALR), a função `execute`
    irá *sobrescrever* o `pc` com um novo valor.

## `CPU::execute(uint32_t instr, Bus& bus)`

-   **Função:** As *Etapas de Decodificação (Decode) e Execução
    (Execute)*.

-   Esta é a função mais complexa e o cérebro real da CPU.

### Decodificação (A \"Bancada de Registadores\")

As primeiras linhas da função são o *Decodificador de Instrução*:

    // Em cpu.cpp -> execute()
    uint32_t opcode = instr & 0x7F;
    uint32_t rd     = (instr >> 7)  & 0x1F;
    uint32_t funct3 = (instr >> 12) & 0x7;
    uint32_t rs1    = (instr >> 15) & 0x1F;
    uint32_t rs2    = (instr >> 20) & 0x1F;
    uint32_t funct7 = (instr >> 25) & 0x7F;

    switch (opcode)
    {
        // ...
    }

-   Você está a \"quebrar\" a instrução binária de 32 bits nos seus
    campos constituintes.

-   `opcode`: Diz o *tipo* da instrução (R-Type, I-Type, Load, Store,
    etc.).

-   `rd`: O índice do *registador de destino*.

-   `rs1`, `rs2`: Os índices dos *registadores fonte*.

-   `funct3`, `funct7`: Campos de função que ajudam a diferenciar
    operações.

### Execução (A \"ULA\" e a \"Unidade de Controlo\")

O `switch (opcode)` é a sua *Unidade de Controlo* principal.

#### case 0x13 (I-Type / Operações Imediatas)

-   **Decodificação:**
    `int32_t imm = (int32_t)(instr & 0xFFF00000) >> 20;` decodifica o
    imediato de 12 bits e faz a *extensão de sinal*.

-   **Execução:** O `switch(funct3)` atua como a *ULA (Unidade Lógica e
    Aritmética)*.

        // Em cpu.cpp -> execute()
        case 0x13: // I-Type
        {
            int32_t imm = (int32_t)(instr & 0xFFF00000) >> 20;
            switch (funct3) {
            case 0x0: regs[rd] = regs[rs1] + imm; break; // ADDI
            case 0x4: regs[rd] = regs[rs1] ^ imm; break; // XORI
            // ...
            }
            break;
        }

#### case 0x33 (R-Type / Operações Reg-Reg)

-   **Execução:** Similar ao I-Type, mas a operação é entre `regs[rs1]`
    e `regs[rs2]`.

-   **Detalhe Chave:** Para `funct3=0x0` (ADD/SUB) \[\...\] você precisa
    verificar o `funct7`.

        // Em cpu.cpp -> execute()
        case 0x33: // R-Type
        {
            switch (funct3) {
            case 0x0: // ADD / SUB
                if (funct7 == 0x20) regs[rd] = regs[rs1] - regs[rs2]; // SUB
                else regs[rd] = regs[rs1] + regs[rs2]; // ADD
                break;
            // ...
            }
            break;
        }

#### case 0x03 (I-Type / LOAD)

-   **Execução:** Esta é uma operação de 2 estágios:

    1.  **Cálculo de Endereço (ULA):**
        `uint32_t addr = regs[rs1] + imm;`.

    2.  **Acesso à Memória (Load Unit):** O `switch(funct3)` chama o
        barramento.

    3.  **Writeback:** O valor lido da memória é escrito em `regs[rd]`.

    ```{=html}
    <!-- -->
    ```
        // Em cpu.cpp -> execute()
        case 0x03: // Load
        {
            int32_t imm = (int32_t)instr >> 20;
            uint32_t addr = regs[rs1] + imm;
            switch (funct3) {
            case 0x0: regs[rd] = (int8_t)bus.readByte(addr); break; // LB
            case 0x2: regs[rd] = bus.readWord(addr); break; // LW
            case 0x4: regs[rd] = (uint8_t)bus.readByte(addr); break; // LBU
            // ...
            }
            break;
        }

#### case 0x23 (S-Type / STORE)

-   **Decodificação:** A decodificação do imediato S-Type é mais
    complexa. O seu código *recombina* corretamente esses dois campos.

-   **Execução:**

    1.  **Cálculo de Endereço (ULA):**
        `uint32_t addr = regs[rs1] + imm;`.

    2.  **Acesso à Memória (Store Unit):** Pega o valor de `regs[rs2]` e
        *escreve-o* na memória.

    ```{=html}
    <!-- -->
    ```
        // Em cpu.cpp -> execute()
        case 0x23: // Store
        {
            // ... CORREÇÃO DO BUG ...
            uint32_t imm_11_5 = (instr >> 25) & 0x7F; // Extrai bits 11:5
            uint32_t imm_4_0  = (instr >> 7)  & 0x1F; // Extrai bits 4:0
            int32_t imm = (imm_11_5 << 5) | imm_4_0;
            // ... (extensão de sinal) ...
            uint32_t addr = regs[rs1] + imm;
            switch (funct3) {
            case 0x0: bus.writeByte(addr, regs[rs2] & 0xFF); break; // SB
            case 0x2: bus.writeWord(addr, regs[rs2]); break; // SW
            // ...
            }
            break;
        }

#### case 0x63 (B-Type / Branch)

-   **Execução:**

#### case 0x67 (I-Type / JALR)

-   **Execução:** Similar ao JAL, mas o alvo é calculado a partir de um
    registador: `(regs[rs1] + imm)`.

#### case 0x73 (SYSTEM / CSRs)

-   Esta é a sua *Unidade de Privilégio (M-Mode)*.

-   **if (funct3 == 0) (Trap/Exceção):**

    -   **ECALL (0x00000073):** Simula uma \"system call\". O hardware
        reage:

        -   `mcause = 11;`: Define a Causa da Exceção.

        -   `mepc = pc - 4;`: Salva o endereço da instrução ECALL.

        -   `pc = mtvec;`: *Salta* para o *handler* de interrupção.

        ```{=html}
        <!-- -->
        ```
            // Em cpu.cpp -> execute() -> case 0x73
            if (funct3 == 0) {
                if (instr == 0x00000073) { // ECALL
                    mcause = 11; // Causa: Environment call from M-mode
                    mepc = pc - 4; // Salva o PC da instrução ECALL
                    pc = mtvec;    // Pula para o trap handler
                } else if (instr == 0x30200073) { // MRET
                    pc = mepc; // Retorna do trap
                }
            }

    -   **MRET (0x30200073):** Retorna da exceção.

-   **else (Operações CSR):**

    -   Lida com CSRRW, CSRRS, CSRRC e as suas variantes imediatas (I).

    -   `uint32_t old_val = read_csr(csr_addr);`: Lê o valor antigo.

    -   `regs[rd] = old_val;`: A parte \"R\" (Read) da instrução.

    -   `write_csr(...)`: A parte \"W\" (Write) da instrução.

    ```{=html}
    <!-- -->
    ```
        // Em cpu.cpp -> execute() -> case 0x73
        else {
            // Instruções CSR
            uint32_t csr_addr = (instr >> 20) & 0xFFF;
            uint32_t old_val = read_csr(csr_addr);

            switch (funct3) {
                case 0x1: // CSRRW (Atomic Read/Write CSR)
                    regs[rd] = old_val;
                    write_csr(csr_addr, regs[rs1]);
                    break;
                case 0x5: // CSRRWI (Atomic Read/Write CSR Immediate)
                    regs[rd] = old_val;
                    write_csr(csr_addr, imm);
                    break;
                // ...
            }
        }

## `read_csr()` e `write_csr()`

-   **Função:** Abstraem o \"banco de registadores CSR\".

-   O seu emulador armazena o estado dos CSRs como variáveis de membro
    (ex: `mstatus`, `mepc`).

-   O hardware real acede a esses registadores por um *endereço* (ex:
    0x300, 0x341).

-   Essas funções `switch` fazem a \"tradução\".

        // Em cpu.cpp
        uint32_t CPU::read_csr(uint32_t addr) {
            switch(addr) {
                case 0x300: return mstatus;
                case 0x305: return mtvec;
                case 0x341: return mepc;
                case 0x342: return mcause;
                // ...
                default: return 0; // Ignora outros
            }
        }

        void CPU::write_csr(uint32_t addr, uint32_t value) {
             switch(addr) {
                case 0x300: mstatus = value; break;
                case 0x305: mtvec = value; break;
                case 0x341: mepc = value; break;
                case 0x342: mcause = value; break;
                // ...
                default: break; // Ignora outros
            }
        }

## Conclusão da Análise da CPU

O seu código implementa corretamente o núcleo da ISA RV32I, incluindo o
mecanismo de trap de M-Mode e acesso a CSRs, que são as partes mais
complexas.

# O Ciclo de Vida de uma Instrução (Exemplos) {#sec:ciclo_de_vida}

Para unir tudo, veja o fluxo de dados para diferentes operações:

## Exemplo A: `fetch` (Busca de Instrução no PC `0x80000020`)

1.  **CPU (`run`):** Chama `fetch(bus)`.

2.  **CPU (`fetch`):** Chama `bus.readWord(0x80000020)`.

3.  **BUS (`readWord`):**

    -   Verifica o endereço: `0x80000020` está na faixa da `MAIN_RAM`.

    -   Calcula o endereço local: `0x80000020 - 0x80000000 = 0x20`.

    -   Chama `ram->readByte(0x20)`, `ram->readByte(0x21)`,
        `ram->readByte(0x22)`, `ram->readByte(0x23)`.

4.  **RAM (`readByte`):** Lê os 4 bytes do seu `std::vector` interno.

5.  **BUS:** Remonta os 4 bytes em um `uint32_t` e o retorna.

6.  **CPU:** Recebe a instrução e incrementa `pc += 4`.

## Exemplo B: `addi x1, x1, 5` (Operação Interna)

1.  **CPU (`execute`):**

    -   Decodifica `opcode 0x13` (I-Type).

    -   Lê `regs[x1]`.

    -   Soma `5`.

    -   Escreve o resultado em `regs[x1]`.

2.  **BUS:** **Não é usado.** A operação é inteiramente contida na
    `CPU`.

## Exemplo C: `sw x10, 0(x0)` (Store para `tohost` - Fim do Teste)

Este é o evento mais complexo e importante, assumindo que `rs1+imm`
resulte em `0x80001000` (o endereço de `tohost`).

1.  **CPU (`execute`):**

    -   Decodifica `opcode 0x23` (S-Type / STORE).

    -   Calcula o endereço de memória: (ex: `0x80001000`).

    -   Lê o valor de `regs[x10]` (o código de resultado do teste, ex:
        `1` para PASS).

    -   Chama `bus.writeWord(0x80001000, regs[x10])`.

2.  **BUS (`writeWord`):**

    -   Verifica o endereço: `0x80001000` está na faixa dos
        `PERIPHERALS`.

    -   Calcula o endereço local: `0x80001000 - 0x80001000 = 0x0`.

    -   Chama `peripherals->writeByte(0x0, ...)` 4 vezes para escrever a
        palavra.

3.  **PERIPHERALS (`writeByte`):**

    -   Recebe a escrita no endereço local `0x0`.

    -   Define `tohost_word` com o valor recebido.

    -   Define `test_result = tohost_word`.

    -   Define `simulation_should_halt = true`. : A flag é definida em
        `ram.cpp`, linha 64.

            // Em ram.cpp
            void Peripherals::writeByte(uint32_t local_addr, uint8_t data) {
                if (local_addr >= LOCAL_TOHOST_ADDR && local_addr <= LOCAL_TOHOST_ADDR + 3) {
                    // ...
                    simulation_should_halt = true;
                    test_result = tohost_word;
                }
            }

4.  **CPU (`run`):**

    -   A instrução `sw` termina.

    -   O loop `while` no `cpu.run` faz a sua próxima verificação:
        `!bus.peripherals->simulation_should_halt`.

            // Em cpu.cpp
            while (running && ... && !bus.peripherals->simulation_should_halt)

    -   Como a flag é agora `true`, o loop termina. A função `cpu.run()`
        retorna.

5.  **MAIN (`run_single_test`):**

    -   A execução retorna do `cpu.run()`.

    -   O `main` verifica `bus.peripherals->test_result` (que é `1`).

    -   Imprime \"PASS\".

# Conclusão da Arquitetura

O projeto demonstra uma arquitetura de emulador modular e robusta. A
separação de responsabilidades é clara:

-   **`CPU`** implementa a **lógica da ISA**.

-   **`Bus`** implementa o **roteamento do mapa de memória**.

-   **`RAM/Peripherals`** implementam o **comportamento do hardware** em
    endereços específicos.

Este design permite que o sistema seja facilmente expandido, por
exemplo, adicionando um novo periférico (como um timer ou UART)
simplesmente:

1.  Definindo um novo `const uint32_t` em `ram.h` para o seu endereço.

        // Exemplo de expansão (em ram.h):
        const uint32_t MAIN_RAM_START = 0x80000000;
        const uint32_t PERIPHERALS_START = 0x80001000;
        // const uint32_t NEW_DEVICE_START = 0x80002000; // <-- Adicionar aqui

2.  Adicionando um `else if` no `Bus` para rotear para ele.

        // Exemplo de expansão (em bus.cpp -> readByte/writeByte):
        if (addr >= PERIPHERALS_START && ...) {
            // ...
        }
        else if (addr >= MAIN_RAM_START && ...) {
            // ...
        }
        // else if (addr >= NEW_DEVICE_START && ...) { // <-- Adicionar aqui
        //    return new_device->readByte(local);
        // }

3.  Criando a sua classe (ex: `Timer.cpp`) e ligando-a ao construtor do
    `Bus`.
