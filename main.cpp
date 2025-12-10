#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>
#include <numeric>
#include <iomanip>
using namespace std;

// ==========================================
// 1. CONFIGURAÇÃO E ESTRUTURAS DE DADOS
// ==========================================

// Configurações Globais (Mapeando o objeto 'c' do texto)
struct Config {
    int nIndv = 100;           // Tamanho da população          
    int nMortosCat = 30;       // Indivíduos mortos na catástrofe
    double pCat = 0.05;        // Probabilidade de catástrofe (5%)
    int nWaypoints = 100;      // Número de pontos de passagem 
    int nJuntas = 3;
    int nGenes = nWaypoints * nJuntas;
    
    // Limites dos genes (ângulos em graus, ex: -180 a 180) 
    vector<double> genesLmin = {-180.0, 0, 0};
    vector<double> genesLmax = {180.0, 90.0, 180.0};
    
    // Mutação
    string _mut = "_mut_acu"; // _mut_pad, _mut_acu, _mut_acl, _mut_cao
    double pMutPos = 0.5;          // Chance de sinal positivo
    double mutBase = 1.0;          // Taxa base de mutação (1 grau)
    double incMutBase = 0.5;       // Incremento por estagnação
    double tetoMut = 10.0;         // Teto para mutação acumulativa limitada
    
    // Catástrofe
    string _cat = "_cat_dis"; // _cat_apt, _cat_prg, _cat_dis, _cat_est

    // Seleção
    string _sel = "_sel_rol"; // _sel_elit, _sel_rol, _sel_torn

    // Probabilidade de quantos genes mutar (simplificado: pesos iguais)
    vector<double> listaPNumGene; 
    // Probabilidade de qual gene mutar
    vector<double> listaPCadaGene;

    Config() {
        // Inicializa probabilidades uniformes para simplificação
        listaPNumGene.assign(nGenes, 1.0/nGenes);
        listaPCadaGene.assign(nGenes, 1.0/nGenes);
    }
} c;

struct Obstaculo {
    double x, y, z, raio;
};
Obstaculo bolaDeDemolicao = {-1, -1, -1, 0}; // Uma bola na posição 5,5,5

// Estrutura do Indivíduo
struct Individuo {
    vector<double> genes;
    double fitness;
    int passoVitoria;
    bool venceu;
    
    // Construtor padrão
    Individuo() {}
    Individuo(vector<double> g) : genes(g), fitness(-1e9) {}
};

// Variáveis de Estado do Algoritmo
Individuo melhorGeral;
int estagAtual = 0;
double incAtual = 0.0;
mt19937 rng(random_device{}()); // Gerador de números aleatórios

// ==========================================
// 2. FÍSICA DO BRAÇO (Cinemática Direta)
// ==========================================

struct Ponto { double x, y, z; };

// Retorna a posição final da mão baseada nos ângulos (genes)
Ponto cinematicaDireta(const vector<double>& angulos) {
    // Comprimentos dos segmentos
    double L_umero = 10.0;    // Do ombro ao cotovelo
    double L_antebraco = 10.0; // Do cotovelo à mão

    // Mapeamento dos Genes para nomes legíveis
    double angulo_base    = angulos[0] * (M_PI / 180.0); // Rotação no plano XY
    double angulo_ombro   = angulos[1] * (M_PI / 180.0); // Elevação (Z)
    double angulo_cotovelo= angulos[2] * (M_PI / 180.0); // Relativo ao úmero

    // PASSO 1: Calcular a geometria no plano vertical (formado pelo braço levantado)
    // O ângulo global do antebraço é a soma do ombro + cotovelo (ou subtração, dependendo do referencial).
    // Vamos assumir que cotovelo 0 é esticado alinhado com o úmero.
    
    // Projeção horizontal (raio) do ombro até o cotovelo
    double r_cotovelo = L_umero * cos(angulo_ombro);
    double z_cotovelo = L_umero * sin(angulo_ombro);
    
    // Ajuste físico: Ângulo absoluto do antebraço = Ombro - Cotovelo (para dobrar "fechando")
    double angulo_abs = angulo_ombro - angulo_cotovelo; // angulo do antebraço em relação ao horizonte

    double r_total = L_umero * cos(angulo_ombro) + L_antebraco * cos(angulo_abs);
    double z_total = L_umero * sin(angulo_ombro) + L_antebraco * sin(angulo_abs);

    // PASSO 2: Projetar o raio total (r_total) no espaço 3D usando a Base
    double x = r_total * cos(angulo_base);
    double y = r_total * sin(angulo_base);
    double z = z_total;

    return {x, y, z};
}

