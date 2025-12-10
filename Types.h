#ifndef TYPES_H
#define TYPES_H

#include <vector>

// Estrutura para pontos no espaço 3D
struct Ponto { 
    double x, y, z; 
};

// Estrutura para obstáculos
struct Obstaculo {
    double x, y, z, raio;
};

// Estrutura do Indivíduo (Solução)
struct Individuo {
    std::vector<double> genes;
    double fitness;
    int passoVitoria;
    bool venceu;
    
    Individuo() : fitness(-1e9), passoVitoria(0), venceu(false) {}
    Individuo(std::vector<double> g) : genes(g), fitness(-1e9), passoVitoria(0), venceu(false) {}
};

#endif