#include "Utils.h"

// Definição das variáveis globais
std::mt19937 rng(std::random_device{}());
int estagAtual = 0;
double incAtual = 0.0;

void alterarIncrementoDaMutacaoAtual(bool resetar) {
    if (resetar) incAtual = 0;
    else incAtual++;
    estagAtual = 0;
}

double escolherNumReal(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}

bool escolherZeroUm(double prob) {
    std::bernoulli_distribution d(prob);
    return d(rng);
}

int escolherIndiceDeProbabilidades(const std::vector<double>& probs) {
    std::discrete_distribution<int> d(probs.begin(), probs.end());
    return d(rng);
}

int escolherIndiceDeLista(int size) {
    std::uniform_int_distribution<int> d(0, size - 1);
    return d(rng);
}