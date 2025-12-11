#include "Config.h"

// Criação da variável global de configurações
Config c;

Config::Config() {
    nGenes = nWaypoints;

    // Inicializa probabilidades uniformes
    if (nGenes > 0) {
        listaPNumGene.assign(nGenes, 1.0/nGenes);
        listaPCadaGene.assign(nGenes, 1.0/nGenes);
    }
}