#include "Config.h"

// Criação da variável global de configurações
Config c;

Config::Config() {
    nGenes = nWaypoints * nJuntas;
    
    // Inicializa limites padrões (serão sobrescritos na main, mas é bom ter padrão)
    genesLmin = {-180.0, 0, 0};
    genesLmax = {180.0, 90.0, 180.0};

    // Inicializa probabilidades uniformes
    if (nGenes > 0) {
        listaPNumGene.assign(nGenes, 1.0/nGenes);
        listaPCadaGene.assign(nGenes, 1.0/nGenes);
    }
}