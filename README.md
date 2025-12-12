# BRASIL - Braço Simulado Legal

**BRASIL** é um simulador de braço robótico 3D controlado por um **Algoritmo Evolutivo (Algoritmo Genético)**. O projeto utiliza um "motor" de cálculo em C++ para a evolução e física, e uma interface gráfica em Python (Pygame + OpenGL) para visualização em tempo real.

O objetivo da IA é controlar as juntas do robô para alcançar um alvo específico no espaço 3D, desviando de obstáculos e otimizando o gasto de energia (movimento).

---

## Funcionalidades

* **Algoritmo Genético em C++:** Implementação robusta com Seleção por Roleta, Elitismo, Crossover Aritmético, Mutação Adaptativa e Catástrofes.
    * **Seleção por Roleta:** Escolhe os pais para a reprodução de forma probabilística, onde indivíduos com maior fitness possuem maior chance de seleção.
    * **Elitismo:** Preserva o melhor indivíduo da geração atual, copiando-o diretamente para a próxima para garantir que a qualidade da solução nunca regrida.
    * **Crossover Aritmético:** Combina os genes dos pais através de uma média matemática, ideal para a representação de valores contínuos (velocidades) usada no projeto.
    * **Mutação Acumulativa:** Aumenta a taxa de mutação automaticamente quando a população estagna.
    * **Catástrofes:** Reinicia parte da população com novos indivíduos aleatórios quando o algoritmo fica preso em máximos locais por muito tempo.
* **Planejamento de Trajetória:** O genoma não representa apenas uma pose, mas uma sequência de velocidades angulares, permitindo que o robô desenhe uma trajetória suave.
* **Cinemática Direta 3D:** Cálculo trigonométrico para mapear ângulos das juntas em coordenadas (X, Y, Z).
* **Visualização Híbrida:** Comunicação via *pipe* (stdout) entre o backend C++ e o frontend Python.

---

## Como Funciona o Algoritmo
### O Genoma
No algoritmo o gene representa a velocidade angular em um determinado instante de tempo.
* **Indivíduo**: Uma matriz de [Waypoints] x [Juntas].
  * **Waypoints**: Número de pontos desejados na trajetória
  * **Juntas**: Número de eixos (3)
* **Trajetória**: A pose no tempo t é calculada somando a velocidade do gene à pose do tempo t-1.

### Função de Fitness (Avaliação)
A nota de cada indivíduo é calculada baseada em:
* **Distância**: Quão perto a ponta do braço chegou do alvo (prioridade máxima).
* **Colisão**: Penalidade severa se tocar em um obstáculo.
* **Eficiência**: Penalidade leve baseada na quantidade total de movimento (evita que o braço fique "tremendo").
* **Bônus**: Recompensa enorme se atingir o alvo antes do tempo acabar.

---

## Configuração Física do Braço

Por simular um braço robótico real, a simulação possui a seguinte morfologia (parametrizável no arquivo `Config.cpp`):

* **Graus de Liberdade:** 3 Juntas Rotativas (Base, Ombro, Cotovelo).
* **Base:** Fixa na origem `(0, 0, 0)`.

| Componente | Comprimento (un) | Limite Mínimo (Ângulo) | Limite Máximo (Ângulo) |
| :--- | :---: | :---: | :---: |
| **Eixo 1 (Base)** | - | -180° | +180° |
| **Eixo 2 (Ombro)** | 10.0 | 0° | +90° |
| **Eixo 3 (Cotovelo)** | 10.0 | 0° | +180° |

> **Nota:** As "unidades" (un) são adimensionais na simulação, mas podem ser interpretadas dependendo da escala desejada.

---

## Simulação e Comportamento

**1. Estado Inicial:**
O robô é inicializado na posição vertical de repouso: `[Base: 0°, Ombro: 90°, Cotovelo: 0°]`.

**2. Ciclo de Execução:**
O algoritmo define um alvo aleatório no espaço de trabalho válido. O braço então performa a melhor trajetória encontrada pela evolução, demonstrando visualmente a convergência, o número total de passos de tempo necessários para o alcance e o fitness médio obtido.
<p align="center">
  <img src="https://github.com/user-attachments/assets/0085217a-3724-4f14-8d2c-50074715e47c" width="480" alt="Captura de tela 2025-12-12 112600" />&nbsp;&nbsp;&nbsp;&nbsp;
  <img src="https://github.com/user-attachments/assets/85a12896-be5e-4e2f-bf10-442827fa788c" width="380" alt="Captura de tela 2025-12-12 112541" />
</p>

---

## Tecnologias Utilizadas

* **Core (Backend):** C++11
* **Visualização (Frontend):** Python 3
    * `pygame` (Janela e Input)
    * `PyOpenGL` (Renderização 3D)
    * `numpy` (Matemática auxiliar)
* **Análise:** `matplotlib` (Para visualização do espaço de trabalho no script auxiliar)

---

## Pré-Requisitos

Antes de rodar, certifique-se de ter os compiladores e bibliotecas instalados.

### 1. Compilador C++ e Make
* **Windows:** Necessário MinGW (g++) e Make (`mingw32-make`).
* **Linux:**
    ```bash
    sudo apt install build-essential
    ```

### 2. Python e Bibliotecas
Instale as dependências do Python:
```bash
pip install pygame PyOpenGL PyOpenGL_accelerate numpy matplotlib
```

---

## Como Compilar e Executar
O projeto conta com um makefile inteligente que detecta seu sistema operacional.
### Passo 1: Compilar
No terminal, na raiz do projeto, execute:
```bash
make
# Ou no Windows, se não tiver o alias 'make' configurado:
mingw32-make
```
### Passo 2: Rodar a Simulação
Para iniciar a simulação visual (que roda o script Python e chama o C++ automaticamente):
```bash
make run
# Ou, para Windows:
mingw32-make run
```
### Comando de Limpeza
Para apagar os arquivos compilados (.o e executáveis):
```bash
make clean
# Ou, para Windows:
mingw32-make clean
```

---

## Estrutura de Arquivos
* **main.cpp**: Loop principal, controle de fluxo e comunicação com Python.
* **Evolution.cpp**: Lógica de seleção, cruzamento, mutação e catástrofe.
* **Robot.cpp**: Física, cinemática direta e detecção de colisão.
* **Config.cpp**: Parâmetros globais (tamanho da população, taxas, limites).
* **simulation.py**: Script de visualização (recebe dados do C++ e desenha na tela).
* **funcaoBraco.py**: Script auxiliar para plotar o volume alcançável do robô com Matplotlib.

---

## Visualização do Espaço Alcançável
O arquivo funcaoBraco.py pode ser executado separadamente para analisar a matemática do braço e ver quais pontos ele consegue alcançar fisicamente (nuvem de pontos).
```bash
python funcaoBraco.py
```
<img src="https://github.com/user-attachments/assets/35b331d6-3f10-499b-9423-44f6b3f4ecfa" width="360" alt="Visualização do Espaço de Trabalho">
