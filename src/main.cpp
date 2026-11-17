// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window; 

// GLM header file
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

// shaders header file
#include <common/shader.hpp>
#include <common/model.hpp>

GLuint VertexArrayID;
GLuint programID;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Os nossos modelos
Model* aviaoModel;
Model* pistaModel;
Model* nuvensModel;
Model* bolaModel;
Model* desertoModel;
Model* prediosModel;
Model* casaModel;
Model* luzModel;

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int larguraJanela = SCR_WIDTH;
int alturaJanela = SCR_HEIGHT;

struct FisicasAviao{
    float thrust = 24500.0f;// Eixo Z (Empurra o avião para a frente) - igual ao drag a 70 m/s com flaps a 0
    float drag = 0.0f; // Eixo Z (Trava o avião, luta contra o Thrust)
    float lift = 0.0f; // Eixo Y (Puxa o avião para cima)
    float weight = 0.0f; // Eixo Y (Puxa o avião para baixo, luta contra o Lift)

    float massa = 40000.0f;
    float gravidade = 9.81f;

};

struct EstadoAviao{
    // Eixo X (lados) | Eixo Y (altura) | Eixo Z (frente/trás)
   // Posição inicial da aterragem: no eixo da pista (Z = 0), a 1300 m do ponto de mira, a 150 m de altura (acima do planeio de 3 graus, que a essa distância seria 72 m: o piloto desce mais depressa até o apanhar)
   glm::vec3 posicao = glm::vec3(4800.0f, 200.0f, 0.0f);

   // A velocidade a que o avião se move em cada eixo
    glm::vec3 velocidade = glm::vec3(0.0f, 0.0f, -70.0f);

    // Rotação: Pitch(X - Nariz Cima/Baixo), Yaw(Y - Nariz Esquerda/Direita), Roll(Z - Asas)
    glm::vec3 rotacao = glm::vec3(0.0f, -90.0f, 0.0f);

    // Direção para onde o avião realmente se move (yaw). Segue o nariz aos poucos: é isto que dá resistência ao virar.
    float yawMovimento = -90.0f;
    bool noChao = false;
};


struct TOLDaterragem{
    float vRef = 70.0f; // Velocidade de aproximação ideal (m/s)
    float ventoVelocidade = 10.0f; //velocidade
    float ventoDirecao = 45.0f; //angulo do vento
    float nivelFlaps = 0.0f; // 0.0 (Recolhidos) a 1.0 (Totalmente estendidos para máximo arrasto)
};

FisicasAviao fisicas;
EstadoAviao estado;
TOLDaterragem told;

float anguloDia = 0.2f;
float duracaoDia = 90.0f;
float raioCeu = 5500.0f;

glm::vec3 sunPos, moonPos;

// Efeitos visuais (só mexem no desenho e na câmara, não na física)
float fovAtual = 45.0f;       // campo de visão da câmara (graus): aumenta com a velocidade
float distCamera = 30.0f;     // distância da câmara ao avião: aumenta quando aceleras
float tremorCamera = 0.0f;    // intensidade da vibração da câmara

// Câmara controlada pelo rato: orbita à volta do avião. Valores iniciais = vista de trás do avião (tecla R repõe)
const float PITCH_INICIAL = 18.4f;  // graus (equivale a 10 de altura para 30 de distância)
float camYaw = 0.0f;                // graus: roda à volta do avião (0 = atrás do avião)
float camPitch = PITCH_INICIAL;     // graus: sobe/desce à volta do avião
float efeitoPitch = 0.0f;     // nariz sobe ao acelerar + turbulência (graus)
float brilhoMotores = 0.0f;   // 0 a 1, segue o thrust



//--------------------------------------------------------------------------------
void transferDataToGPUMemory(void)
{
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);
    
    programID = LoadShaders( 
        "shaders/TransformVertexShader.vertexshader", 
        "shaders/ColorFragmentShader.fragmentshader" 
    );

    // Carregar todos os assets 
    aviaoModel = new Model("assets/plane.obj");
    pistaModel = new Model("assets/pista.obj");
    nuvensModel = new Model("assets/Low Poly Clouds.obj");
    bolaModel = new Model("assets/ball.obj");
    desertoModel = new Model("assets/deserto.obj");
    prediosModel = new Model("assets/torres.obj");
    casaModel = new Model("assets/house.obj");
    luzModel = new Model("assets/luz.obj"); 
}



//--------------------------------------------------------------------------------
void cleanupDataFromGPU()
{
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteProgram(programID);
    
    delete aviaoModel;
    delete pistaModel;
    delete nuvensModel;
    delete bolaModel;
    delete desertoModel;
    delete prediosModel;
    delete casaModel;
    delete luzModel;
}

