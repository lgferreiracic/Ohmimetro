# EmbarcaTech_Ohmimetro.
<p align="center">
  <img src="Group 658.png" alt="EmbarcaTech" width="300">
</p>

## Atividade: Ohmímetro

![C](https://img.shields.io/badge/c-%2300599C.svg?style=for-the-badge&logo=c&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-%23008FBA.svg?style=for-the-badge&logo=cmake&logoColor=white)
![Raspberry Pi](https://img.shields.io/badge/-Raspberry_Pi-C51A4A?style=for-the-badge&logo=Raspberry-Pi)
![GitHub](https://img.shields.io/badge/github-%23121011.svg?style=for-the-badge&logo=github&logoColor=white)
![Windows 11](https://img.shields.io/badge/Windows%2011-%230079d5.svg?style=for-the-badge&logo=Windows%2011&logoColor=white)

## Descrição do Projeto

Este projeto consiste no desenvolvimento de um ohmímetro utilizando a plataforma BitDogLab (baseada no RP2040). O sistema é capaz de medir a resistência de um resistor desconhecido, aproximá-la para o valor comercial mais próximo da série E24, identificar as cores correspondentes das faixas do resistor e apresentar todas essas informações de forma textual em um display OLED e visualmente através de uma matriz de LEDs WS2812.

## Componentes Utilizados

- **Microcontrolador Raspberry Pi Pico W (RP2040)**: Responsável pelo controle dos pinos GPIO.
- **BitDogLab - Botão B**: Conectado à GPIO 6.
- **BitDogLab - Display SSD1306**: Conectado via I2C nas GPIOs 14 e 15.
- **BitDogLab - Matriz de LEDs WS2812B**: Conectada à GPIO 7.
- **BitDogLab - ADC**: Conectada à GPIO 28.
- **Protoboard**
- **Jumpers Macho-Fêmea**: Quantidade 3x.
- **Resistores**: Dois resistores por medição, um para referência e divisão de tensão. O segundo será medido.

## Ambiente de Desenvolvimento

- **VS Code**: Ambiente de desenvolvimento utilizado para escrever e debugar o código.
- **Linguagem C**: Linguagem de programação utilizada no desenvolvimento do projeto.
- **Pico SDK**: Kit de Desenvolvimento de Software utilizado para programar a placa Raspberry Pi Pico W.
- **Simulador Wokwi**: Ferramenta de simulação utilizada para testar o projeto.

## Guia de Instalação

1. Clone o repositório:
2. Importe o projeto utilizando a extensão da Raspberry Pi.
3. Compile o código utilizando a extensão da Raspberry Pi.
4. Caso queira executar na placa BitDogLab, insira o UF2 na placa em modo bootsel.
5. Para a simulação, basta executar pela extensão no ambiente integrado do VSCode.

## Guia de Uso

1. Garantir a disponibilidade do pino 28 na BitDogLab, trocando o jumper que liga o microfone.
2. Conecte os jumpers no pino do VCC, GND e GPIO 28.
3. Coloque os resistores em série na protoboard.
4. Conecte o jumper do VCC ao resistor de referência.
5. Conecte o jumper do ADC entre os dois resistores.
6. Conecte o jumper do GND ao resistor que deverá ser analisado.
7. O sistema mede a resistência usando o ADC. O valor medido é aproximado para o valor comercial E24 mais próximo.
8. As cores correspondentes são exibidas no display OLED e na matriz de LEDs.
9. Se desejar reinicializar o sistema, o usuário pode pressionar o botão B.

## Testes

Testes básicos foram implementados para garantir que cada componente está funcionando corretamente. 

## Desenvolvedor

[Lucas Gabriel Ferreira](https://github.com/usuario-lider)

## Vídeo da Solução

[Link do YouTube](https://www.youtube.com/watch?v=Yg7zrFLfNIc)


