import pygame
from pygame.locals import *
from OpenGL.GL import *
from OpenGL.GLU import *
import numpy as np
import math
from collections import deque

# --- Configurações Físicas do Robô ---
LINK_1 = 2.0
LINK_2 = 2.1


# --- Classe Câmera ---
class Camera:
    def __init__(self):
        self.yaw = 45.0  
        self.pitch = 30.0 
        self.distance = 12.0
        self.sensitivity = 0.5
        self.zoom_speed = 0.5

    def handle_input(self, event):
        if event.type == pygame.MOUSEMOTION:
            if pygame.mouse.get_pressed()[0]:
                dx, dy = event.rel
                self.yaw -= dx * self.sensitivity
                self.pitch += dy * self.sensitivity
                # Limita para não virar de ponta cabeça
                self.pitch = max(min(self.pitch, 89.0), -10.0)
        
        if event.type == pygame.MOUSEWHEEL:
            self.distance -= event.y * self.zoom_speed
            if self.distance < 2.0: self.distance = 2.0

    def apply(self):
        glMatrixMode(GL_MODELVIEW)
        glLoadIdentity()
        
        # Z é altura, XY é plano horizontal
        rad_yaw = math.radians(self.yaw)
        rad_pitch = math.radians(self.pitch)
        
        cam_x = self.distance * math.cos(rad_pitch) * math.cos(rad_yaw)
        cam_y = self.distance * math.cos(rad_pitch) * math.sin(rad_yaw)
        cam_z = self.distance * math.sin(rad_pitch)
        
        gluLookAt(cam_x, cam_y, cam_z, 0, 0, 0, 0, 0, 1)


# --- Matemática do Robô ---
def calculate_ik(target_x, target_y, target_z):
    # Base (livre 360°)
    theta_base = math.atan2(target_y, target_x)
    r_ground = math.sqrt(target_x**2 + target_y**2)
    
    # Não deixa o braço ir para de baixo do plano XY
    z_height = max(target_z, 0.0) 

    dist_sq = r_ground**2 + z_height**2
    dist = math.sqrt(dist_sq)

    # Restrição Física: Alcance Máximo do Braço
    if dist > (LINK_1 + LINK_2):
        dist = LINK_1 + LINK_2
        dist_sq = dist**2

    try:
        alpha = math.atan2(z_height, r_ground)
        
        # Ombro (Cálculo do ângulo interno)
        cos_shoulder = (LINK_1**2 + dist_sq - LINK_2**2) / (2 * LINK_1 * dist)
        theta_shoulder_internal = math.acos(cos_shoulder)
        
        # Ângulo bruto do ombro
        raw_theta_shoulder = alpha + theta_shoulder_internal

        # Cotovelo (Cálculo do ângulo interno)
        cos_elbow = (LINK_1**2 + LINK_2**2 - dist_sq) / (2 * LINK_1 * LINK_2)
        raw_theta_elbow = math.acos(cos_elbow) - math.pi 

        # --- Restrições ---
        # RESTRIÇÃO DO OMBRO: 0 a 90 graus (0 a PI/2 radianos)
        theta_shoulder = max(0.0, min(raw_theta_shoulder, math.pi / 2))

        # RESTRIÇÃO DO COTOVELO: 180 graus de liberdade
        theta_elbow = max(-math.pi, min(raw_theta_elbow, 0.0))

        return theta_base, theta_shoulder, theta_elbow
    except ValueError:
        return 0, 0, 0

def calculate_fk(base_rad, shoulder_rad, elbow_rad):
    """
    Cinemática Direta: Ângulos -> (x,y,z) real
    """
    # Projeção no plano vertical (formado pelo braço)
    # L1
    r1 = LINK_1 * math.cos(shoulder_rad)
    z1 = LINK_1 * math.sin(shoulder_rad)
    
    # L2 (soma o ângulo pois o cotovelo é relativo ao ombro)
    angle_sum = shoulder_rad + elbow_rad
    r2 = LINK_2 * math.cos(angle_sum)
    z2 = LINK_2 * math.sin(angle_sum)
    
    # Coordenadas totais (r = raio horizontal, z = altura)
    r_total = r1 + r2
    z_total = z1 + z2
    
    # Converte polar (r, base_angle) para cartesiano (x, y)
    x = r_total * math.cos(base_rad)
    y = r_total * math.sin(base_rad)
    
    return (x, y, z_total)


# --- Desenho ---
def draw_grid_z_up():
    """ Desenha um grid no chão (z=0) """
    glLineWidth(1.0)
    glColor3f(0.25, 0.25, 0.25)
    
    glBegin(GL_LINES)
    step = 10
    # Linhas paralelas ao eixo X
    for y in range(-step, step + 1):
        glVertex3f(-step, y, 0)
        glVertex3f(step, y, 0)
    # Linhas paralelas ao eixo Y
    for x in range(-step, step + 1):
        glVertex3f(x, -step, 0)
        glVertex3f(x, step, 0)
    glEnd()

    # Eixos RGB (X=R, Y=G, Z=B)
    glLineWidth(3.0)
    glBegin(GL_LINES)
    glColor3f(1, 0, 0); glVertex3f(0,0,0); glVertex3f(3,0,0) # X
    glColor3f(0, 1, 0); glVertex3f(0,0,0); glVertex3f(0,3,0) # Y
    glColor3f(0, 0, 1); glVertex3f(0,0,0); glVertex3f(0,0,3) # Z (Cima)
    glEnd()

