#ifndef ROBOT_H
#define ROBOT_H

#include "Types.h"
#include <vector>

// Objetos globais do ambiente (Obstaculo e Pose Inicial)
extern Obstaculo bolaDeDemolicao;
extern std::vector<double> poseInicial;

Ponto cinematicaDireta(const std::vector<double>& angulos);
bool verificarColisao(Ponto p);
double calcularFitness(Individuo& ind, Ponto alvo);

#endif