//--------------------------------------------------------------------------------
// FUNÇÕES MODULARES DE DESENHO
//--------------------------------------------------------------------------------

void drawAviao(glm::mat4 Projection, glm::mat4 View, GLuint programID) {
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint modelMatrixLoc = glGetUniformLocation(programID, "ModelMatrix");
    GLuint objectColorLoc = glGetUniformLocation(programID, "objectColor");

    // Matriz de Modelo do Avião (Posição, Escala e Rotação independentes)
    glm::mat4 ModelMatrix = glm::mat4(1.0f);

    ModelMatrix = glm::translate(ModelMatrix, estado.posicao);
    ModelMatrix = glm::rotate(ModelMatrix, glm::radians(estado.rotacao.y), glm::vec3(0.0f,1.0f,0.0f)); //yaw
    ModelMatrix = glm::rotate(ModelMatrix, glm::radians(estado.rotacao.x + efeitoPitch), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch (Nariz)
    ModelMatrix = glm::rotate(ModelMatrix, glm::radians(estado.rotacao.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Roll (Asas) 
    
    glm::mat4 MVP = Projection * View * ModelMatrix;
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &ModelMatrix[0][0]);
    
    glUniform3f(objectColorLoc, 0.7f, 0.75f, 0.8f);

    GLuint hasTextureLoc = glGetUniformLocation(programID, "hasTexture");
    glUniform1i(hasTextureLoc, 0);  

    glUniform1f(glGetUniformLocation(programID, "engineGlow"), brilhoMotores);
    aviaoModel->Draw(programID);
 
}

void drawPista(glm::mat4 Projection, glm::mat4 View, GLuint programID) {
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint modelMatrixLoc = glGetUniformLocation(programID, "ModelMatrix");

    glm::mat4 ModelMatrix = glm::mat4(1.0f);
   
    ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
    ModelMatrix = glm::scale(ModelMatrix, glm::vec3(3400.0f, 0.0f, 150.0f));


    glm::mat4 MVP = Projection * View * ModelMatrix;
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &ModelMatrix[0][0]);
    

    GLuint hasTextureLoc = glGetUniformLocation(programID, "hasTexture");
    glUniform1i(hasTextureLoc, 1);

    pistaModel->Draw(programID);
}

void drawDeserto(glm::mat4 Projection, glm::mat4 View, GLuint programID){
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint modelMatrixLoc = glGetUniformLocation(programID, "ModelMatrix");
    
    GLuint hasTextureLoc = glGetUniformLocation(programID, "hasTexture");
    glUniform1i(hasTextureLoc, 1);

    glm::mat4 ModelMatrix = glm::mat4(1.0f);
    
    ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, -2.0f, 0.0f));
    
    ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5000.0f, 1.0f, 5000.0f));

    glm::mat4 MVP = Projection * View * ModelMatrix;
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &ModelMatrix[0][0]);
    
    // Tom da areia: amarelo torrado (multiplica a cor da textura; só afeta o deserto)
    GLuint tintLoc = glGetUniformLocation(programID, "tint");
    glUniform3f(tintLoc, 1.6f, 1.3f, 0.6f);
    desertoModel->Draw(programID);
    glUniform3f(tintLoc, 1.0f, 1.0f, 1.0f);
}

void drawNuvens(glm::mat4 Projection, glm::mat4 View, GLuint programID) {
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint modelMatrixLoc = glGetUniformLocation(programID, "ModelMatrix");
    GLuint hasTextureLoc = glGetUniformLocation(programID, "hasTexture");
    GLuint objectColorLoc = glGetUniformLocation(programID, "objectColor");
    glUniform1i(hasTextureLoc, 0);

    glUniform3f(objectColorLoc, 1.0f, 1.0f, 1.0f);



    glm::mat4 ModelMatrix = glm::mat4(1.0f);

    ModelMatrix = glm::translate(ModelMatrix, glm::vec3(2025.0f, 400.0f, -415.0f));
    ModelMatrix = glm::scale(ModelMatrix, glm::vec3(100.0f, 100.0f, 100.0f));

    glm::mat4 MVP = Projection * View * ModelMatrix;
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &ModelMatrix[0][0]);
    glUniform3f(objectColorLoc, 1.0f, 1.0f, 1.0f);

    nuvensModel->Draw(programID);
}