def draw_trail(points):
    if len(points) < 2: return
    glDisable(GL_LIGHTING)
    glLineWidth(2.0)
    glBegin(GL_LINE_STRIP)
    for i, p in enumerate(points):
        # Gradiente do ciano para o azul escuro
        alpha = i / len(points)
        glColor4f(0.0, 1.0, 1.0, alpha) 
        glVertex3f(p[0], p[1], p[2])
    glEnd()

def draw_robot(base_rad, shoulder_rad, elbow_rad):
    quadric = gluNewQuadric()
    
    # Ângulos para graus (OpenGL usa graus)
    base_deg = math.degrees(base_rad)
    shoulder_deg = math.degrees(shoulder_rad)
    elbow_deg = math.degrees(elbow_rad)

    # Base (Rotaciona em Z) 
    glPushMatrix()
    glRotatef(base_deg, 0, 0, 1) 
    
    # Desenha a junta da base
    glColor3f(0.7, 0.7, 0.7)
    gluSphere(quadric, 0.4, 16, 16)

    # -- Braço 1 --
    glRotatef(-shoulder_deg, 0, 1, 0) # Eixo Y local, negativo para levantar
    
    # Desenha Braço 1
    glColor3f(0.0, 0.4, 0.8) # Azulão
    glPushMatrix()
    glRotatef(90, 0, 1, 0) # Alinha o cilindro (Z default) com eixo X
    gluCylinder(quadric, 0.15, 0.15, LINK_1, 16, 16)
    glPopMatrix()
    
    # Move para o cotovelo
    glTranslatef(LINK_1, 0, 0) # Move em X local

    # -- Braço 2 --
    glColor3f(0.8, 0.2, 0.2)
    gluSphere(quadric, 0.25, 16, 16) # Junta Cotovelo
    
    glRotatef(-elbow_deg, 0, 1, 0) # Rotação relativa ao ombro
    
    glColor3f(1.0, 0.8, 0.0) # Amarelo
    glPushMatrix()
    glRotatef(90, 0, 1, 0)
    gluCylinder(quadric, 0.1, 0.1, LINK_2, 16, 16)
    glPopMatrix()
    
    glTranslatef(LINK_2, 0, 0)
    
    # -- Ponta --
    glColor3f(0.0, 1.0, 0.0) # Verde
    gluSphere(quadric, 0.12, 16, 16)

    glPopMatrix() # Restaura matriz

# --- Stream de Dados ---
def data_stream_generator():
    """
    Temos que mudar essa função para chamar a função evolutiva
    """
    t = 0
    while True:
        x = 2 + 1.0 * math.cos(t)
        y = 2 + 1.0 * math.sin(t)
        # z = 1.5 + 1.0 * math.sin(t * 3) # Altura varia entre 0.5 e 2.5
        z = 1.0
        t += 0.02
        yield (x, y, z)

# --- Main ---
def main():
    pygame.init()
    display = (800, 600)
    
    # Atributos para suavizar serrilhado
    pygame.display.gl_set_attribute(pygame.GL_MULTISAMPLEBUFFERS, 1)
    pygame.display.gl_set_attribute(pygame.GL_MULTISAMPLESAMPLES, 4)
    
    pygame.display.set_mode(display, DOUBLEBUF | OPENGL)
    pygame.display.set_caption("Simulador Robô (Z-UP Correto)")

    glMatrixMode(GL_PROJECTION)
    glLoadIdentity()
    gluPerspective(45, (display[0]/display[1]), 0.1, 100.0)
    
    glMatrixMode(GL_MODELVIEW)
    glEnable(GL_DEPTH_TEST)
    
    # Habilita Blend para transparência no rastro
    glEnable(GL_BLEND)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)

    glClearColor(0.15, 0.15, 0.18, 1.0) # Fundo cinza técnico

    camera = Camera()
    stream = data_stream_generator()
    clock = pygame.time.Clock()
    
    trail_points = deque(maxlen=200)
    target_pos = (2.0, 0.0, 2.0)

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            camera.handle_input(event)

        # Pega dados (X, Y, Z) da stream
        try:
            target_pos = next(stream)
        except StopIteration:
            pass

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        camera.apply()
        
        draw_grid_z_up()

        # Calcula Cinemática (Matemática pura, sem OpenGL)
        angles_rad = calculate_ik(*target_pos)
        
        # Calcula Posição REAL da ponta (Forward Kinematics) para o rastro
        tip_pos_world = calculate_fk(*angles_rad)
        trail_points.append(tip_pos_world)

        # Renderiza
        draw_trail(trail_points)
        draw_robot(*angles_rad)

        # Desenha o alvo (Fantasma branco)
        glPushMatrix()
        glColor4f(1, 1, 1, 0.5)
        glTranslatef(target_pos[0], target_pos[1], target_pos[2])
        gluSphere(gluNewQuadric(), 0.08, 8, 8)
        glPopMatrix()

        pygame.display.flip()
        clock.tick(60)

    pygame.quit()

if __name__ == "__main__":
    main()