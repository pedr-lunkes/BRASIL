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

bool segmentoColideEsfera(Ponto p1, Ponto p2, Obstaculo obs) {
    double ab_x = p2.x - p1.x;
    double ab_y = p2.y - p1.y;
    double ab_z = p2.z - p1.z; // vetor ab

    double ac_x = obs.x - p1.x;
    double ac_y = obs.y - p1.y;
    double ac_z = obs.z - p1.z; // vetor ac
                                
    double dot = ab_x * ac_x + ab_y * ac_y + ab_z * ac_z; // Produto Vetorial
    
    double len_sq = ab_x*ab_x + ab_y*ab_y + ab_z*ab_z; // ||ab||² 

    double t = -1.0;
    if (len_sq != 0)
        t = dot / len_sq;
    
    t = std::max(0.0, std::min(1.0, t));

    // Projeção
    Ponto closest;
    closest.x = p1.x + ab_x * t;
    closest.y = p1.y + ab_y * t;
    closest.z = p1.z + ab_z * t;

    Ponto centroEsfera = {obs.x, obs.y, obs.z};
    double dist = distSq(closest, centroEsfera);

    return dist < (obs.raio * obs.raio);
}

Ponto cinematicaDireta(const vector<double>& angulos) {
    double L_umero = 10.0;
    double L_antebraco = 10.0;

    double angulo_base     = angulos[0] * (M_PI / 180.0);
    double angulo_ombro    = angulos[1] * (M_PI / 180.0);
    double angulo_cotovelo = angulos[2] * (M_PI / 180.0);
    
    double angulo_abs = angulo_ombro - angulo_cotovelo;

    double r_total = L_umero * cos(angulo_ombro) + L_antebraco * cos(angulo_abs);
    double z_total = L_umero * sin(angulo_ombro) + L_antebraco * sin(angulo_abs);

    double x = r_total * cos(angulo_base);
    double y = r_total * sin(angulo_base);
    double z = z_total;

    return {x, y, z};
}

bool verificarColisao(const vector<double>& angulos) {
    double L_umero = 10.0;

    double angulo_base = angulos[0] * (M_PI / 180.0);
    double angulo_ombro = angulos[1] * (M_PI / 180.0);

    Ponto p0 = {0.0, 0.0, 0.0};

    double r_cotovelo = L_umero * cos(angulo_ombro);
    double z_cotovelo = L_umero * sin(angulo_ombro);

    Ponto p1;
    p1.x = r_cotovelo * cos(angulo_base);
    p1.y = r_cotovelo * sin(angulo_base);
    p1.z = z_cotovelo;

    Ponto p2 = cinematicaDireta(angulos);

    if (segmentoColideEsfera(p0, p1, bolaDeDemolicao)) return true;
    if (segmentoColideEsfera(p1, p2, bolaDeDemolicao)) return true;
    return false;

}

vector<double> move(vector<double> p1, vector<double> v) {
    vector<double> np(p1.size());
    for (int i = 0; i < v.size(); i++) {
        np[i] = p1[i] + v[i];

        if (np[i] < c.baseLmin[i]) np[i] = c.baseLmin[i];
        if (np[i] > c.baseLmax[i]) np[i] = c.baseLmax[i];
    }
    return np;
}

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
        vector<double> poseAnt = trajetoria[i-1];
        vector<double> poseAtual = move(poseAnt, ind.genoma[i]);
        trajetoria.push_back(poseAtual);

        Ponto p = cinematicaDireta(poseAtual);
        trajetoriaPontiforme.push_back(p);
        double dist = sqrt(pow(p.x - alvo.x, 2) + pow(p.y - alvo.y, 2) + pow(p.z - alvo.z, 2));

        if (dist < 0.2) {
            ind.venceu = true;
            ind.passoVitoria = (int)i;
            bonusObjetivo = 1000.0 + (c.nWaypoints - i) * 500.0;
            break;
        }

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

    if (ind.venceu) {
        ind.fitness = bonusObjetivo - penalidadeTotal;
    } else {
        ind.fitness = -penalidadeTotal - distFinal * 2000;
    }
    return ind.fitness;
}
