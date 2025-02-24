# Projeto: Cronômetro com BitDogLab

## Autor
Rodrigo Nunes Sampaio Ribeiro  
Contato: rnsribeiro@gmail.com

## Video de Demonstração
<a href="https://www.youtube.com/watch?v=kfQC4YPR4o0" target="_blank">
  <img src="https://img.youtube.com/vi/kfQC4YPR4o0/0.jpg" alt="Demonstração BitDogLab">
</a>

## Descrição
Este projeto cria um cronômetro interativo utilizando a placa **BitDogLab** baseada no Raspberry Pi Pico W RP2040. O cronômetro é controlado por dois botões físicos (A e B), exibe o tempo decorrido no formato "HH:MM:SS" em um display OLED SSD1306, e representa os valores de segundos, minutos e horas em sistema binário usando uma matriz de LEDs WS2812 5x5. Além disso, inclui feedback sonoro por meio de dois buzzers, emitindo tons distintos para cada botão pressionado.

## Funcionalidades
- **Início do Cronômetro:** Pressione o botão A (GP5) para iniciar o cronômetro; um som de 1000 Hz é emitido pelo buzzer 1 (GP21).  
- **Pausa/Continuação:** Pressione o botão A novamente para pausar ou continuar o cronômetro; o mesmo som de 1000 Hz é emitido. Durante a pausa, o OLED alterna entre "Paused" e o tempo pausado, e os LEDs mantêm o último estado.  
- **Reset:** Pressione o botão B (GP6) para pausar e entrar no modo de reset, emitindo um som de 500 Hz pelo buzzer 2 (GP10). Pressione B novamente para zerar o cronômetro e apagar os LEDs (som de 500 Hz); pressione A para cancelar o reset e continuar (som de 1000 Hz).  
- **Exibição do Tempo:**  
  - O display OLED (128x64) mostra o tempo em formato "HH:MM:SS".  
  - A matriz de LEDs WS2812 exibe:  
    - Segundos em verde (6 LEDs).  
    - Minutos em azul (6 LEDs).  
    - Horas em vermelho (5 LEDs).  
- **Feedback Sonoro:**  
  - Botão A: 1000 Hz por 100 ms (buzzer 1).  
  - Botão B: 500 Hz por 100 ms (buzzer 2).  
- **Reinício Automático:** Após 32 horas, o cronômetro reinicia automaticamente para "00:00:00".

## Tecnologias Utilizadas
- **Hardware:**  
  - Placa **BitDogLab** com Raspberry Pi Pico W RP2040.  
  - Display OLED SSD1306 (128x64, I2C em GP14/GP15).  
  - Matriz de LEDs WS2812 5x5 (GP7).  
  - Botões físicos (GP5 para botão A, GP6 para botão B).  
  - Buzzers (GP21 para buzzer 1, GP10 para buzzer 2).  
- **Software:**  
  - Programação em **C**.  
  - Bibliotecas do Pico SDK:  
    - `pico_stdlib`, `hardware_pio`, `hardware_clocks`, `hardware_i2c`, `hardware_pwm`.  
  - Biblioteca customizada para OLED: `ssd1306.h`, `ssd1306_i2c.h`, `ssd1306_i2c.c`, `ssd1306_font.h`.  
  - Programa PIO para LEDs WS2812: `ws2818b.pio`.

## Estrutura do Código
- **main.c:** Lógica principal do cronômetro, controle dos botões, exibição no OLED e LEDs, e sons nos buzzers.  
- **inc/ssd1306_*.h/.c:** Biblioteca para controle do display OLED SSD1306.  
- **ws2818b.pio:** Programa PIO para controlar os LEDs WS2812.  
- **CMakeLists.txt:** Configuração para compilação do projeto com o Pico SDK.

## Como Usar
1. **Conectar a Placa:** Certifique-se de que a placa BitDogLab está conectada ao computador via USB.  
2. **Compilar e Carregar:**  
   - Certifique-se de ter o Pico SDK instalado e `PICO_SDK_PATH` configurado (ex.: `/home/rod/pico-sdk`).  
   - Crie um diretório `build`, configure e compile:  
     ```bash
     mkdir build
     cd build
     cmake ..
     make
     ```
   - Carregue o arquivo `.uf2` gerado (ex.: `chronometer_project.uf2`) na placa via USB.  
3. **Operação:**  
   - Ao ligar, o OLED alterna entre "Press A to start" e "00:00:00", LEDs apagados.  
   - Pressione o botão A (GP5) para iniciar o cronômetro (som de 1000 Hz).  
   - Pressione A novamente para pausar (som de 1000 Hz); LEDs ficam acesos, OLED alterna "Paused" e o tempo.  
   - Pressione o botão B (GP6) para entrar no modo de reset (som de 500 Hz); OLED mostra "Press B to reset".  
   - Pressione B novamente para zerar e apagar os LEDs (som de 500 Hz), ou A para continuar (som de 1000 Hz).  

## Requisitos
- **Pico SDK:** Versão 1.5.1 ou superior.  
- **Hardware:** Placa BitDogLab com os pinos configurados como descrito.  
- **Ferramentas:** CMake, GCC ARM (ex.: `arm-none-eabi-gcc`), e um terminal para compilação.

## Futuras Melhorias
- **Tempos de Volta (Laps):** Adicionar funcionalidade para salvar tempos intermediários com um botão adicional.  
- **Formato Configurável:** Permitir alternar entre formatos de exibição (ex.: "MM:SS" ou "HH:MM").  
- **Aplicativo Externo:** Implementar comunicação via USB ou Wi-Fi (Pico W) para registro de dados em um aplicativo.  
- **Ajuste de Tons:** Permitir personalização das frequências dos buzzers via botões ou configuração.  
- **Fonte OLED:** Adicionar suporte a fontes maiores ou personalizadas no display OLED.



## Licença
Este projeto é de código aberto e pode ser modificado ou distribuído livremente, desde que atribuído crédito ao autor original.
