# EvilTwin-Esp32

**Equipe:** Saulo Rafael, Luiz Filipe
**Equipe:** Saulo Rafael, Luiz Filipe

## ⚠️ Aviso ético e legal

Este é um **projeto acadêmico** desenvolvido para a disciplina de Algoritmos e Estrutura de Dados (CESAR School), com finalidade **exclusivamente educacional**. Todos os testes são realizados em **ambiente de laboratório controlado**, contra redes e dispositivos **da própria equipe**, com dados fictícios. Nenhuma credencial ou informação real é coletada ou armazenada. Executar ataques de desautenticação, Evil Twin ou captura de tráfego contra redes ou pessoas sem autorização é **crime** no Brasil (Lei 12.737/2012 e Marco Civil da Internet). O objetivo deste projeto é compreender o ataque para saber defender-se dele.
Este é um **projeto acadêmico** desenvolvido para a disciplina de Algoritmos e Estrutura de Dados (CESAR School), com finalidade **exclusivamente educacional**. Todos os testes são realizados em **ambiente de laboratório controlado**, contra redes e dispositivos **da própria equipe**, com dados fictícios. Nenhuma credencial ou informação real é coletada ou armazenada. Executar ataques de desautenticação, Evil Twin ou captura de tráfego contra redes ou pessoas sem autorização é **crime** no Brasil (Lei 12.737/2012 e Marco Civil da Internet). O objetivo deste projeto é compreender o ataque para saber defender-se dele.

---

## 📖 1. Contexto e Problemática

Em redes Wi‑Fi, dispositivos clientes confiam automaticamente em pontos de acesso que apresentem o mesmo **SSID** (nome da rede) e, em muitos casos, o mesmo **BSSID** (endereço MAC do AP). Um atacante pode explorar essa confiança criando um **AP falso** que clona essas características e, ao mesmo tempo, **forçar a desautenticação** dos clientes do AP legítimo. Uma vez desconectados, os dispositivos tendem a se reconectar ao AP falso, acreditando ser a rede original.

O **EvilTwin-Esp32** reproduz esse cenário em laboratório para estudar o ataque **Man‑in‑the‑Middle (MITM)** em nível 802.11. O dispositivo não depende de interferência física de radiofrequência: ele injeta **quadros de gerenciamento forjados** para provocar a negação de serviço local no AP real e, em seguida, assume o papel de “ponte” para a rede legítima, permitindo observar e analisar o tráfego da vítima de forma controlada.

---

## ⚙️ 2. Funcionalidades do Dispositivo

- [x] **Clonagem de AP (Evil Twin):** o ESP32 cria um ponto de acesso falso com o mesmo **SSID**, **BSSID (MAC)** e **canal** da rede alvo, forçando largura de banda **HT20** para evitar instabilidades.
- [x] **Desautenticação reativa (BSSID Clone Aggressive):** um *sniffer* monitora pacotes de dados destinados ao BSSID real; quando um cliente (que ainda não está no AP falso) tenta se comunicar com o roteador legítimo, o ESP32 envia um frame **Deauth unicast** apenas para o MAC desse cliente, forçando sua reconexão.
- [x] **Conexão à rede real e NAT (repetidor):** o ESP32 conecta‑se à rede original no modo **Station (STA)** e ativa o **NAPT** via `esp_netif_napt_enable()`, fornecendo acesso real à internet para a vítima e atuando como uma ponte entre o AP falso e o AP legítimo.
- [x] **Bypass do sanity check de frames brutos:** implementação própria da função `ieee80211_raw_frame_sanity_check` para permitir a injeção de quadros de desautenticação que, de outra forma, seriam bloqueados pela pilha Wi‑Fi do ESP-IDF.
- [x] **Captura e logs via servidor web embutido:** o dispositivo mantém um servidor HTTP acessível pelo navegador para visualizar logs, dados capturados e aplicar filtros de busca.

---

## 🛠️ 3. Especificações Técnicas e Componentes

| Item | Especificação |
| :--- | :--- |
| **Microcontrolador** | ESP32 NodeMCU (WROOM) – Wi‑Fi 802.11 b/g/n (2.4 GHz) |
| **Ambiente de desenvolvimento** | VS Code + extensão PlatformIO |
| **Framework** | Arduino + ESP-IDF (necessário para o NAT/NAPT) |
| **Hardware opcional** | Display OLED SSD1306 (I²C) para status; antena externa (conector U.FL ou solda) para melhorar o alcance do sinal |
| **Linguagem** | C/C++ |

---

## 🔌 4. Diagrama de Conexões (Pinout)

