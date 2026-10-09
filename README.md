# EvilTwin-Esp32

***Equipe: Saulo Rafael, Luiz Filipe***

---

## ⚠️ Aviso ético e legal

Este é um **projeto acadêmico** desenvolvido para a disciplina de Algoritmos e Estrutura de Dados (CESAR School), com finalidade **exclusivamente educacional**.

Todos os testes são realizados em **ambiente de laboratório controlado**, contra redes e dispositivos **da própria equipe**, com dados fictícios. Nenhuma credencial ou informação real é coletada ou armazenada.

Executar ataques de desautenticação, Evil Twin ou captura de tráfego contra redes ou pessoas sem autorização é **crime** no Brasil (Lei 12.737/2012 e Marco Civil da Internet). O objetivo deste projeto é compreender o ataque para saber defender-se dele.

---

## Sobre o projeto

O projeto EvilTwin-Esp32 consiste no desenvolvimento de um dispositivo embarcado de IoT baseado no ESP32 cuja proposta é simular um ataque do tipo Evil Twin, no qual o ESP32 cria um ponto de acesso falso com características semelhantes às de uma rede legítima e executa um ataque de desautenticação forçada contra o AP original.

Dessa forma, os dispositivos conectados à rede verdadeira são induzidos a perder a conexão e, consequentemente, tendem a se reconectar ao ponto de acesso falso criado pelo ESP32. Em vez de atuar por interferência física de radiofrequência, o sistema utiliza a injeção de quadros de gerenciamento forjados para provocar a negação de serviço local no AP legítimo e redirecionar a conexão para o AP controlado pelo projeto.

Além disso, o EvilTwin-Esp32 também realiza a captura de tráfego e de informações de conexão no ambiente controlado, funcionando como um sniffer em modo promíscuo. Como o dispositivo não possui interface física dedicada, o projeto implementa um servidor web embutido no próprio ESP32, acessível pelo navegador, que permite visualizar os dados capturados, acompanhar os logs e aplicar filtros de busca semelhantes ao funcionamento do comando "localizar" do navegador.

## Arquitetura em fases (execução sequencial)

As três etapas **não rodam ao mesmo tempo**: são estados de rádio que entrariam em conflito (a desautenticação, por exemplo, derrubaria o próprio AP falso, já que mira no SSID). O sistema funciona de forma **linear**, uma fase de cada vez:

1. **Deauth** — injeção de quadros de desautenticação contra o AP legítimo, forçando os clientes a cair.
2. **Evil Twin** — subida do ponto de acesso falso clonando as características da rede original, para onde os clientes tendem a se reconectar.
3. **Captura / sniffer** — monitoramento do tráfego no ambiente controlado, com os dados expostos pelo servidor web embarcado (logs + filtro de busca).

Essa linearidade também facilita a leitura do código: cada fase é um módulo isolado e relativamente simples de implementar e testar por si só, o que permite ir entregando o projeto por partes.

## Hardware

- ESP32 WROOM
- (opcional) Display OLED SSD1306 para status
- Protoboard e jumpers

## Software

- VS Code + PlatformIO
- Arduino Framework para ESP32
