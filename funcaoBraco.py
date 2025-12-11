import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# --- Configurações do Robô ---
L1 = 1
L2 = 1

# --- Definição do Domínio (Geração de Pontos) ---
num_points = 30000

# Domínio da Base: 0 a 360 graus 
theta1 = np.random.uniform(-np.pi, np.pi, num_points)

# Domínio do Ombro: 0 a 90 graus 
theta2 = np.random.uniform(0, np.pi/2, num_points) 

# Domínio do Cotovelo: -180 a 0 graus
theta3 = np.random.uniform(-np.pi, 0, num_points)

# --- A Função f(t1, t2, t3) -> (x, y, z) ---
def forward_kinematics(t1, t2, t3, l1, l2):
    # Projeção no plano vertical do braço
    # r_proj é a distância horizontal da base até a ponta
    r_proj = l1 * np.cos(t2) + l2 * np.cos(t2 + t3)
    
    # Altura Z
    z = l1 * np.sin(t2) + l2 * np.sin(t2 + t3)
    
    # Coordenadas X e Y baseadas na rotação da base
    x = r_proj * np.cos(t1)
    y = r_proj * np.sin(t1)
    
    return x, y, z, r_proj

# Calcula todos os pontos
x, y, z, r_dist = forward_kinematics(theta1, theta2, theta3, L1, L2)

# --- Plotagem ---
fig = plt.figure(figsize=(14, 6))

# Calculamos o raio máximo absoluto para travar os eixos
max_reach = L1 + L2

# --- PLOT 1: Visualização 3D Isotrópica ---
ax1 = fig.add_subplot(121, projection='3d')
ax1.set_title(f"Volume Alcançável 3D (Escala Real)\nL1={L1}, L2={L2}")

# Filtro de corte (para ver o interior)
mask = (y > 0) | (x > 0) 

# Plotagem
sc = ax1.scatter(x[mask], y[mask], z[mask], c=z[mask], cmap='viridis', s=1, alpha=0.3)

# Referência da Base
ax1.scatter([0], [0], [0], color='red', s=100, label='Base')

ax1.set_xlim(-max_reach, max_reach)
ax1.set_ylim(-max_reach, max_reach)
ax1.set_zlim(-max_reach, max_reach) 

ax1.set_box_aspect([1,1,1]) 

ax1.set_xlabel('X')
ax1.set_ylabel('Y')
ax1.set_zlabel('Z')

# --- PLOT 2: Perfil Lateral 2D (Raio vs Altura) ---
ax2 = fig.add_subplot(122)
ax2.set_title("Perfil Lateral (Corte Transversal)\nMostra Zonas Mortas")

# Plotamos a Distância Radial (distância do centro) vs Altura Z
ax2.scatter(r_dist, z, c='blue', s=1, alpha=0.1, label='Pontos Alcançáveis')

# Adicionando linhas de limites teóricos para explicação
ax2.axvline(x=0, color='k', linestyle='--', alpha=0.5) # Eixo Z
ax2.axhline(y=0, color='k', linestyle='-', alpha=0.5)  # Chão

# Anotações das Zonas Mortas
circle1 = plt.Circle((0, 0), L1-L2, color='red', fill=False, linewidth=2, linestyle='--')
ax2.add_patch(circle1)

# Configurações do gráfico 2D
ax2.set_xlabel('Distância Radial Horizontal (R)')
ax2.set_ylabel('Altura (Z)')
ax2.set_xlim(-0.5, L1+L2+0.5)
ax2.set_ylim(-1, L1+L2+0.5)
ax2.set_aspect('equal')
ax2.grid(True, alpha=0.3)

plt.tight_layout()
plt.show()
