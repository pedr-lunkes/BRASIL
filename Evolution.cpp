#include "Evolution.h"
#include "Config.h"
#include "Utils.h"
#include <algorithm>
#include <numeric>

using namespace std;

Individuo gerarIndividuo() {
    vector<vector<double>> genoma;
    for (int i = 0; i < c.nGenes; i++) {
        vector<double> gene;
        for(int j=0; j<c.nJuntas; j++){
            gene.push_back(escolherNumReal(-c.speed, c.speed));
        }
        genoma.push_back(gene);
    }
    return Individuo(genoma);
}

Individuo realizarMutacao(Individuo ind) {
    int qtdeMutados = escolherIndiceDeProbabilidades(c.listaPNumGene) + 1;
    
    vector<int> genesParaMutar;
    vector<int> indicesDisponiveis(c.nGenes); //100
    iota(indicesDisponiveis.begin(), indicesDisponiveis.end(), 0);
    shuffle(indicesDisponiveis.begin(), indicesDisponiveis.end(), rng);
    
    for(int i=0; i<qtdeMutados && i < (int)indicesDisponiveis.size(); i++) {
        genesParaMutar.push_back(indicesDisponiveis[i]);
    }

    for (int idx : genesParaMutar) {
        double sinal = escolherZeroUm(c.pMutPos) ? 1.0 : -1.0;
        double mutacao = 0;

        if (c._mut == "_mut_pad") {
            mutacao = sinal * c.mutBase;
        } 
        else if (c._mut == "_mut_acu") {
            mutacao = sinal * (c.mutBase + c.incMutBase * incAtual);
        }
        else if (c._mut == "_mut_acl") {
            if (c.incMutBase * incAtual > c.tetoMut) alterarIncrementoDaMutacaoAtual(true);
            mutacao = sinal * (c.mutBase + c.incMutBase * incAtual);
        }
        else if (c._mut == "_mut_cao") {
            mutacao = sinal * escolherNumReal(0, 30.0); 
        }

        int r = escolherIndiceDeLista(c.nJuntas);
        ind.genoma[idx][r] += mutacao;

        if (ind.genoma[idx][r] < c.baseLmin[r]) 
            ind.genoma[idx][r] = c.baseLmin[r];
        if (ind.genoma[idx][r] > c.baseLmax[r])
            ind.genoma[idx][r] = c.baseLmax[r];
    }
    return ind;
}

Individuo realizarCruzamento(const Individuo& pai1, const Individuo& pai2) {
    vector<vector<double>> novoGenoma;
    for(int i=0;i<c.nGenes;i++){
        vector<double> passoFilho;
        for(int j=0;j<c.nJuntas;j++){
            double gene = (pai1.genoma[i][j] + pai2.genoma[i][j]) / 2.0;
            passoFilho.push_back(gene);
        }
        novoGenoma.push_back(passoFilho);
    }
    return Individuo(novoGenoma);
}

vector<Individuo> realizarCatastrofe(vector<Individuo>& pop) {
    vector<Individuo> popNova;
    
    vector<pair<double, int>> aux;
    for(size_t i=0; i<pop.size(); i++) aux.push_back({pop[i].fitness, (int)i});
    sort(aux.rbegin(), aux.rend()); 

    int inicioZonaMorte = c.nMortosCat; 

    for (int i = 0; i < inicioZonaMorte; i++) 
        popNova.push_back(pop[aux[i].second]);

    while (popNova.size() < pop.size()) {
        popNova.push_back(gerarIndividuo());
    }
    return popNova;
}

vector<Individuo> selecaoPorRoleta(vector<Individuo>& pop) {
    int idxPior = 0, idxMelhor = 0;
    for(size_t i=1; i<pop.size(); i++) {
        if(pop[i].fitness < pop[idxPior].fitness) idxPior = i;
        if(pop[i].fitness > pop[idxMelhor].fitness) idxMelhor = i;
    }

    vector<Individuo> novaPop;
    novaPop.push_back(pop[idxMelhor]); 

    vector<double> roleta;
    double fitnessPior = pop[idxPior].fitness;
    double normalizacao = (fitnessPior <= 0) ? (-fitnessPior + 1) : 0;

    for(const auto& ind : pop) {
        roleta.push_back(ind.fitness + normalizacao); 
    }

    int genitor1Idx = escolherIndiceDeProbabilidades(roleta);
    int genitor2Idx = escolherIndiceDeProbabilidades(roleta);
    
    novaPop.push_back(pop[genitor1Idx]);
    novaPop.push_back(pop[genitor2Idx]);

    while(novaPop.size() < pop.size()) {
        Individuo filho = realizarCruzamento(pop[genitor1Idx], pop[genitor2Idx]);
        filho = realizarMutacao(filho);
        novaPop.push_back(filho);
    }
    return novaPop;
}