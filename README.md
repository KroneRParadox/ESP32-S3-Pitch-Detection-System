# ESP32-S3 Pitch Detection System

Este projeto implementa um sistema de **detecção de pitch e análise de áudio em tempo real** utilizando o **ESP32-S3-DevKitC-1**. Ele combina **I2S para captura de áudio**, **FFT para análise espectral** e o **algoritmo YIN para detecção de frequência fundamental**.

## Características

- **Captura de áudio** via microfone I2S.
- **Análise FFT** para extração de espectro.
- **Detecção de pitch** com o algoritmo **YIN**.
- 🎚**Filtro passa-banda** otimizado para eliminar ruídos.
- 🛠**Execução multitarefa** com FreeRTOS para melhor desempenho.
- **Testes automatizados** de processamento de sinais e notas musicais.

## Estrutura do Projeto

📂 src
 ├── 📄 main.c         # Código principal e gerenciamento de tarefas
 ├── 📄 mic.c          # Captura de áudio via I2S
 ├── 📄 filters.c      # Implementação de filtros digitais
 ├── 📄 fft.c          # Transformada Rápida de Fourier (FFT)
 ├── 📄 yin.c          # Algoritmo YIN para detecção de pitch
 ├── 📄 tuner.c        # Conversão de frequência para nota musical
 ├── 📄 utils.c        # Funções auxiliares de matemática e DSP
 ├── 📄 test.c         # Rotinas de teste do sistema


## Requisitos

- **ESP32-S3-DevKitC-1**
- **ESP-IDF** (Ambiente de desenvolvimento da Espressif)
- **Microfone I2S** (ex.: INMP441)

## Funcionamento

### Entrada: Captura de áudio
- O **microfone I2S** captura o áudio e armazena as amostras em um buffer.

### Processamento:
1. **Filtro Passa-Banda**: Remove frequências indesejadas.
2. **FFT**: Analisa o espectro de frequência.
3. **YIN**: Calcula a frequência fundamental.
4. **Conversão para Nota**: Determina a nota musical correspondente.

### Saída:
- A frequência fundamental e a nota musical são enviadas via **UART** no formato:
  ```
  FUND_FREQ=440.00Hz NOTE=A4
  ```

## Testes

O código inclui um módulo de **testes automatizados** (`test.c`) que verifica:
**FFT e cálculo de magnitude**  
**Filtragem digital**  
**Detecção de pitch com YIN**  
**Conversão de frequência para nota musical**  

Para executar os testes:
```sh
idf.py menuconfig   # Ative a opção TESTE=1 no menuconfig
idf.py flash monitor
```

## 📜 Licença

Este projeto está licenciado sob a **MIT License** - veja o arquivo [LICENSE](LICENSE) para mais detalhes.
