import pygame
from pygame.locals import *
from OpenGL.GL import *
from OpenGL.GLU import *
import numpy as np
import subprocess
import sys
import threading
import queue
import random
import multiprocessing

# --- Configurações Físicas ---
LINK_1 = 10.0
LINK_2 = 10.0

# --- Variáveis Globais de Estado da UI ---
current_steps_count = 0

# --- Filas de Comunicação ---
trajectory_queue = queue.Queue()
command_queue = queue.Queue()
graph_data_queue = multiprocessing.Queue() 

# --- Processo de Plotagem (Matplotlib) ---
def graph_process_func(data_q):
    import matplotlib.pyplot as plt
    import matplotlib.animation as animation
    
    def handle_close(evt):
        sys.exit(0)

    fig, ax = plt.subplots()
    fig.canvas.mpl_connect('close_event', handle_close)
    fig.canvas.manager.set_window_title('Evolução do Fitness')
    
    line_best, = ax.plot([], [], 'g-', label='Melhor Fitness')
    line_avg, = ax.plot([], [], 'b--', label='Fitness Médio', alpha=0.6)
    
    ax.set_xlabel('Gerações'); ax.set_ylabel('Fitness'); ax.legend(); ax.grid(True)
    gens, bests, avgs = [], [], []

    def update(frame):
        updated = False
        while not data_q.empty():
            try:
                msg = data_q.get_nowait()
                if msg == "RESET":
                    gens.clear(); bests.clear(); avgs.clear()
                    ax.set_xlim(0, 100); ax.set_ylim(-1000, 1000)
                else:
                    g, b, a = msg
                    gens.append(g); bests.append(b); avgs.append(a); updated = True
            except: pass
        
        if updated:
            line_best.set_data(gens, bests); line_avg.set_data(gens, avgs)
            if gens:
                ax.set_xlim(0, max(gens) * 1.1)
                all_vals = bests + avgs
                min_y = min(all_vals); max_y = max(all_vals)
                margin = (max_y - min_y) * 0.1 if max_y != min_y else 10
                ax.set_ylim(min_y - margin, max_y + margin)
        return line_best, line_avg

    ani = animation.FuncAnimation(fig, update, interval=200, blit=False)
    plt.show()

# --- Classe Thread Solver (C++) ---
class SolverThread(threading.Thread):
    def __init__(self):
        super().__init__()
        self.daemon = True
        self.process = None
        self.running = True

    def run(self):
        global current_steps_count
        while self.running:
            try: target = command_queue.get(timeout=0.5) 
            except queue.Empty: continue

            if self.process and self.process.poll() is None:
                self.process.kill(); self.process.wait()
            
            graph_data_queue.put("RESET")
            tx, ty, tz = target
            
            process_name = "./main" if sys.platform != "win32" else "main.exe"
            try:
                self.process = subprocess.Popen([process_name, str(tx), str(ty), str(tz)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, bufsize=1)
                current_path = []; reading_path = False
                
                while True:
                    if not command_queue.empty(): break 
                    line = self.process.stdout.readline()
                    if not line: break
                    line = line.strip()
                    
                    if line == "START_PATH": current_path = []; reading_path = True
                    elif line == "END_PATH":
                        reading_path = False
                        if current_path: trajectory_queue.put(current_path)
                    elif line.startswith("STATS"):
                        parts = line.split()
                        if len(parts) == 5:
                            current_steps_count = int(parts[4])
                            graph_data_queue.put((int(parts[1]), float(parts[2]), float(parts[3])))
                    elif reading_path:
                        try: current_path.append(tuple(map(float, line.split())))
                        except ValueError: pass
            except FileNotFoundError: print("ERRO: Executável C++ não encontrado.")

# --- Funções Auxiliares ---
def generate_valid_target_fk():
    """ Gera alvo válido via Cinemática Direta """
    angle_base = random.uniform(-np.pi, np.pi)
    angle_shoulder = random.uniform(0, np.pi/2)     
    angle_elbow = random.uniform(0, np.pi)   

    angle_abs = angle_shoulder - angle_elbow
    r1 = LINK_1 * np.cos(angle_shoulder)
    z1 = LINK_1 * np.sin(angle_shoulder)
    
    r2 = LINK_2 * np.cos(angle_abs)
    z2 = LINK_2 * np.sin(angle_abs)
    
    r_total = r1 + r2
    z_total = z1 + z2

    # Converete para coordenadas polares
    x = r_total * np.cos(angle_base)
    y = r_total * np.sin(angle_base)
    z = z_total
    
    return (x, y, z)

def draw_text_gl(x, y, text, font, color=(255, 255, 255)):
    text_surf = font.render(text, True, color)
    text_data = pygame.image.tostring(text_surf, "RGBA", False)
    w, h = text_surf.get_width(), text_surf.get_height()
    glEnable(GL_TEXTURE_2D); tex_id = glGenTextures(1); glBindTexture(GL_TEXTURE_2D, tex_id)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, text_data)
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glColor4f(1, 1, 1, 1)
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, 800, 600, 0, -1, 1)
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glTranslatef(x, y, 0)
    glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex2f(0,0); glTexCoord2f(1,0); glVertex2f(w,0); glTexCoord2f(1,1); glVertex2f(w,h); glTexCoord2f(0,1); glVertex2f(0,h); glEnd()
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW)
    glDeleteTextures([tex_id]); glDisable(GL_TEXTURE_2D); glEnable(GL_DEPTH_TEST)

