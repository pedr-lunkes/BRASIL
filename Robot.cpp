#include "Robot.h"
#include "Config.h"
#include <cmath>
#include <iostream>

using namespace std;

Obstaculo bolaDeDemolicao = {10.0, 5.0, 5.0, 10.0}; 
vector<double> poseInicial = {0.0, 90.0, 0.0};

double distSq(Ponto p1, Ponto p2) {
    return pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2) + pow(p1.z - p2.z, 2);
}

/// @brief Verifica colisão entre um segmento de reta (braço) e uma esfera (obstáculo).
/// 
/// Utiliza álgebra vetorial para encontrar o ponto no segmento [p1, p2] que está 
/// mais próximo do centro da esfera.
/// 
/// @param p1 Coordenada inicial do segmento
/// @param p2 Coordenada final do segmento
/// @param obs Objeto contendo centro (x,y,z) e raio do obstáculo.
/// @return 'true' se a distância mínima for menor que o raio (houve colisão), 'false' caso contrário.
bool segmentoColideEsfera(Ponto p1, Ponto p2, Obstaculo obs) {
    // vetor ab
    double ab_x = p2.x - p1.x;
    double ab_y = p2.y - p1.y;
    double ab_z = p2.z - p1.z;

    // vetor ac
    double ac_x = obs.x - p1.x;
    double ac_y = obs.y - p1.y;
    double ac_z = obs.z - p1.z; 
                         
    // Produto Vetorial
    double dot = ab_x * ac_x + ab_y * ac_y + ab_z * ac_z; 
    
    // ||ab||²
    double len_sq = ab_x*ab_x + ab_y*ab_y + ab_z*ab_z;  

    double t = -1.0;
    if (len_sq != 0)
        t = dot / len_sq;
    
    t = std::max(0.0, std::min(1.0, t));

    // Projeção
    Ponto closest;
    closest.x = p1.x + ab_x * t;
    closest.y = p1.y + ab_y * t;
    closest.z = p1.z + ab_z * t;

    // Distância do ponto mais próximo ao centro da esfera
    Ponto centroEsfera = {obs.x, obs.y, obs.z};
    double dist = distSq(closest, centroEsfera);

    return dist < (obs.raio * obs.raio);
}

/// @brief Calcula a Cinemática Direta do braço robótico.
/// 
/// Transforma o estado do robô do "Espaço das Juntas" (ângulos em graus) para o 
/// Espaço Cartesiano (coordenada X, Y, Z).
///
/// @param angulos Vetor contendo os ângulos atuais das juntas (Base, Ombro, Cotovelo).
/// @return Ponto Coordenada {x, y, z} da ponta do braço.
Ponto cinematicaDireta(const vector<double>& angulos) {
    // Comprimentos dos segmentos do braço e conversão de graus para radianos
    double L_umero = 10.0;
    double L_antebraco = 10.0;

    double angulo_base     = angulos[0] * (M_PI / 180.0);
    double angulo_ombro    = angulos[1] * (M_PI / 180.0);
    double angulo_cotovelo = angulos[2] * (M_PI / 180.0);
    
    // Cálculo da posição da ponta do braço
    double angulo_abs = angulo_ombro - angulo_cotovelo;

    double r_total = L_umero * cos(angulo_ombro) + L_antebraco * cos(angulo_abs);
    double z_total = L_umero * sin(angulo_ombro) + L_antebraco * sin(angulo_abs);

    double x = r_total * cos(angulo_base);
    double y = r_total * sin(angulo_base);
    double z = z_total;

    return {x, y, z};
}

