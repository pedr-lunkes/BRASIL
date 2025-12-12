# ==========================================
# DETECÇÃO DO SISTEMA OPERACIONAL
# ==========================================
ifeq ($(OS),Windows_NT)
    # Configurações para Windows
    TARGET = main.exe
    # 'del' é o comando do Windows. /Q evita perguntar "tem certeza?"
    # 'if exist' evita erro se não houver arquivos para apagar
    CLEAN_CMD = if exist *.o del /Q *.o
    # No Windows geralmente é apenas 'python'
    PYTHON_CMD = python
else
    # Configurações para Linux / Mac
    TARGET = main
    # 'rm -f' apaga sem perguntar e não reclama se o arquivo não existir
    CLEAN_CMD = rm -f *.o
    # No Linux geralmente precisa especificar 'python3'
    PYTHON_CMD = python3
endif


# ==========================================
# CONFIGURAÇÕES DE COMPILAÇÃO
# ==========================================
CXX = g++
CXXFLAGS = -std=c++11


# Lista de objetos (compilados parciais)
OBJS = Config.o Evolution.o Robot.o Utils.o main.o


# ==========================================
# REGRAS (TARGETS)
# ==========================================


# Regra padrão: Compila tudo e depois limpa os .o
all: compile clean


# Linkagem final: Junta todos os .o no executável final
compile: $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS)


# Regra Genérica: Ensina o make a transformar qualquer .cpp em .o
# $< é o arquivo de entrada (.cpp) e $@ é a saída (.o)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


# Limpeza (usa o comando detectado lá em cima)
clean:
	$(CLEAN_CMD)


# Rodar a simulação (usa o python detectado)
run:
	$(PYTHON_CMD) simulation.py