> **Nota:** o núcleo do projeto é o próprio ESP32; não há sensores ou atuadores obrigatórios além do display opcional. A tabela abaixo cobre a ligação do display OLED, caso ele seja utilizado. O ataque em si não requer hardware externo, o rádio Wi‑Fi do ESP32 é o elemento central.

| Componente | Pino do Componente | Pino no ESP32 (GPIO) | Observações |
| :--- | :--- | :--- | :--- |
| Display OLED (SSD1306) | SDA | GPIO 21 | Linha I²C padrão |
| Display OLED (SSD1306) | SCL | GPIO 22 | Linha I²C padrão |
| Antena externa (opcional) | – | – | Conectar ao ponto de antena da placa (U.FL ou solda), se disponível |

*(Diagrama de blocos/fritzing pode ser adicionado futuramente, quando o hardware opcional for integrado.)*

---

## 🚀 5. Instruções de Compilação e Gravação

### 5.1 Clone o repositório

Abra o terminal (PowerShell ou Prompt de Comando) e execute:

```bash
git clone https://github.com/Saul0RS/EvilTwin-Esp32.git
cd EvilTwin-Esp32
```

> **Dica:** evite clonar em caminhos com espaços ou acentos (ex.: `C:\Users\Seu Nome\...`). O ESP‑IDF tem problemas históricos com *whitespace*. Recomendamos `C:\ESP32\EvilTwin-Esp32`.

---

### 5.2 Configure a variável de ambiente `PLATFORMIO_CORE_DIR`

Para evitar problemas de permissão e de caminho com espaços, centralize o *core* do PlatformIO em um diretório simples antes de abrir o projeto:

1. Abra o **Menu Iniciar** e digite **“variáveis de ambiente”**.
2. Clique em **“Editar as variáveis de ambiente do sistema”**.
3. Na janela **Propriedades do Sistema**, clique no botão **“Variáveis de Ambiente…”** (canto inferior direito).
4. Na seção **“Variáveis do usuário para <seu usuário>”**, clique em **“Novo…”**.
5. Preencha:
   - **Nome da variável:** `PLATFORMIO_CORE_DIR`
   - **Valor da variável:** `C:\PlatformIO`
6. Clique em **OK** em todas as janelas abertas para confirmar.
7. **Feche e reabra** o VS Code para que a variável passe a valer na sessão.

---

### 5.3 Compile e grave

1. No VS Code, abra a pasta do projeto clonado.
2. Aguarde a extensão **PlatformIO IDE** carregar o ambiente.
3. Clique no ícone de **“Build”** (✓) na barra inferior do PlatformIO ou execute no terminal integrado:
   ```bash
   pio run
   ```
4. Conecte o ESP32 via USB e envie o firmware:
   ```bash
   pio run --target upload
   ```

---
## 📖 1. Contexto e Problemática

Em redes Wi‑Fi, dispositivos clientes confiam automaticamente em pontos de acesso que apresentem o mesmo **SSID** (nome da rede) e, em muitos casos, o mesmo **BSSID** (endereço MAC do AP). Um atacante pode explorar essa confiança criando um **AP falso** que clona essas características e, ao mesmo tempo, **forçar a desautenticação** dos clientes do AP legítimo. Uma vez desconectados, os dispositivos tendem a se reconectar ao AP falso, acreditando ser a rede original.

O **EvilTwin-Esp32** reproduz esse cenário em laboratório para estudar o ataque **Man‑in‑the‑Middle (MITM)** em nível 802.11. O dispositivo não depende de interferência física de radiofrequência: ele injeta **quadros de gerenciamento forjados** para provocar a negação de serviço local no AP real e, em seguida, assume o papel de “ponte” para a rede legítima, permitindo observar e analisar o tráfego da vítima de forma controlada.

---

## ⚙️ 2. Funcionalidades do Dispositivo

- [x] **Clonagem de AP (Evil Twin):** o ESP32 cria um ponto de acesso falso com o mesmo **SSID**, **BSSID (MAC)** e **canal** da rede alvo, forçando largura de banda **HT20** para evitar instabilidades.
- [x] **Desautenticação reativa (BSSID Clone Aggressive):** um *sniffer* monitora pacotes de dados destinados ao BSSID real; quando um cliente (que ainda não está no AP falso) tenta se comunicar com o roteador legítimo, o ESP32 envia um frame **Deauth unicast** apenas para o MAC desse cliente, forçando sua reconexão.
- [x] **Conexão à rede real e NAT (repetidor):** o ESP32 conecta‑se à rede original no modo **Station (STA)** e ativa o **NAPT** via `esp_netif_napt_enable()`, fornecendo acesso real à internet para a vítima e atuando como uma ponte entre o AP falso e o AP legítimo.
- [x] **Bypass do sanity check de frames brutos:** implementação própria da função `ieee80211_raw_frame_sanity_check` para permitir a injeção de quadros de desautenticação que, de outra forma, seriam bloqueados pela pilha Wi‑Fi do ESP-IDF.
- [x] **Captura e logs via servidor web embutido:** o dispositivo mantém um servidor HTTP acessível pelo navegador para visualizar logs, dados capturados e aplicar filtros de busca.