// Desenha uma luz (pequena malha brilhante, sem sombreamento) com a matriz dada e a cor dada
void drawLuz(glm::mat4 Projection, glm::mat4 View, GLuint programID, glm::mat4 ModelMatrix, glm::vec3 cor){
    glm::mat4 MVP = Projection * View * ModelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(programID, "MVP"), 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(programID, "ModelMatrix"), 1, GL_FALSE, &ModelMatrix[0][0]);
    glUniform1i(glGetUniformLocation(programID, "hasTexture"), 0);
    glUniform3f(glGetUniformLocation(programID, "emissiveColor"), cor.r, cor.g, cor.b);

    GLuint emissiveLoc = glGetUniformLocation(programID, "emissive");
    glUniform1i(emissiveLoc, 1);
    luzModel->Draw(programID);
    glUniform1i(emissiveLoc, 0);
}

void drawLuzesPista(glm::mat4 Projection, glm::mat4 View, GLuint programID){
    const float espacamento = 200.0f; 
    for (float x = -3400.0f; x <= 3400.0f; x += espacamento) {
        for (int lado = -1; lado <= 1; lado += 2) {
            glm::mat4 M = glm::translate(glm::mat4(1.0f), glm::vec3(x, 1.0f, lado * 160.0f));
            M = glm::scale(M, glm::vec3(3.0f)); 
            drawLuz(Projection, View, programID, M, glm::vec3(1.0f, 0.95f, 0.65f)); // branco quente
        }
    }
}

void drawBola(glm::mat4 Projection, glm::mat4 View, GLuint programID,  glm::vec3 pos, float escala, glm::vec3 cor){
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint modelMatrixLoc = glGetUniformLocation(programID, "ModelMatrix");

    glm::mat4 ModelMatrix = glm::mat4(1.0f);

    ModelMatrix = glm::translate(ModelMatrix, pos);
    ModelMatrix = glm::scale(ModelMatrix, glm::vec3(escala)); 

    glm::mat4 MVP = Projection * View * ModelMatrix;
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &ModelMatrix[0][0]);
   
    glUniform3f(glGetUniformLocation(programID, "emissiveColor"), cor.r, cor.g, cor.b);

    GLuint hasTextureLoc = glGetUniformLocation(programID, "hasTexture");
    glUniform1i(hasTextureLoc, 0);

    GLuint emissiveLoc = glGetUniformLocation(programID, "emissive");
    glUniform1i(emissiveLoc, 1);
    bolaModel->Draw(programID);
    glUniform1i(emissiveLoc, 0);

}

void drawPredios(glm::mat4 Projection, glm::mat4 View, GLuint programID){
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");
    GLuint modelMatrixLoc = glGetUniformLocation(programID, "ModelMatrix");
    GLuint hasTextureLoc = glGetUniformLocation(programID, "hasTexture");

    glm::mat4 ModelMatrix = glm::mat4(1.0f);
    ModelMatrix = glm::scale(ModelMatrix, glm::vec3(1.0f, 3.0f, 1.0f));

    glm::mat4 MVP = Projection * View * ModelMatrix;
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
    glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &ModelMatrix[0][0]);

    glUniform1i(hasTextureLoc, 0);

    prediosModel->Draw(programID);

   
    glm::mat4 ModelMatrixCasa = glm::mat4(1.0f);
    ModelMatrixCasa = glm::translate(ModelMatrixCasa, glm::vec3(1000.0f, 0.0f, -450.0f));
    ModelMatrixCasa = glm::scale(ModelMatrixCasa, glm::vec3(60.0f, 60.0f, 60.0f));

    glm::mat4 MVPCasa = Projection * View * ModelMatrixCasa;
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVPCasa[0][0]);
    glUniformMatrix4fv(modelMatrixLoc, 1, GL_FALSE, &ModelMatrixCasa[0][0]);

    glUniform1i(hasTextureLoc, 1);

    casaModel->Draw(programID);
}

