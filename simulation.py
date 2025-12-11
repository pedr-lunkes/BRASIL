import pygame
from pygame.locals import *
from OpenGL.GL import *
from OpenGL.GLU import *
import numpy as np
from collections import deque
import subprocess
import sys
import threading
import queue
import random
import math

# --- Configurações Físicas ---
LINK_1 = 10.0
LINK_2 = 10.0

# --- Filas de Comunicação ---
trajectory_queue = queue.Queue()
command_queue = queue.Queue()

# --- Classe da Thread do Solver (C++) ---
class SolverThread(threading.Thread):
    def __init__(self):
        super().__init__()
        self.daemon = True
        self.process = None
        self.running = True

    def run(self):
        while self.running:
            try:
                target = command_queue.get(timeout=1.0) 
            except queue.Empty:
                continue

            if self.process and self.process.poll() is None:
                self.process.kill()
                self.process.wait()

            tx, ty, tz = target
            print(f"Solver Thread: Calculando para ({tx:.2f}, {ty:.2f}, {tz:.2f})")

            # ATENÇÃO: Verifique se o nome do executável está correto
            process_name = "./main" if sys.platform != "win32" else "main.exe"
            try:
                self.process = subprocess.Popen(
                    [process_name, str(tx), str(ty), str(tz)],
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    bufsize=1
                )

                current_path = []
                reading_path = False

                while True:
                    if not command_queue.empty(): break 
                    
                    line = self.process.stdout.readline()
                    if not line: break

                    line = line.strip()
                    if line == "START_PATH":
                        current_path = []
                        reading_path = True
                    elif line == "END_PATH":
                        reading_path = False
                        if current_path: trajectory_queue.put(current_path)
                    elif reading_path:
                        try:
                            parts = line.split()
                            if len(parts) == 3:
                                current_path.append(tuple(map(float, parts)))
                        except ValueError: pass
            except FileNotFoundError:
                print("ERRO: Executável não encontrado. Compile o C++!")

# --- Câmera ---
class Camera:
    def __init__(self):
        self.yaw = 45.0  
        self.pitch = 30.0 
        self.distance = 45.0
        self.sensitivity = 0.5
        self.zoom_speed = 1.0

    def handle_input(self, event):
        if event.type == pygame.MOUSEMOTION:
            if pygame.mouse.get_pressed()[0]:
                dx, dy = event.rel
                self.yaw -= dx * self.sensitivity
                self.pitch += dy * self.sensitivity
                self.pitch = max(min(self.pitch, 89.0), -89.0)
        if event.type == pygame.MOUSEWHEEL:
            self.distance -= event.y * self.zoom_speed
            if self.distance < 5.0: self.distance = 5.0

    def apply(self):
        glMatrixMode(GL_MODELVIEW); glLoadIdentity()
        rad_yaw, rad_pitch = math.radians(self.yaw), math.radians(self.pitch)
        cam_x = self.distance * math.cos(rad_pitch) * math.cos(rad_yaw)
        cam_y = self.distance * math.cos(rad_pitch) * math.sin(rad_yaw)
        cam_z = self.distance * math.sin(rad_pitch)
        gluLookAt(cam_x, cam_y, cam_z, 0, 0, 0, 0, 0, 1)

