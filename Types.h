#ifndef TYPES_H
#define TYPES_H

#include <vector>
using namespace std;

// Estrutura para pontos no espaço 3D
struct Ponto { 
    double x, y, z; 
};

// Estrutura para obstáculos
struct Obstaculo {
    double x, y, z, raio;
};

// Estrutura do Indivíduo
struct Individuo {
    vector<vector<double>> genoma;
    double fitness;
    int passoVitoria;
    bool venceu;
    
    Individuo() : fitness(-1e9), passoVitoria(0), venceu(false) {}
    Individuo(vector<vector<double>> g) : genoma(g), fitness(-1e9), passoVitoria(0), venceu(false) {}
};

#endif