bool verificarColisao(Ponto p) {
    // Distância do ponto até o centro da bola
    double dist = sqrt(pow(p.x - bolaDeDemolicao.x, 2) + 
                            pow(p.y - bolaDeDemolicao.y, 2) + 
                            pow(p.z - bolaDeDemolicao.z, 2));
    return dist < bolaDeDemolicao.raio; // Se for menor, bateu
}

// Ponto Inicial fixo (onde o braço começa)
vector<double> poseInicial = {0.0, 90.0, 0.0}; 

double calcularFitness(Individuo& ind, Ponto alvo) {
    double penalidadeTotal = 0.0;
    double bonusObjetivo = 0.0;
    
    // Vetor para guardar a trajetória completa (Inicio -> P1 -> P2 -> ... -> Fim)
    vector<vector<double>> trajetoria;
    trajetoria.push_back(poseInicial);

    // 1. Decodificar os Genes em Poses (Waypoints)
    for (int i = 0; i < c.nWaypoints; i++) {
        vector<double> pose;
        for (int j = 0; j < c.nJuntas; j++) {
            pose.push_back(ind.genes[i * c.nJuntas + j]);
        }
        trajetoria.push_back(pose);
    }

    double distMinGlobal = 1e9;

    // 2. Avaliar a Trajetória
    for (size_t i = 1; i < trajetoria.size(); i++) {
        vector<double>& poseAnt = trajetoria[i-1];
        vector<double>& poseAtual = trajetoria[i];

        // A. Calcular posição cartesiana desta pose
        Ponto p = cinematicaDireta(poseAtual);

        double dist = sqrt(pow(p.x - alvo.x, 2) + pow(p.y - alvo.y, 2) + pow(p.z - alvo.z, 2));

        if (dist < distMinGlobal) distMinGlobal = dist;

        if (dist < 1.0) {
            ind.venceu = true;
            ind.passoVitoria = (int)i;

            bonusObjetivo = 50000.0 + (c.nWaypoints - i) * 100.0;
            break;
        }

        penalidadeTotal += dist * 1.0;

        // B. Checar Colisão (Penalidade Brutal)
        if (verificarColisao(p)) {
            penalidadeTotal += 2000.0; 
        }


        // C. Penalizar movimento excessivo (Eficiência)
        // Soma a diferença angular de cada junta
        double movimento = 0;
        for(int k=0; k<3; k++) movimento += abs(poseAtual[k] - poseAnt[k]);
        penalidadeTotal += movimento * 0.5; // Peso leve
    }

    if (ind.venceu) {
        ind.fitness = bonusObjetivo - penalidadeTotal;
    } else {
        ind.fitness = -penalidadeTotal - (distMinGlobal * 100.0);
    }
    return ind.fitness;
}

// ==========================================
// 3. FUNÇÕES AUXILIARES (Matemática e Random)
// ==========================================

double escolherNumReal(double min, double max) {
    uniform_real_distribution<double> dist(min, max);
    return dist(rng);
}

bool escolherZeroUm(double prob) {
    bernoulli_distribution d(prob);
    return d(rng);
}

int escolherIndiceDeProbabilidades(const vector<double>& probs) {
    discrete_distribution<int> d(probs.begin(), probs.end());
    return d(rng);
}

int escolherIndiceDeLista(int size) {
    uniform_int_distribution<int> d(0, size - 1);
    return d(rng);
}

void alterarIncrementoDaMutacaoAtual(bool resetar) {
    if (resetar) incAtual = 0;
    else incAtual++; // Aumenta o fator multiplicativo
    estagAtual = 0;
}

// ==========================================
// 4. LÓGICA DO ALGORITMO EVOLUTIVO
// ==========================================

Individuo gerarIndividuo() {
    vector<double> genes;
    for (int i = 0; i < c.nGenes; i++) {
        genes.push_back(escolherNumReal(c.genesLmin[i], c.genesLmax[i]));
    }
    return Individuo(genes);
}

