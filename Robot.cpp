#include "Robot.h"
#include "Config.h"
#include <cmath>

using namespace std;

// Definição do obstáculo
Obstaculo bolaDeDemolicao = {-1, -1, -1, 0}; 
vector<double> poseInicial = {0.0, 90.0, 0.0};

Ponto cinematicaDireta(const vector<double>& angulos) {
    double L_umero = 10.0;
    double L_antebraco = 10.0;

    double angulo_base     = angulos[0] * (M_PI / 180.0);
    double angulo_ombro    = angulos[1] * (M_PI / 180.0);
    double angulo_cotovelo = angulos[2] * (M_PI / 180.0);

    double r_cotovelo = L_umero * cos(angulo_ombro);
    double z_cotovelo = L_umero * sin(angulo_ombro);
    
    double angulo_abs = angulo_ombro - angulo_cotovelo;

    double r_total = L_umero * cos(angulo_ombro) + L_antebraco * cos(angulo_abs);
    double z_total = L_umero * sin(angulo_ombro) + L_antebraco * sin(angulo_abs);

    double x = r_total * cos(angulo_base);
    double y = r_total * sin(angulo_base);
    double z = z_total;

    return {x, y, z};
}

bool verificarColisao(Ponto p) {
    double dist = sqrt(pow(p.x - bolaDeDemolicao.x, 2) + 
                       pow(p.y - bolaDeDemolicao.y, 2) + 
                       pow(p.z - bolaDeDemolicao.z, 2));
    return dist < bolaDeDemolicao.raio;
}

double calcularFitness(Individuo& ind, Ponto alvo) {
    double penalidadeTotal = 0.0;
    double bonusObjetivo = 0.0;
    
    vector<vector<double>> trajetoria;
    trajetoria.push_back(poseInicial);

    for (int i = 0; i < c.nWaypoints; i++) {
        vector<double> pose = ind.genoma[i];
        trajetoria.push_back(pose);
    }

    double distMinGlobal = 1e9;
    ind.venceu = false;

    for (size_t i = 1; i < trajetoria.size(); i++) {
        vector<double>& poseAnt = trajetoria[i-1];
        vector<double>& poseAtual = trajetoria[i];

        Ponto p = cinematicaDireta(poseAtual);
        double dist = sqrt(pow(p.x - alvo.x, 2) + pow(p.y - alvo.y, 2) + pow(p.z - alvo.z, 2));

        if (dist < distMinGlobal) distMinGlobal = dist;

        if (dist < 1.0) {
            ind.venceu = true;
            ind.passoVitoria = (int)i;
            bonusObjetivo = 50000.0 + (c.nWaypoints - i) * 100.0;
            break;
        }

        penalidadeTotal += dist * 1.0;

        if (verificarColisao(p)) {
            penalidadeTotal += 2000.0; 
        }

        double movimento = 0;
        for(int k=0; k<3; k++) movimento += abs(poseAtual[k] - poseAnt[k]);
        penalidadeTotal += movimento * 0.5;
    }

    if (ind.venceu) {
        ind.fitness = bonusObjetivo - penalidadeTotal;
    } else {
        ind.fitness = -penalidadeTotal - (distMinGlobal * 100.0);
    }
    return ind.fitness;
}