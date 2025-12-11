#ifndef CONFIG_H
#define CONFIG_H

#include <vector>
#include <string>
using namespace std;

struct Config {
    int nIndv = 100;           
    int nMortosCat = 95;       
    double pCat = 0.01;        
    
    int nWaypoints = 100;      
    int nJuntas = 3;
    int nGenes; // Calculado no construtor

    // Quantidades de gerações que serão mandados para o print (simulador)
    int printGeracoes = 3;
    
    // Limites
    vector<double> baseLmin = {-180.0, 0.0, 0.0};
    vector<double> baseLmax = {180.0, 90.0, 180.0};
    double speed = 5;
    
    // Mutação
    double pMutPos = 0.5;          
    double mutBase = 0.2;          
    double incMutBase = 0.5;       
    double tetoMut = 6.0;      
    int minEstag = 1;   
    
    // Catástrofe e Seleção
    string _cat = "_cat_dis"; 
    int minEstagCat = 6;
    string _sel = "_sel_rol"; 

    // Probabilidades
    vector<double> listaPNumGene; 
    vector<double> listaPCadaGene;

    Config(); // Construtor
};

// Declaração da variável global 'c' (definida no .cpp)
extern Config c;

#endif