// Mutação conforme descrito (Padrão, Acumulativa, Limitada, Caótica)
Individuo realizarMutacao(Individuo ind) {
    // Sorteia se vamos mutar 1, 2 ou 3 genes
    int qtdeMutados = escolherIndiceDeProbabilidades(c.listaPNumGene) + 1;
    
    // Sorteia quais genes mutar
    vector<int> genesParaMutar;
    vector<int> indicesDisponiveis(c.nGenes);
    iota(indicesDisponiveis.begin(), indicesDisponiveis.end(), 0);
    shuffle(indicesDisponiveis.begin(), indicesDisponiveis.end(), rng);
    
    for(int i=0; i<qtdeMutados && i < (int)indicesDisponiveis.size(); i++) {
        genesParaMutar.push_back(indicesDisponiveis[i]);
    }

    // Aplica mutação
    for (int idx : genesParaMutar) {
        double sinal = escolherZeroUm(c.pMutPos) ? 1.0 : -1.0;
        double mutacao = 0;

        if (c._mut == "_mut_pad") { // Mutação Padrão
            mutacao = sinal * c.mutBase;
        } 
        else if (c._mut == "_mut_acu") { // Mutação Acumulativa
            mutacao = sinal * (c.mutBase + c.incMutBase * incAtual);
        }
        else if (c._mut == "_mut_acl") { // Mutação Acumulativa Limitada
            if (c.incMutBase * incAtual > c.tetoMut) alterarIncrementoDaMutacaoAtual(true);
            mutacao = sinal * (c.mutBase + c.incMutBase * incAtual);
        }
        else if (c._mut == "_mut_cao") { // Mutação Caótica
            mutacao = sinal * escolherNumReal(0, 30.0); 
        }

        ind.genes[idx] += mutacao;

        // Limitação
        if (ind.genes[idx] < c.genesLmin[idx]) ind.genes[idx] = c.genesLmin[idx];
        if (ind.genes[idx] > c.genesLmax[idx]) ind.genes[idx] = c.genesLmax[idx];
    }
    return ind;
}

// Cruzamento (Média Aritmética)
Individuo realizarCruzamento(const Individuo& pai1, const Individuo& pai2) {
    vector<double> novosGenes(c.nGenes);
    for (int i = 0; i < c.nGenes; i++) {
        novosGenes[i] = (pai1.genes[i] + pai2.genes[i]) / 2.0;
    }
    return Individuo(novosGenes);
}

// ==========================================
// 5. CATÁSTROFES
// ==========================================

vector<Individuo> realizarCatastrofe(vector<Individuo>& pop) {
    vector<Individuo> popNova;
    
    // Ordena população auxiliar para saber quem matar
    // Ordena do PIOR para o MELHOR (fitness crescente)
    vector<pair<double, int>> aux;
    for(size_t i=0; i<pop.size(); i++) aux.push_back({pop[i].fitness, (int)i});
    sort(aux.begin(), aux.end()); // [0] é o pior, [end] é o melhor

    // O melhor indivíduo NUNCA é substituído (elitismo forçado na catástrofe)
    // Mas a lógica abaixo seleciona quem VIVE para a popNova

    if (c._cat == "_cat_apt") { // Mata os piores (mantém os do final da lista ordenada)
        for (int i = c.nMortosCat; i < (int)pop.size(); i++) 
            popNova.push_back(pop[aux[i].second]);
    }
    else if (c._cat == "_cat_prg") { // Mata os melhores (exceto o absoluto, talvez?)
        // Texto diz: "Mata os n melhores". Mantém os piores.
        for (int i = 0; i < (int)pop.size() - c.nMortosCat; i++) 
            popNova.push_back(pop[aux[i].second]);
    }
    else if (c._cat == "_cat_dis") { // Mata os medianos
        int mortos = c.nMortosCat;
        int inicioZonaMorte = (pop.size() - mortos) / 2; // Aproximação do centro
        
        // Adiciona os piores (antes do meio)
        for (int i = 0; i < inicioZonaMorte; i++) 
            popNova.push_back(pop[aux[i].second]);
        // Adiciona os melhores (depois do meio)
        for (int i = inicioZonaMorte + mortos; i < (int)pop.size(); i++)
            popNova.push_back(pop[aux[i].second]);
    }
    else if (c._cat == "_cat_est") { // Mata os extremos (piores e melhores)
        int matarPonta = c.nMortosCat / 2;
        // Salva apenas o miolo
        for (int i = matarPonta; i < (int)pop.size() - matarPonta; i++)
             popNova.push_back(pop[aux[i].second]);
    }

    // Recompõe a população com novos aleatórios
    while (popNova.size() < pop.size()) {
        popNova.push_back(gerarIndividuo());
    }
    
    // Garante que o melhor geral esteja lá se ele não foi selecionado pela lógica
    // (Opcional, mas a lógica JS sugere preservação do melhorGeral em memória separada)
    
    return popNova;
}