# --- Funções de Desenho Auxiliares ---
def draw_button_gl(x, y, w, h, text, font):
    """
    Desenha um botão corrigindo a orientação da textura.
    """
    # 1. Cria a imagem do botão na CPU
    surf = pygame.Surface((w, h))
    surf.fill((50, 50, 200)) 
    pygame.draw.rect(surf, (255, 255, 255), (0, 0, w, h), 2) 
    
    text_surf = font.render(text, True, (255, 255, 255))
    text_rect = text_surf.get_rect(center=(w/2, h/2))
    surf.blit(text_surf, text_rect)

    # 2. Converte para Textura OpenGL (CORREÇÃO AQUI: "False" para não inverter)
    text_data = pygame.image.tostring(surf, "RGBA", False)
    
    glEnable(GL_TEXTURE_2D)
    tex_id = glGenTextures(1)
    glBindTexture(GL_TEXTURE_2D, tex_id)
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    
    # Upload da textura
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, text_data)

    # 3. Desenha em Modo Ortogonal
    glDisable(GL_DEPTH_TEST) 
    glDisable(GL_LIGHTING)
    glColor4f(1, 1, 1, 1) 

    glMatrixMode(GL_PROJECTION)
    glPushMatrix()
    glLoadIdentity()
    # Define origem (0,0) no canto SUPERIOR esquerdo
    glOrtho(0, 800, 600, 0, -1, 1) 

    glMatrixMode(GL_MODELVIEW)
    glPushMatrix()
    glLoadIdentity()
    glTranslatef(x, y, 0)

    # Desenha o quadrado
    glBegin(GL_QUADS)
    # Como não invertemos a string, mapeamos o topo da textura (0,0) para o topo do quad
    glTexCoord2f(0, 0); glVertex2f(0, 0) # Top-Left
    glTexCoord2f(1, 0); glVertex2f(w, 0) # Top-Right
    glTexCoord2f(1, 1); glVertex2f(w, h) # Bottom-Right
    glTexCoord2f(0, 1); glVertex2f(0, h) # Bottom-Left
    glEnd()

    # 4. Restaura estado
    glPopMatrix() 
    glMatrixMode(GL_PROJECTION)
    glPopMatrix() 
    glMatrixMode(GL_MODELVIEW)
    
    glDeleteTextures([tex_id]) 
    glDisable(GL_TEXTURE_2D)
    glEnable(GL_DEPTH_TEST)

def calculate_ik(target_x, target_y, target_z):
    # Lógica simplificada de IK para visualização
    theta_base = math.atan2(target_y, target_x)
    r_ground = math.sqrt(target_x**2 + target_y**2)
    dist_sq = r_ground**2 + target_z**2
    dist = math.sqrt(dist_sq)
    if dist > (LINK_1 + LINK_2): dist = LINK_1 + LINK_2; dist_sq = dist**2
    
    try:
        alpha = math.atan2(target_z, r_ground)
        cos_shoulder = (LINK_1**2 + dist_sq - LINK_2**2) / (2 * LINK_1 * dist)
        cos_shoulder = max(min(cos_shoulder, 1.0), -1.0)
        theta_shoulder = alpha + math.acos(cos_shoulder)
        cos_elbow = (LINK_1**2 + LINK_2**2 - dist_sq) / (2 * LINK_1 * LINK_2)
        cos_elbow = max(min(cos_elbow, 1.0), -1.0)
        theta_elbow = math.acos(cos_elbow) - math.pi 
        return theta_base, theta_shoulder, theta_elbow
    except: return 0,0,0

def draw_robot(angles):
    base, shoulder, elbow = angles
    quadric = gluNewQuadric()
    glPushMatrix()
    glRotatef(math.degrees(base), 0, 0, 1)
    glColor3f(0.7, 0.7, 0.7); gluSphere(quadric, 0.8, 16, 16)
    glRotatef(-math.degrees(shoulder), 0, 1, 0)
    glColor3f(0.0, 0.4, 0.8)
    glPushMatrix(); glRotatef(90, 0, 1, 0); gluCylinder(quadric, 0.4, 0.4, LINK_1, 16, 16); glPopMatrix()
    glTranslatef(LINK_1, 0, 0)
    glColor3f(0.8, 0.2, 0.2); gluSphere(quadric, 0.6, 16, 16)
    glRotatef(-math.degrees(elbow), 0, 1, 0)
    glColor3f(1.0, 0.8, 0.0)
    glPushMatrix(); glRotatef(90, 0, 1, 0); gluCylinder(quadric, 0.3, 0.3, LINK_2, 16, 16); glPopMatrix()
    glTranslatef(LINK_2, 0, 0)
    glColor3f(0.0, 1.0, 0.0); gluSphere(quadric, 0.3, 16, 16)
    glPopMatrix()

