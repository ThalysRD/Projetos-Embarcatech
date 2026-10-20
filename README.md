# Projetos EmbarcaTech

Coleção de exercícios de sistemas embarcados em C, desenvolvidos durante a formação EmbarcaTech. O programa é uma residência educacional remunerada, sem vínculo empregatício.

## Projetos

| Pasta | Conteúdo |
|---|---|
| `Jogo da memória` | Jogo com joystick, botões, display OLED, matriz de LEDs e integração de rede |
| `Menu interativo` | Navegação e interação com periféricos da placa |
| `Sinal de trânsito para pedestres` | Simulação de sinalização e acionamento para pedestres |
| `Sinal de trânsito simples` | Exemplo de semáforo com diagrama para simulação Wokwi |

## Hardware e ferramentas

Os projetos com CMake selecionam `pico_w` e usam Raspberry Pi Pico SDK, CMake e toolchain ARM. Os arquivos gerados citam Pico SDK 2.1.0. Os pinos, periféricos e recursos PIO/I2C/ADC estão definidos em cada fonte; confira a compatibilidade com sua placa antes de gravar firmware.

## Compilar um projeto

Configure o Pico SDK e o toolchain, de preferência pela extensão Raspberry Pi Pico do VS Code. Exemplo com o menu:

```sh
cmake -S "Menu interativo" -B build/menu -DPICO_BOARD=pico_w
cmake --build build/menu
```

A importação do SDK depende da instalação local. O exemplo de semáforo simples usa estrutura diferente, com `diagram.json` e referência Wokwi, sem o mesmo projeto CMake.

## Configuração e cuidados

O jogo contém configurações de Wi-Fi e ThingSpeak diretamente no código. Antes de executar ou reutilizar, substitua-as por valores próprios; credenciais publicadas devem ser revogadas/trocadas e retiradas do histórico em uma revisão específica. Não use dados de terceiros presentes em cópias antigas.

## Estado da documentação

Esta coleção demonstra aprendizado em periféricos e comunicação embarcada. A compilação, montagem elétrica e funcionamento em placa não foram validados nesta revisão. Não há métricas de desempenho ou suíte automatizada declaradas aqui.