def draw_button_gl(x, y, w, h, text, font, text_color=(255,255,255)):
    surf = pygame.Surface((w, h)); surf.fill((50, 50, 150)) 
    pygame.draw.rect(surf, (200, 200, 200), (0, 0, w, h), 2) 
    text_surf = font.render(text, True, text_color)
    text_rect = text_surf.get_rect(center=(w/2, h/2))
    surf.blit(text_surf, text_rect)
    text_data = pygame.image.tostring(surf, "RGBA", False)
    glEnable(GL_TEXTURE_2D); tex_id = glGenTextures(1); glBindTexture(GL_TEXTURE_2D, tex_id)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, text_data)
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glColor4f(1,1,1,1)
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0, 800, 600, 0, -1, 1)
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity(); glTranslatef(x, y, 0)
    glBegin(GL_QUADS); glTexCoord2f(0,0); glVertex2f(0,0); glTexCoord2f(1,0); glVertex2f(w,0); glTexCoord2f(1,1); glVertex2f(w,h); glTexCoord2f(0,1); glVertex2f(0,h); glEnd()
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW)
    glDeleteTextures([tex_id]); glDisable(GL_TEXTURE_2D); glEnable(GL_DEPTH_TEST)

# --- Câmera e IK ---
class Camera:
    def __init__(self):
        self.yaw = 45.0; self.pitch = 30.0; self.distance = 45.0
        self.sensitivity = 0.5; self.zoom_speed = 2.0
    def handle_input(self, event):
        if event.type == pygame.MOUSEMOTION and pygame.mouse.get_pressed()[0]:
            dx, dy = event.rel; self.yaw -= dx * self.sensitivity; self.pitch = max(min(self.pitch + dy * self.sensitivity, 89.0), -89.0)
        if event.type == pygame.MOUSEWHEEL: self.distance = max(5.0, self.distance - event.y * self.zoom_speed)
    def apply(self):
        glMatrixMode(GL_MODELVIEW); glLoadIdentity()
        rad_yaw, rad_pitch = np.radians(self.yaw), np.radians(self.pitch)
        gluLookAt(self.distance*np.cos(rad_pitch)*np.cos(rad_yaw), self.distance*np.cos(rad_pitch)*np.sin(rad_yaw), self.distance*np.sin(rad_pitch), 0,0,0, 0,0,1)

def calculate_ik(target_x, target_y, target_z):
    theta_base = np.atan2(target_y, target_x)
    r_ground = np.sqrt(target_x**2 + target_y**2); dist_sq = r_ground**2 + target_z**2; dist = np.sqrt(dist_sq)
    if dist > (LINK_1 + LINK_2 - 0.001): dist = LINK_1 + LINK_2 - 0.001; dist_sq = dist**2 
    try:
        alpha = np.atan2(target_z, r_ground)
        cos_sh = (LINK_1**2 + dist_sq - LINK_2**2) / (2 * LINK_1 * dist); cos_sh = max(min(cos_sh, 1.0), -1.0)
        theta_sh = alpha + np.acos(cos_sh)
        cos_el = (LINK_1**2 + LINK_2**2 - dist_sq) / (2 * LINK_1 * LINK_2); cos_el = max(min(cos_el, 1.0), -1.0)
        theta_el = np.acos(cos_el) - np.pi 
        return theta_base, theta_sh, theta_el
    except: return 0,0,0

def draw_robot(angles, ghost_mode=False):
    base, shoulder, elbow = angles
    quadric = gluNewQuadric()
    if ghost_mode:
        c_base, c_link1, c_joint1, c_link2, c_tip = (0.8,0.8,0.8,0.3), (0.7,0.7,1.0,0.3), (0.8,0.6,0.6,0.3), (1.0,1.0,0.7,0.3), (0.7,1.0,0.7,0.3)
        glEnable(GL_CULL_FACE) 
    else:
        c_base, c_link1, c_joint1, c_link2, c_tip = (0.7,0.7,0.7,1.0), (0.0,0.4,0.8,1.0), (0.8,0.2,0.2,1.0), (1.0,0.8,0.0,1.0), (0.0,1.0,0.0,1.0)
        glDisable(GL_CULL_FACE)

    glPushMatrix()
    glRotatef(np.degrees(base), 0, 0, 1)
    glColor4f(*c_base); gluSphere(quadric, 0.8, 16, 16)
    glRotatef(-np.degrees(shoulder), 0, 1, 0)
    glColor4f(*c_link1); glPushMatrix(); glRotatef(90, 0, 1, 0); gluCylinder(quadric, 0.4, 0.4, LINK_1, 16, 16); glPopMatrix()
    glTranslatef(LINK_1, 0, 0)
    glColor4f(*c_joint1); gluSphere(quadric, 0.6, 16, 16)
    glRotatef(-np.degrees(elbow), 0, 1, 0)
    glColor4f(*c_link2); glPushMatrix(); glRotatef(90, 0, 1, 0); gluCylinder(quadric, 0.3, 0.3, LINK_2, 16, 16); glPopMatrix()
    glTranslatef(LINK_2, 0, 0)
    glColor4f(*c_tip); gluSphere(quadric, 0.3, 16, 16)
    glPopMatrix()
    glDisable(GL_CULL_FACE)