def draw_grid():
    glLineWidth(1.0); glColor3f(0.25, 0.25, 0.25)
    glBegin(GL_LINES)
    for i in range(-20, 21, 2):
        glVertex3f(-20, i, 0); glVertex3f(20, i, 0); glVertex3f(i, -20, 0); glVertex3f(i, 20, 0)
    glEnd()

def draw_trail(points):
    if len(points) < 2: return
    glDisable(GL_LIGHTING); glLineWidth(2.0); glBegin(GL_LINE_STRIP)
    for i, p in enumerate(points):
        glColor4f(0.0, 1.0, 1.0, i/len(points)) 
        glVertex3f(p[0], p[1], p[2])
    glEnd()

# --- Main ---
def main():
    pygame.init()
    display = (800, 600)
    pygame.display.gl_set_attribute(pygame.GL_MULTISAMPLEBUFFERS, 1)
    pygame.display.gl_set_attribute(pygame.GL_MULTISAMPLESAMPLES, 4)
    pygame.display.set_mode(display, DOUBLEBUF | OPENGL)
    pygame.display.set_caption("Simulador Threaded com UI OpenGL")
    
    font = pygame.font.SysFont('Arial', 18, bold=True)

    # Init OpenGL
    glMatrixMode(GL_PROJECTION); glLoadIdentity()
    gluPerspective(45, (display[0]/display[1]), 0.1, 100.0)
    glMatrixMode(GL_MODELVIEW); glEnable(GL_DEPTH_TEST)
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    glClearColor(0.15, 0.15, 0.18, 1.0)

    camera = Camera()
    clock = pygame.time.Clock()
    
    # Inicia Solver
    solver_thread = SolverThread()
    solver_thread.start()

    # Estado Inicial
    target_pos = (15.0, 5.0, 5.0)
    command_queue.put(target_pos)
    trail_points = []
    
    # Definição do Botão
    btn_rect = pygame.Rect(20, 20, 200, 40)

    running = True
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            
            camera.handle_input(event)
            
            # Input do Botão
            if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                mouse_pos = pygame.mouse.get_pos()
                if btn_rect.collidepoint(mouse_pos):
                    # Gera Novo Alvo
                    r = random.uniform(5.0, 18.0)
                    th = random.uniform(0, 2*math.pi)
                    h = random.uniform(0, 15.0)
                    target_pos = (r*math.cos(th), r*math.sin(th), h)
                    
                    print(f"UI: Novo alvo solicitado -> {target_pos}")
                    command_queue.put(target_pos)
                    trail_points = []

        # Atualiza Dados
        try:
            while not trajectory_queue.empty():
                trail_points = trajectory_queue.get_nowait()
        except queue.Empty: pass

        # --- Renderização ---
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        
        # 3D World
        camera.apply()
        draw_grid()
        
        # Alvo (Esfera Grande)
        glPushMatrix()
        glColor4f(1, 1, 1, 0.5)
        glTranslatef(target_pos[0], target_pos[1], target_pos[2])
        gluSphere(gluNewQuadric(), 1.5, 8, 8)
        glPopMatrix()

        # Robô e Rastro
        if trail_points:
            draw_trail(trail_points)
            draw_robot(calculate_ik(*trail_points[-1]))

        # UI 2D (Botão)
        # Atenção: Isso desenha "por cima" do 3D
        draw_button_gl(btn_rect.x, btn_rect.y, btn_rect.w, btn_rect.h, 
                      "NOVO ALVO ALEATORIO", font)

        pygame.display.flip()
        clock.tick(60)

    solver_thread.running = False
    pygame.quit()

if __name__ == "__main__":
    main()