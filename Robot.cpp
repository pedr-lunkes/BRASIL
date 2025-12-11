#include "Robot.h"
#include "Config.h"
#include <cmath>
#include <iostream>

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

        if (dist < 0.5) {
            ind.venceu = true;
            ind.passoVitoria = (int)i;
            bonusObjetivo = 1000.0 + (c.nWaypoints - i) * 500.0;
            break;
        }

        penalidadeTotal += dist * 1.5;
        distFinal = dist;

        if (verificarColisao(p)) {
            penalidadeTotal += 2000.0; 
        }

        double movimento = 0;
        for(int k=0; k<c.nJuntas; k++) movimento += abs(poseAtual[k] - poseAnt[k]);
        penalidadeTotal += movimento * 0.5;
    }

    ind.trajetoria = trajetoriaPontiforme;

    if (ind.venceu) {
        ind.fitness = bonusObjetivo - penalidadeTotal;
    } else {
        ind.fitness = -penalidadeTotal - distFinal * 1000;
    }
    return ind.fitness;
}