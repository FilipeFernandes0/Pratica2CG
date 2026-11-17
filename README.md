# Pratica2CG — Simulador de Aterragem

Trabalho prático 2 de Computação Gráfica: simulação de uma **aterragem** de um avião numa pista, em **OpenGL 3.3 (core)** com C++.

O avião aproxima-se da pista, desce por um planeio de 3°, faz o *flare*, toca no chão, trava e pára. Por defeito a aterragem é feita por um **piloto automático**; também é possível voar em **modo manual**.

## 1. Extrair

Se recebeu o projeto num `.zip`:

```bash
unzip pratica2.zip
cd pratica2
```

(Se recebeu uma pasta, basta entrar nela com `cd pratica2`.)

## 2. Requisitos

Linux (testado em Ubuntu/WSL2) com suporte a OpenGL 3.3. Bibliotecas necessárias:
**GLEW, GLFW, GLM, Assimp**, OpenGL/GLU e um compilador C++ (g++ com C++11) com `make`.

No Ubuntu/Debian:

```bash
sudo apt install build-essential libglew-dev libglfw3-dev libglm-dev libassimp-dev libgl1-mesa-dev libglu1-mesa-dev
```

## 3. Compilar e executar

Dentro da pasta do projeto (onde está o `Makefile`):

```bash
make
./bin/pratica
```

> O programa tem de ser executado **a partir da pasta do projeto**, porque lê os modelos e os shaders por caminhos relativos (`assets/...` e `shaders/...`).
>
> No arranque são carregados vários modelos `.obj` (avião, pista, nuvens, etc.), por isso a janela pode demorar alguns segundos a aparecer.

Para limpar os ficheiros compilados: `make clean`.

## 4. O que se vê

- Ao arrancar, o **piloto automático está ligado**: o avião nasce a 4800 m do centro da pista, a 200 m de altura, e aterra sozinho (cerca de 45 s até parar).
- No **terminal** aparecem, 4 vezes por segundo, os dados de voo: thrust, drag, sustentação, peso, flaps, velocidade, velocidade de referência (Vref), altitude, posição e fase da aterragem. Aparecem também os relatórios do toque e da paragem (distância depois da cabeceira, velocidade, taxa de descida, se ficou dentro da pista).
- A cena tem deserto, pista, nuvens, prédios, sol e lua que dão a volta ao céu (ciclo dia/noite), e luzes ao longo da pista ao anoitecer.

## 5. Controlos

### Geral (sempre ativos)

| Tecla / rato | Ação |
|---|---|
| **M** | Alterna entre **piloto automático** e **controlo manual** |
| **T** (manter premida) | Acelera o ciclo dia/noite |
| **Botão esquerdo do rato + arrastar** | Roda a câmara à volta do avião |
| **R** | Repõe a câmara atrás do avião |
| **Esc** | Fecha o programa |

### Controlo manual (depois de carregar em **M**)

Com o piloto automático ligado, estas teclas ficam bloqueadas.

| Tecla | Ação |
|---|---|
| **W** / **S** | Aumenta / diminui o thrust (motores) |
| **↑** / **↓** | Nariz para baixo / nariz para cima (pitch) |
| **←** / **→** | Vira para a esquerda / direita (yaw) |
| **Q** / **E** | Inclina as asas (roll) |
| **Espaço** / **Shift esquerdo** | Abre / fecha os flaps |

Para aterrar manualmente: manter cerca de 252 km/h (Vref), flaps abertos, descer com um planeio suave (nariz ligeiramente para baixo), alinhar com a pista (eixo Z = 0), tirar o thrust ao tocar e deixar o avião travar.

## 6. Notas técnicas

- **Iluminação:** uma luz pontual (o sol de dia, a lua de noite) com o modelo de **Phong** calculado no *fragment shader* (ambiente + difusa + especular).
- **Modelos:** carregados de ficheiros `.obj` com a biblioteca Assimp; texturas com `stb_image`.
- **Física:** as quatro forças (thrust, drag, peso, sustentação) integradas por Euler; vento, atrito no chão e limites do mapa. É um modelo simplificado, não um 6DOF completo.
- **TOLD:** velocidade de referência, flaps, vento, planeio e relatórios de toque e rolagem.

## 7. Estrutura

```
src/        main.cpp (simulador), model.cpp, mesh.cpp, shader.cpp
common/     cabeçalhos (model, mesh, shader, stb_image)
shaders/    vertex shader e fragment shader (GLSL)
assets/     modelos .obj, materiais .mtl e texturas
bin/ obj/   saída da compilação (ficam vazias na entrega)
Makefile
```