def draw_grid():
    glLineWidth(1.0); glColor3f(0.25, 0.25, 0.25); glBegin(GL_LINES)
    for i in range(-30, 31, 5): glVertex3f(-30, i, 0); glVertex3f(30, i, 0); glVertex3f(i, -30, 0); glVertex3f(i, 30, 0)
    glEnd()

def draw_trail(points):
    if len(points) < 2: return
    glDisable(GL_LIGHTING); glLineWidth(2.0); glBegin(GL_LINE_STRIP)
    for i, p in enumerate(points): glColor4f(0.0, 1.0, 1.0, i/len(points)); glVertex3f(p[0], p[1], p[2])
    glEnd()

# --- Main ---
def main():
    # CORREÇÃO AQUI: Declarar variável global dentro da função
    global current_steps_count
    
    pygame.init()
    display = (800, 600)
    pygame.display.gl_set_attribute(pygame.GL_MULTISAMPLEBUFFERS, 1); pygame.display.gl_set_attribute(pygame.GL_MULTISAMPLESAMPLES, 4)
    pygame.display.set_mode(display, DOUBLEBUF | OPENGL)
    pygame.display.set_caption("Simulador 3D - Evolução Robótica")
    
    font = pygame.font.SysFont('Arial', 16, bold=True)
    font_info = pygame.font.SysFont('Consolas', 16)

    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(45, (display[0]/display[1]), 0.1, 200.0)
    glMatrixMode(GL_MODELVIEW); glEnable(GL_DEPTH_TEST); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    glClearColor(0.15, 0.15, 0.18, 1.0)

    camera = Camera(); clock = pygame.time.Clock()

    graph_process = multiprocessing.Process(target=graph_process_func, args=(graph_data_queue,))
    graph_process.start()
    solver_thread = SolverThread(); solver_thread.start()

    target_pos = generate_valid_target_fk()
    command_queue.put(target_pos)
    trail_points = []
    
    btn_new_target = pygame.Rect(20, 20, 180, 35)
    btn_ghost_mode = pygame.Rect(20, 65, 180, 35)

    show_ghost_mode = False
    running = True

    try:
        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT: running = False
                
                if event.type == pygame.KEYDOWN and event.key == pygame.K_ESCAPE:
                    running = False
                    print("ESC pressionado. Encerrando...")

                camera.handle_input(event)

                if event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                    mouse_pos = pygame.mouse.get_pos()
                    
                    if btn_new_target.collidepoint(mouse_pos):
                        target_pos = generate_valid_target_fk()
                        command_queue.put(target_pos)
                        trail_points = []
                        current_steps_count = 0
                    
                    if btn_ghost_mode.collidepoint(mouse_pos):
                        show_ghost_mode = not show_ghost_mode

            try:
                while not trajectory_queue.empty(): trail_points = trajectory_queue.get_nowait()
            except queue.Empty: pass

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
            camera.apply(); draw_grid()
            
            glPushMatrix(); glColor4f(1, 1, 1, 0.5); glTranslatef(*target_pos); gluSphere(gluNewQuadric(), 1.0, 16, 16); glPopMatrix()

            if show_ghost_mode:
                ideal_angles = calculate_ik(*target_pos)
                draw_robot(ideal_angles, ghost_mode=True)

            if trail_points:
                draw_trail(trail_points)
                draw_robot(calculate_ik(*trail_points[-1]), ghost_mode=False)

            draw_button_gl(btn_new_target.x, btn_new_target.y, btn_new_target.w, btn_new_target.h, "NOVO ALVO", font)
            
            ghost_text = f"FANTASMA: {'ON' if show_ghost_mode else 'OFF'}"
            ghost_color = (100, 255, 100) if show_ghost_mode else (255, 100, 100)
            draw_button_gl(btn_ghost_mode.x, btn_ghost_mode.y, btn_ghost_mode.w, btn_ghost_mode.h, ghost_text, font, ghost_color)
            
            draw_text_gl(20, 110, f"Passos: {current_steps_count}", font_info, (0, 255, 255))

            pygame.display.flip(); clock.tick(60)

    finally:
        print("Limpando processos...")
        solver_thread.running = False
        if graph_process.is_alive():
            graph_process.terminate()
            graph_process.kill()
        pygame.quit()
        print("Encerrado com sucesso.")

if __name__ == "__main__":
    main()