//--------------------------------------------------------------------------------
// GESTOR PRINCIPAL
//--------------------------------------------------------------------------------
void draw(void)
{
    sunPos = glm::vec3(-raioCeu*cos(anguloDia), raioCeu * sin(anguloDia), 0.0f);
    moonPos = -sunPos;

    float h = sin(anguloDia);
 

    glm::vec3 ceuDia(0.5f, 0.7f, 0.9f), ceuPoente(0.9f, 0.5f, 0.3f), ceuNoite(0.02f, 0.03f, 0.08f);
    glm::vec3 ceu = (h > 0.0f) ? glm::mix(ceuPoente, ceuDia,   glm::clamp(h * 3.0f,  0.0f, 1.0f))
                        : glm::mix(ceuPoente, ceuNoite, glm::clamp(-h * 3.0f, 0.0f, 1.0f));
    glClearColor(ceu.r, ceu.g, ceu.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(programID);
    glUniform3f(glGetUniformLocation(programID, "tint"), 1.0f, 1.0f, 1.0f); 

    // 1. Configurar Câmara 
    glm::mat4 Projection = glm::perspective(glm::radians(fovAtual), float(larguraJanela) / float(alturaJanela), 1.0f, 10000.0f);
    // Vibração da câmara (mais forte com mais thrust) e distância que varia quando aceleras
    float t = static_cast<float>(glfwGetTime());
    glm::vec3 tremor = glm::vec3(0.0f, sin(10.0f * t),cos(10.0f * t)) * tremorCamera;
    // Posição da câmara numa esfera à volta do avião (os ângulos vêm do rato)
    float raioCamera = glm::length(glm::vec2(distCamera, 10.0f)); // mesma distância que antes
    float yawC = glm::radians(camYaw);
    float pitchC = glm::radians(camPitch);
    glm::vec3 offsetCamera = raioCamera * glm::vec3(cos(pitchC) * cos(yawC), sin(pitchC), cos(pitchC) * sin(yawC)) + tremor;
    glm::vec3 cameraPos = estado.posicao + offsetCamera;
    glm::mat4 View = glm::lookAt(cameraPos, estado.posicao, glm::vec3(0, 1, 0));

    // 2. Configurar Luz Global (Aplica-se a todos os objetos)
    GLuint lightPosLoc = glGetUniformLocation(programID, "lightPos");
    GLuint viewPosLoc  = glGetUniformLocation(programID, "viewPos");
    GLuint lightColorLoc = glGetUniformLocation(programID, "lightColor");

    float fatorSol  = glm::clamp( h * 3.0f, 0.0f, 1.0f);
    float fatorLua  = glm::clamp(-h * 3.0f, 0.0f, 1.0f);

    glm::vec3 corSolDia   = glm::vec3(1.0f, 0.95f, 0.85f);
    glm::vec3 corSolPoente = glm::vec3(1.0f, 0.5f, 0.2f);
    glm::vec3 corSol = glm::mix(corSolPoente, corSolDia, fatorSol);

    glm::vec3 corLua = glm::vec3(0.4f, 0.5f, 0.8f) * 0.35f;

    glm::vec3 posLuz = (h >= 0.0f) ? sunPos : moonPos;
    glm::vec3 corLuz = corSol * fatorSol + corLua * fatorLua;

    glUniform3f(lightPosLoc,   posLuz.x, posLuz.y, posLuz.z);
    glUniform3f(viewPosLoc,    cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(lightColorLoc, corLuz.r, corLuz.g, corLuz.b);

    drawDeserto(Projection, View, programID);
    drawPista(Projection, View, programID);
    drawNuvens(Projection, View, programID);
    if (h > -0.1f){
     drawBola(Projection, View, programID, sunPos,  5.0f, glm::vec3(1.0f, 0.9f, 0.5f));
    }

    if (h <  0.1f) {
        drawBola(Projection, View, programID, moonPos, 3.0f, glm::vec3(0.8f, 0.85f, 1.0f));
    }
    drawAviao(Projection, View, programID);
    drawPredios(Projection,View,programID);
    if (h < 0.2f) drawLuzesPista(Projection, View, programID); // luzes da pista só ao fim da tarde e à noite
}

//--------------------------------------------------------------------------------



// ==========================================
// ATERRAGEM AUTOMÁTICA (tecla M alterna entre automático e manual)
// ==========================================
// A pista vai de X = +3400 (cabeceira, por onde o avião entra) a X = -3400 (fim). O avião voa para -X.
// Fases: APROXIMACAO (desce pelo planeio de 3 graus) -> FLARE (suaviza a descida perto do chão)
//        -> ROLAGEM (no chão, motores a zero e a travar) -> PARADO
bool pilotoAutomatico = true;
enum FaseAterragem { APROXIMACAO, FLARE, ROLAGEM, PARADO };
FaseAterragem fase = APROXIMACAO;

const float CABECEIRA_X   = 3400.0f;  // início da pista
const float FIM_PISTA_X   = -3400.0f; // fim da pista
const float PONTO_MIRA_X  = 3500.0f;  // onde o planeio de 3 graus chegaria ao chão (junto à cabeceira); o flare faz tocar ~300 m depois
const float ANGULO_PLANEIO = 3.0f;    // graus (planeio normal de aterragem)
const float ALTURA_FLARE  = 15.0f;    // abaixo desta altura começa o "flare"
const float ALTURA_CHAO   = 3.5f;     // altura do centro do avião quando está pousado

float toqueX = 0.0f;                  // dados do toque, para o relatório final
float toqueVelocidade = 0.0f;
float toqueDescida = 0.0f;

void pilotoAutomaticoAterragem(float dt){
    float v = -estado.velocidade.z;                       // velocidade para a frente (m/s)
    float alturaSolo = estado.posicao.y - ALTURA_CHAO;    // altura acima da pista
    static float descidaAnterior = 0.0f;                  // taxa de descida do frame anterior (m/s)

    // Vento: componente contra o avião (reduz a velocidade em relação ao chão) e componente lateral
    float ventoRad = glm::radians(told.ventoDirecao);
    float ventoX = sin(ventoRad) * told.ventoVelocidade;
    float ventoZ = cos(ventoRad) * told.ventoVelocidade;

    // ---------- MUDANÇAS DE FASE ----------
    if (fase == APROXIMACAO && alturaSolo < ALTURA_FLARE) {
        fase = FLARE;
    }
    if ((fase == APROXIMACAO || fase == FLARE) && estado.noChao) {
        fase = ROLAGEM;
        toqueX = estado.posicao.x;
        toqueVelocidade = v;
        toqueDescida = descidaAnterior;
        printf("\n>>> TOQUE: X = %.0f m (%.0f m depois da cabeceira) | velocidade %.0f km/h (Vref %.0f) | descida %.1f m/s (%s)\n\n",
               toqueX, CABECEIRA_X - toqueX, toqueVelocidade * 3.6f, told.vRef * 3.6f, toqueDescida,
               toqueDescida > 3.0f ? "POUSO DURO" : "suave");
    }
    if (fase == ROLAGEM && v < 0.5f) {
        fase = PARADO;
        float rolagem = toqueX - estado.posicao.x;
        printf("\n>>> PARADO: rolagem de %.0f m | sobram %.0f m de pista | %s\n\n",
               rolagem, estado.posicao.x - FIM_PISTA_X,
               estado.posicao.x > FIM_PISTA_X ? "aterragem dentro da pista" : "SAIU DA PISTA");
    }

    // ---------- FLAPS: abre-os até ao máximo (configuração de aterragem) ----------
    if (fase != PARADO && told.nivelFlaps < 1.0f) {
        told.nivelFlaps = glm::min(1.0f, told.nivelFlaps + 0.5f * dt);
    }

    // ---------- THRUST: mantém a velocidade de aproximação (vRef); no chão corta os motores ----------
    if (fase == APROXIMACAO || fase == FLARE) {
        // compensa o drag e corrige a diferença de velocidade (4000 N por cada m/s de erro)
        fisicas.thrust = glm::clamp(fisicas.drag + 4000.0f * (told.vRef - v), 0.0f, 120000.0f);
    } else {
        fisicas.thrust = 0.0f;
    }

    // ---------- DIREÇÃO: alinha com o eixo da pista (Z = 0) ----------
    // yaw = -90 é voar para -X. Se Z > 0 (avião ao lado), vira um pouco para -Z, e vice-versa.
    float correcaoCrab = 0.0f; // virar o nariz contra o vento lateral
    if (fase == APROXIMACAO || fase == FLARE) {
        correcaoCrab = glm::degrees(asin(glm::clamp(ventoZ / glm::max(v, 1.0f), -0.5f, 0.5f)));
    }
    float correcaoZ = glm::clamp(0.1f * estado.posicao.z, -10.0f, 10.0f); // graus
    float yawAlvo = -90.0f - correcaoCrab - correcaoZ;
    estado.yawMovimento += glm::clamp(yawAlvo - estado.yawMovimento, -3.0f * dt, 3.0f * dt); // vira no máximo 3 graus/s
    estado.rotacao.y = estado.yawMovimento;
    estado.rotacao.z = 0.0f; // asas niveladas

    // ---------- PITCH: segue o planeio e depois faz o flare ----------
    if (fase == APROXIMACAO || fase == FLARE) {
        float descida; // taxa de descida pretendida (m/s)
        if (fase == APROXIMACAO) {
            float velocidadeSolo = v - ventoX;                                  // o vento de frente reduz a velocidade no chão
            float distancia = glm::max(estado.posicao.x - PONTO_MIRA_X, 0.0f);   // distância horizontal até ao ponto de mira
            float alturaIdeal = ALTURA_CHAO + distancia * tan(glm::radians(ANGULO_PLANEIO));
            float erro = estado.posicao.y - alturaIdeal;                         // positivo = acima do planeio
            descida = velocidadeSolo * tan(glm::radians(ANGULO_PLANEIO)) + 0.08f * erro;
            descida = glm::clamp(descida, 0.0f, 8.0f);
        } else {
            descida = glm::clamp(alturaSolo / 3.0f, 0.5f, 5.0f); // quanto mais perto do chão, mais suave (mínimo 0,5 m/s para tocar)
        }
        // taxa de descida = v*sin(pitch) - velocidade.y  ->  pitch = asin((descida + velocidade.y) / v)
        float seno = glm::clamp((descida + estado.velocidade.y) / glm::max(v, 1.0f), -0.3f, 0.3f);
        float pitchAlvo = glm::degrees(asin(seno));
        estado.rotacao.x += glm::clamp(pitchAlvo - estado.rotacao.x, -6.0f * dt, 6.0f * dt); // roda no máximo 6 graus/s

        descidaAnterior = v * sin(glm::radians(estado.rotacao.x)) - estado.velocidade.y; // para o relatório do toque
    }
}
// ===AUTOPILOTO-FIM===

 void atualizarFisica(GLFWwindow* window, float deltaTime) {

    // ==========================================
    // 0. CICLO DIA/NOITE (T acelera o tempo 10x)
    // ==========================================
    deltaTime = glm::min(deltaTime, 0.05f); // no 1º frame o deltaTime inclui o tempo de carregar os modelos
    float velocidadeTempo = 0.5f;
    if(glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS){
        velocidadeTempo = 10.0f;
    }
    anguloDia += (2.0f * 3.14159265f / duracaoDia) * velocidadeTempo * deltaTime;

    // ==========================================
    // 1. CONTROLOS DE POTÊNCIA (OS MOTORES)
    // ==========================================
    
    if(!pilotoAutomatico && glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        fisicas.thrust += 5000.0f * deltaTime; 
    }
    if(!pilotoAutomatico && glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        fisicas.thrust -= 5000.0f * deltaTime; 
    }
    if(fisicas.thrust < 0.0f) {
        fisicas.thrust = 0.0f;
    }
    if(fisicas.thrust > 120000.0f) {
        fisicas.thrust = 120000.0f; // máximo dos motores
    }


    // ==========================================
    // 2. CONTROLOS das ROTAÇÕES
    // ==========================================
    float velocidadeCurva = 30.0f; 

    if(!pilotoAutomatico && glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS){
        estado.rotacao.x += velocidadeCurva * deltaTime; 
    }
    if(!pilotoAutomatico && glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS){
        estado.rotacao.x -= velocidadeCurva * deltaTime; 
    }
    if(!pilotoAutomatico && glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS){
        estado.rotacao.y += velocidadeCurva * deltaTime; 
    }
    if(!pilotoAutomatico && glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS){
        estado.rotacao.y -= velocidadeCurva * deltaTime; 
    }

    if(!pilotoAutomatico && glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS){
        estado.rotacao.z -= velocidadeCurva * deltaTime; 
    }
    if(!pilotoAutomatico && glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS){
        estado.rotacao.z += velocidadeCurva * deltaTime; 
    }

    // O nariz não pode passar de 60 graus para cima nem para baixo
    estado.rotacao.x = glm::clamp(estado.rotacao.x, -60.0f, 60.0f);

    // No chão o avião não roda: fica direito (sem pitch nem roll) e alinhado com a direção em que anda
    if (estado.noChao) {
        estado.rotacao.x = 0.0f;
        estado.rotacao.z = 0.0f;
        estado.rotacao.y = estado.yawMovimento;
    }

    // ==========================================
    // 3. SISTEMA DE FLAPS e TRAVÕES
    // ==========================================
    
    if(!pilotoAutomatico && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS){
        told.nivelFlaps += 0.5f * deltaTime; 
        if(told.nivelFlaps > 1.0f) told.nivelFlaps = 1.0f; 
    }
    if(!pilotoAutomatico && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS){
        told.nivelFlaps -= 0.5f * deltaTime; 
        if(told.nivelFlaps < 0.0f) told.nivelFlaps = 0.0f; 
    }

    // ==========================================
    // 3.5 ATERRAGEM AUTOMÁTICA (tecla M alterna automático / manual)
    // ==========================================
    static bool mAnterior = false;
    bool mAgora = (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS);
    if (mAgora && !mAnterior) {
        pilotoAutomatico = !pilotoAutomatico;
        printf(pilotoAutomatico ? "\n>>> PILOTO AUTOMATICO LIGADO\n\n" : "\n>>> CONTROLO MANUAL\n\n");
    }
    mAnterior = mAgora;
    if (pilotoAutomatico) pilotoAutomaticoAterragem(deltaTime); // sobrepõe thrust, flaps, pitch e yaw

    // ==========================================
    // 4. FORÇAS E VELOCIDADE
    // ==========================================
    
    float multiplicadorDrag = 5.0f + (15.0f * told.nivelFlaps);
    fisicas.drag = multiplicadorDrag * (estado.velocidade.z * estado.velocidade.z);
    
    // A sustentação sobe com a velocidade ao quadrado (como o drag) e iguala o peso à velocidade vRef.
    fisicas.weight = fisicas.massa * fisicas.gravidade;
    fisicas.lift = fisicas.weight * (estado.velocidade.z * estado.velocidade.z) / (told.vRef * told.vRef);

    // Força vertical = sustentação - peso -> aceleração -> velocidade vertical
    float aceleracaoY = (fisicas.lift - fisicas.weight) / fisicas.massa;
    estado.velocidade.y += aceleracaoY * deltaTime;
    estado.velocidade.y *= (1.0f - 1.0f * deltaTime); // resistência do ar na vertical (senão acelerava para sempre)

    // Atrito com a pista (só quando está no chão): coeficiente x peso (massa x gravidade)
    float atrito = 0.0f;
    if (estado.noChao) {
        atrito = 0.3f * fisicas.massa * fisicas.gravidade;
    }

    float forcaResultanteZ = fisicas.thrust - fisicas.drag - atrito;
    float aceleracaoZ = forcaResultanteZ / fisicas.massa;
    
    estado.velocidade.z -= aceleracaoZ * deltaTime;

    // O atrito só trava: nunca deixa o avião andar para trás (velocidade.z é negativa para a frente)
    if (estado.velocidade.z > 0.0f) estado.velocidade.z = 0.0f;

    // ==========================================
    // 5. TRIGONOMETRIA E MOVIMENTO 3D
    // ==========================================

    // Resistência ao virar (esquerda/direita):
    // "diferenca" é quanto o nariz está à frente da direção em que o avião se move.
    float diferenca = estado.rotacao.y - estado.yawMovimento;
    estado.rotacao.y    -= 3.0f * diferenca * deltaTime; // o nariz é puxado de volta para a trajetória (resiste a virar)
    estado.yawMovimento += 0.6f * diferenca * deltaTime; // a trajetória segue o nariz devagar
    
    float yawRad = glm::radians(estado.yawMovimento);
    float pitchRad = glm::radians(estado.rotacao.x);

    glm::vec3 frente;
    frente.x = sin(yawRad) * cos(pitchRad);
    frente.y = -sin(pitchRad); 
    frente.z = cos(yawRad) * cos(pitchRad);

    float rapidezReal = -estado.velocidade.z;
    estado.posicao.x += frente.x * rapidezReal * deltaTime;
    estado.posicao.y += frente.y * rapidezReal * deltaTime;
    estado.posicao.z += frente.z * rapidezReal * deltaTime;
    estado.posicao.y += estado.velocidade.y * deltaTime; // subida/descida causada pela sustentação e pelo peso

    // ==========================================
    // 6. EFEITOS DO TOLD (METEOROLOGIA)
    // ==========================================

    float ventoRad = glm::radians(told.ventoDirecao);
    float ventoEmpurraX = sin(ventoRad) * told.ventoVelocidade;
    float ventoEmpurraZ = cos(ventoRad) * told.ventoVelocidade;
    if (!estado.noChao) { // no chão o vento não empurra o avião
        estado.posicao.x += ventoEmpurraX * deltaTime;
        estado.posicao.z += ventoEmpurraZ * deltaTime;
    }

    // ==========================================
    // 7. LIMITES DO MUNDO
    // ==========================================
    const float limiteMapa   = 4900.0f;   
    const float alturaMaxima = 10000.0f;
    const float alturaChao   = 3.5f;     

    estado.posicao.x = glm::clamp(estado.posicao.x, -limiteMapa, limiteMapa);
    estado.posicao.z = glm::clamp(estado.posicao.z, -limiteMapa, limiteMapa);
    estado.posicao.y = glm::min(estado.posicao.y, alturaMaxima);

    if (estado.posicao.y <= alturaChao) {
        estado.posicao.y = alturaChao;
        estado.noChao = true;
        if (estado.velocidade.y < 0.0f) estado.velocidade.y = 0.0f; // o chão pára a queda
    } else {
        estado.noChao = false;
    }

    // ==========================================
    // 8. EFEITOS VISUAIS (câmara e animação do avião; não alteram a física)
    // ==========================================
    // "impulso" vai de -1 (a travar) a +1 (a acelerar forte): é a força resultante em Z
    float impulso = glm::clamp(forcaResultanteZ / 30000.0f, -1.0f, 1.0f);
    float velFator = glm::clamp(rapidezReal / 140.0f, 0.0f, 1.0f);
    float suave = glm::min(1.0f, 3.0f * deltaTime); // quão depressa os efeitos acompanham o alvo

    // Campo de visão: sobe com a velocidade e com o impulso
    fovAtual += (glm::clamp(42.0f + 16.0f * velFator + 5.0f * impulso, 38.0f, 65.0f) - fovAtual) * suave;
    // A câmara afasta-se quando aceleras e aproxima-se quando travas
    distCamera += ((30.0f + 6.0f * impulso) - distCamera) * suave;
    // Vibração: só quando aceleras (impulso > 0). Ao travar ou a velocidade constante pára
    brilhoMotores = glm::clamp(2.0f * fisicas.thrust / 120000.0f, 0.0f, 1.0f);
    tremorCamera = estado.noChao ? 0.0f : glm::max(impulso, 0.0f) / 2.0f;

    // Animação do avião (no chão fica quieto)
    float tempo = static_cast<float>(glfwGetTime());
    float alvoPitch = 0.0f;
    if (!estado.noChao) {
        // O nariz sobe ao acelerar e desce ao travar
        alvoPitch = -3.0f * impulso;
        // Turbulência leve
        alvoPitch += 0.4f * sin(tempo * 1.7f + 1.0f) + 0.2f * sin(tempo * 3.3f);
    }
    efeitoPitch += (alvoPitch - efeitoPitch) * suave;

    // ==========================================
    // 9. INFORMAÇÃO NO TERMINAL (4 vezes por segundo)
    // ==========================================
    static float tempoPrint = 0.0f;
    tempoPrint += deltaTime;
    if (tempoPrint >= 0.25f) {
        tempoPrint = 0.0f;

        float velKmH = -(estado.velocidade.z) * 3.6f;
        const char* estadoVel = "ok";
        if (rapidezReal < 0.9f * told.vRef) estadoVel = "LENTO";
        if (rapidezReal > 1.1f * told.vRef) estadoVel = "RAPIDO";

        printf("Thrust: %6.0f | Drag: %6.0f | Lift: %6.0f | Peso: %6.0f | Flaps: %.2f | Vel: %4.0f km/h | Vref: %3.0f km/h (%s) | Alt: %5.0f m | X: %6.0f Z: %5.0f%s\n",
               fisicas.thrust, fisicas.drag, fisicas.lift, fisicas.weight, told.nivelFlaps, velKmH, told.vRef * 3.6f, estadoVel, estado.posicao.y,
               estado.posicao.x, estado.posicao.z,
               !pilotoAutomatico ? " | MANUAL" : (fase == APROXIMACAO ? " | AUTO: aproximacao" : fase == FLARE ? " | AUTO: flare" : fase == ROLAGEM ? " | AUTO: rolagem" : " | AUTO: parado"));
    }
}

void mouse_callback(GLFWwindow* janela, double xpos, double ypos)
{
    static bool primeiraLeitura = true;
    static double ultimoX = 0.0, ultimoY = 0.0;
    if (primeiraLeitura) { 
        ultimoX = xpos; ultimoY = ypos;
        primeiraLeitura = false;
    }
    if (glfwGetMouseButton(janela, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        float sensibilidade = 0.1f; 
        camYaw   += static_cast<float>(xpos - ultimoX) * sensibilidade;
        camPitch += static_cast<float>(ypos - ultimoY) * sensibilidade; 
    }
    ultimoX = xpos;
    ultimoY = ypos;

    camPitch = glm::clamp(camPitch, 5.0f, 80.0f);
}

void framebuffer_size_callback(GLFWwindow* janela, int largura, int altura)
{
    if (largura > 0 && altura > 0) {
        larguraJanela = largura;
        alturaJanela = altura;
        glViewport(0, 0, largura, altura);
    }
}

int main()
{
    glfwInit();
    
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); 
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window = glfwCreateWindow( SCR_WIDTH, SCR_HEIGHT, "Trabalho CG - Simulador", NULL, NULL);
    glfwMakeContextCurrent(window);
    
    glewExperimental = true; 
    glewInit();
    
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwGetFramebufferSize(window, &larguraJanela, &alturaJanela);

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Câmara com o rato: o cursor continua visível; arrasta com o botão direito para rodar a câmara
    glfwSetCursorPosCallback(window, mouse_callback);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    transferDataToGPUMemory();



   
    
    do{

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;


        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            camYaw = 0.0f;
            camPitch = PITCH_INICIAL;
        }

        atualizarFisica(window, deltaTime);
        draw();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
        
    } while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
    
    cleanupDataFromGPU();
    glfwTerminate();
    
    return 0;
}