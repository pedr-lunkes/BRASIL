#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <random>

// Variáveis Globais de Estado
extern std::mt19937 rng;
extern int estagAtual;
extern double incAtual;

// Funções Auxiliares
double escolherNumReal(double min, double max);
bool escolherZeroUm(double prob);
int escolherIndiceDeProbabilidades(const std::vector<double>& probs);
int escolherIndiceDeLista(int size);

// Função que controla a agressividade da mutação
void alterarIncrementoDaMutacaoAtual(bool resetar);

#endif