/// @brief Verifica se o robô colide com o obstaculo dado uma configuração de ângulos.
/// @param angulos Vetor contendo os ângulos atuais das juntas (Base, Ombro, Cotovelo).
/// @return 'true' se houver colisão, 'false' caso contrário.
bool verificarColisao(const vector<double>& angulos) {
    double L_umero = 10.0;

    double angulo_base = angulos[0] * (M_PI / 180.0);
    double angulo_ombro = angulos[1] * (M_PI / 180.0);

    // Posição da base do robô
    Ponto p0 = {0.0, 0.0, 0.0}; 

    double r_cotovelo = L_umero * cos(angulo_ombro);
    double z_cotovelo = L_umero * sin(angulo_ombro);

    // Posição do cotovelo
    Ponto p1;
    p1.x = r_cotovelo * cos(angulo_base);
    p1.y = r_cotovelo * sin(angulo_base);
    p1.z = z_cotovelo;

    // Posição da ponta do braço
    Ponto p2 = cinematicaDireta(angulos);

    // Verifica colisão dos dois segmentos do braço com a esfera
    if (segmentoColideEsfera(p0, p1, bolaDeDemolicao)) return true;
    if (segmentoColideEsfera(p1, p2, bolaDeDemolicao)) return true;
    return false;
}

/// @brief Move o robô aplicando um vetor de velocidades às posições atuais.
/// @param p1 Posições atuais das juntas.
/// @param v Vetor de velocidades a serem aplicadas.
/// @return Posições atualizadas após o movimento, respeitando os limites definidos.
vector<double> move(vector<double> p1, vector<double> v) {
    vector<double> np(p1.size());
    for (int i = 0; i < v.size(); i++) {
        np[i] = p1[i] + v[i];

        if (np[i] < c.baseLmin[i]) np[i] = c.baseLmin[i];
        if (np[i] > c.baseLmax[i]) np[i] = c.baseLmax[i];
    }
    return np;
}

/// @brief Avalia a qualidade (Fitness) de um indivíduo simulando sua trajetória completa.
/// 
/// Esta função executa o "fenótipo" do robô: transforma o genoma (lista de velocidades)
/// em uma trajetória física passo a passo.
/// 
/// @param ind Referência para o indivíduo (será modificado com a nota e trajetória).
/// @param alvo Coordenada (x,y,z) que o robô deve alcançar.
/// @return O valor numérico do fitness calculado.
double calcularFitness(Individuo& ind, Ponto alvo) {
    double penalidadeTotal = 0.0;
    double bonusObjetivo = 0.0;
    
    vector<vector<double>> trajetoria;
    vector<Ponto> trajetoriaPontiforme;
    trajetoria.push_back(poseInicial);
    trajetoriaPontiforme.push_back(cinematicaDireta(poseInicial));

    double distFinal = 0;
    ind.venceu = false;

    for (int i = 1; i < c.nWaypoints; i++) {
        // Calcula trajetória passo a passo
        vector<double> poseAnt = trajetoria[i-1];
        vector<double> poseAtual = move(poseAnt, ind.genoma[i]);
        trajetoria.push_back(poseAtual);

        Ponto p = cinematicaDireta(poseAtual);
        trajetoriaPontiforme.push_back(p);
        double dist = sqrt(pow(p.x - alvo.x, 2) + pow(p.y - alvo.y, 2) + pow(p.z - alvo.z, 2));

        // Verifica se o alvo foi alcançado
        if (dist < 0.2) {
            ind.venceu = true;
            ind.passoVitoria = (int)i;
            bonusObjetivo = 1000.0 + (c.nWaypoints - i) * 500.0;
            break;
        }

        // Penalidades
        penalidadeTotal += dist * 1.5;
        distFinal = dist;

        if (verificarColisao(poseAtual)) {
            penalidadeTotal += 2000.0; 
        }

        double movimento = 0;
        for(int k=0; k<c.nJuntas; k++) movimento += abs(poseAtual[k] - poseAnt[k]);
        penalidadeTotal += movimento * 0.1;
    }

    ind.trajetoria = trajetoriaPontiforme;

    // Cálculo final do fitness
    if (ind.venceu) {
        ind.fitness = bonusObjetivo - penalidadeTotal;
    } else {
        ind.fitness = -penalidadeTotal - distFinal * 2000;
    }
    return ind.fitness;
}