// ==========================================
// 6. MÉTODOS DE SELEÇÃO
// ==========================================

vector<Individuo> selecaoPorRoleta(vector<Individuo>& pop) {
    // Identifica pior e melhor índice
    int idxPior = 0, idxMelhor = 0;
    for(size_t i=1; i<pop.size(); i++) {
        if(pop[i].fitness < pop[idxPior].fitness) idxPior = i;
        if(pop[i].fitness > pop[idxMelhor].fitness) idxMelhor = i;
    }

    vector<Individuo> novaPop;
    novaPop.push_back(pop[idxMelhor]); // Elitismo básico

    // Normalização da Roleta
    vector<double> roleta;
    double fitnessPior = pop[idxPior].fitness;
    double normalizacao = (fitnessPior <= 0) ? (-fitnessPior + 1) : 0; // Se todos > 0, normalizacao pode ser 0 ou manter lógica

    for(const auto& ind : pop) {
        roleta.push_back(ind.fitness + normalizacao); 
    }

    // Seleciona 2 pais
    int genitor1Idx = escolherIndiceDeProbabilidades(roleta);
    int genitor2Idx = escolherIndiceDeProbabilidades(roleta);
    
    // Adiciona os genitores (conforme texto, eles entram na nova geração? 
    // O texto diz "gerarão nIndv-3 indivíduos", implicando que pais + melhor entram).
    novaPop.push_back(pop[genitor1Idx]);
    novaPop.push_back(pop[genitor2Idx]);

    while(novaPop.size() < pop.size()) {
        Individuo filho = realizarCruzamento(pop[genitor1Idx], pop[genitor2Idx]);
        filho = realizarMutacao(filho);
        novaPop.push_back(filho);
    }
    return novaPop;
}

// ==========================================
// 7. LOOP PRINCIPAL (MAIN)
// ==========================================