---

## 🛠️ 3. Especificações Técnicas e Componentes

| Item | Especificação |
| :--- | :--- |
| **Microcontrolador** | ESP32 NodeMCU (WROOM) – Wi‑Fi 802.11 b/g/n (2.4 GHz) |
| **Ambiente de desenvolvimento** | VS Code + extensão PlatformIO |
| **Framework** | Arduino + ESP-IDF (necessário para o NAT/NAPT) |
| **Hardware opcional** | Display OLED SSD1306 (I²C) para status; antena externa (conector U.FL ou solda) para melhorar o alcance do sinal |
| **Linguagem** | C/C++ |

---

## 🔌 4. Diagrama de Conexões (Pinout)

> **Nota:** o núcleo do projeto é o próprio ESP32; não há sensores ou atuadores obrigatórios além do display opcional. A tabela abaixo cobre a ligação do display OLED, caso ele seja utilizado. O ataque em si não requer hardware externo, o rádio Wi‑Fi do ESP32 é o elemento central.

| Componente | Pino do Componente | Pino no ESP32 (GPIO) | Observações |
| :--- | :--- | :--- | :--- |
| Display OLED (SSD1306) | SDA | GPIO 21 | Linha I²C padrão |
| Display OLED (SSD1306) | SCL | GPIO 22 | Linha I²C padrão |
| Antena externa (opcional) | – | – | Conectar ao ponto de antena da placa (U.FL ou solda), se disponível |

*(Diagrama de blocos/fritzing pode ser adicionado futuramente, quando o hardware opcional for integrado.)*

---

## 🚀 5. Instruções de Compilação e Gravação

### 5.1 Clone o repositório

Abra o terminal (PowerShell ou Prompt de Comando) e execute:

```bash
git clone https://github.com/Saul0RS/EvilTwin-Esp32.git
cd EvilTwin-Esp32
```

> **Dica:** evite clonar em caminhos com espaços ou acentos (ex.: `C:\Users\Seu Nome\...`). O ESP‑IDF tem problemas históricos com *whitespace*. Recomendamos `C:\ESP32\EvilTwin-Esp32`.

---

### 5.2 Configure a variável de ambiente `PLATFORMIO_CORE_DIR`

Para evitar problemas de permissão e de caminho com espaços, centralize o *core* do PlatformIO em um diretório simples antes de abrir o projeto:

1. Abra o **Menu Iniciar** e digite **“variáveis de ambiente”**.
2. Clique em **“Editar as variáveis de ambiente do sistema”**.
3. Na janela **Propriedades do Sistema**, clique no botão **“Variáveis de Ambiente…”** (canto inferior direito).
4. Na seção **“Variáveis do usuário para <seu usuário>”**, clique em **“Novo…”**.
5. Preencha:
   - **Nome da variável:** `PLATFORMIO_CORE_DIR`
   - **Valor da variável:** `C:\PlatformIO`
6. Clique em **OK** em todas as janelas abertas para confirmar.
7. **Feche e reabra** o VS Code para que a variável passe a valer na sessão.

---

### 5.3 Compile e grave

1. No VS Code, abra a pasta do projeto clonado.
2. Aguarde a extensão **PlatformIO IDE** carregar o ambiente.
3. Clique no ícone de **“Build”** (✓) na barra inferior do PlatformIO ou execute no terminal integrado:
   ```bash
   pio run
   ```
4. Conecte o ESP32 via USB e envie o firmware:
   ```bash
   pio run --target upload
   ```

---

## Arquitetura em fases (execução sequencial)

As três etapas **não rodam ao mesmo tempo**: são estados de rádio que entrariam em conflito (a desautenticação, por exemplo, derrubaria o próprio AP falso, já que mira no SSID). O sistema funciona de forma **linear**, uma fase de cada vez:

1. **Deauth** — injeção de quadros de desautenticação contra o AP legítimo, forçando os clientes a cair.
2. **Evil Twin** — subida do ponto de acesso falso clonando as características da rede original, para onde os clientes tendem a se reconectar.
3. **Captura / sniffer** — monitoramento do tráfego no ambiente controlado, com os dados expostos pelo servidor web embarcado (logs + filtro de busca).

Essa linearidade também facilita a leitura do código: cada fase é um módulo isolado e relativamente simples de implementar e testar por si só, o que permite ir entregando o projeto por partes.
