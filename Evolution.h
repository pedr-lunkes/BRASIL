#ifndef EVOLUTION_H
#define EVOLUTION_H

#include "Types.h"
#include <vector>

Individuo gerarIndividuo();
Individuo realizarMutacao(Individuo ind);
Individuo realizarCruzamento(const Individuo& pai1, const Individuo& pai2);
std::vector<Individuo> realizarCatastrofe(std::vector<Individuo>& pop);
std::vector<Individuo> selecaoPorRoleta(std::vector<Individuo>& pop);

#endif