int main() {
    vector<double> baseLmin = {-180.0, 0.0, 0.0};
    vector<double> baseLmax = {180.0, 90.0, 180.0};
    
    c.genesLmin.clear();
    c.genesLmax.clear();
    
    // Configura os limites para TODOS os waypoints
    for(int i=0; i<c.nWaypoints; i++) {
        c.genesLmin.insert(c.genesLmin.end(), baseLmin.begin(), baseLmin.end());
        c.genesLmax.insert(c.genesLmax.end(), baseLmax.begin(), baseLmax.end());
    }
    
    // Atualiza probabilidade de mutação
    c.listaPNumGene.assign(c.nGenes, 1.0/c.nGenes);
    c.listaPCadaGene.assign(c.nGenes, 1.0/c.nGenes);
    
    Ponto alvo = {0.0, 0.0, 20.0};
    
    vector<Individuo> pop;
    for(int i=0; i<c.nIndv; i++) pop.push_back(gerarIndividuo());

    for(auto& ind : pop) calcularFitness(ind, alvo);
    melhorGeral = pop[0];

    int geracoes = 1;
    int maxGeracoes = 2000;

    cout << "========================================\n";
    cout << "   EVOLUCAO DE TRAJETORIA ROBOTICA 3D   \n";
    cout << "========================================\n";
    cout << "Waypoints (Passos): " << c.nWaypoints << "\n";
    cout << "Total de Genes    : " << c.nGenes << "\n";
    cout << "Alvo Desejado     : (" << alvo.x << ", " << alvo.y << ", " << alvo.z << ")\n";
    cout << "Obstaculo (Bola)  : (" << bolaDeDemolicao.x << ", " << bolaDeDemolicao.y 
         << ", " << bolaDeDemolicao.z << ") Raio: " << bolaDeDemolicao.raio << "\n";
    cout << "----------------------------------------\n\n";

    while (geracoes) {

        int idxMelhorLocal = 0;
        for(size_t i=0; i<pop.size(); i++) {
            calcularFitness(pop[i], alvo);
            if(pop[i].fitness > pop[idxMelhorLocal].fitness) idxMelhorLocal = i;
        }

        Individuo& melhorLocal = pop[idxMelhorLocal];

        if (abs(melhorLocal.fitness - melhorGeral.fitness) < 0.1 && 
           (c._mut == "_mut_acu" || c._mut == "_mut_acl")) {
            estagAtual++;
            int limiteEstagnacao = 3; 
            if (estagAtual >= limiteEstagnacao) alterarIncrementoDaMutacaoAtual(false);
        } 
        else if (melhorLocal.fitness > melhorGeral.fitness) {
            melhorGeral = melhorLocal;
            if (c._mut == "_mut_acu" || c._mut == "_mut_acl") alterarIncrementoDaMutacaoAtual(true);
        }

        if (escolherZeroUm(c.pCat)) {
            if (c._mut == "_mut_acu" || c._mut == "_mut_acl") alterarIncrementoDaMutacaoAtual(true);
            pop = realizarCatastrofe(pop);
            for(auto& ind : pop) calcularFitness(ind, alvo);
            geracoes++;
            continue; 
        }

        if (c._sel == "_sel_rol") pop = selecaoPorRoleta(pop);
        
        // Critério de parada (Fitness próximo de 0 é perfeito)
        if (melhorGeral.fitness > -0.5) {
            cout << ">>> Solucao otima encontrada antes do limite de geracoes! <<<\n";
            break;
        }

        geracoes++;
        
        // Relatório a cada 100 gerações
        if (geracoes % 100 == 0) {
            cout << "Geracao " << setw(5) << geracoes 
                 << " | Fitness: " << setw(10) << fixed << setprecision(4) << melhorGeral.fitness 
                 << " | Estagnacao: " << setw(3) << estagAtual 
                 << " | Fator Mutacao: " << incAtual << "x\n";
        }
    }

    // relatório final
    cout << "\n========================================\n";
    cout << "         RESULTADO DA SIMULACAO         \n";
    cout << "========================================\n";
    
    // Reconstrói e imprime passo a passo
    cout << fixed << setprecision(2);
    
    // Ponto inicial fixo (hardcoded ou vindo de config)
    vector<double> poseZero = {0.0, 90.0, 0.0}; 
    Ponto pZero = cinematicaDireta(poseZero);
    cout << "[Inicio] -> Pos: (" << pZero.x << ", " << pZero.y << ", " << pZero.z << ")\n";

    // Itera sobre os waypoints do melhor indivíduo
    for(int w = 0; w < c.nWaypoints; w++) {
        // Extrai os 3 genes deste passo
        vector<double> posePasso;
        int baseIdx = w * c.nJuntas;
        
        posePasso.push_back(melhorGeral.genes[baseIdx]);     // Base
        posePasso.push_back(melhorGeral.genes[baseIdx + 1]); // Ombro
        posePasso.push_back(melhorGeral.genes[baseIdx + 2]); // Cotovelo

        Ponto p = cinematicaDireta(posePasso);
        
        // Verifica se bateu (apenas para informar no print)
        bool bateu = verificarColisao(p); 
        
        cout << "[Passo " << w+1 << "] -> "
             << "Pos: (" << setw(6) << p.x << ", " << setw(6) << p.y << ", " << setw(6) << p.z << ") "
             << "| Angulos: [" << setw(6) << posePasso[0] << ", " << setw(6) << posePasso[1] << ", " << setw(6) << posePasso[2] << "] "
             << (bateu ? " <<< COLISAO!" : "") 
             << "\n";
    }

    // Calcula distância final real
    // (Recalcula ultimo ponto)
    vector<double> ultimaPose;
    for(int i=0; i<3; i++) ultimaPose.push_back(melhorGeral.genes[(c.nWaypoints-1)*3 + i]);
    Ponto finalReal = cinematicaDireta(ultimaPose);
    
    double distFinal = sqrt(pow(finalReal.x - alvo.x, 2) + pow(finalReal.y - alvo.y, 2) + pow(finalReal.z - alvo.z, 2));

    cout << "----------------------------------------\n";
    cout << "Distancia Final ao Alvo: " << distFinal << "\n";
    cout << "Fitness Total          : " << melhorGeral.fitness << "\n";
    cout << "========================================\n";

    return 0;
}
