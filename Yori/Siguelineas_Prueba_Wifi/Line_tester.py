import tkinter as tk
from tkinter import messagebox
import socket

# --- CONFIGURACIÓN WI-FI ---
ROBOT_IP = "192.168.4.1" 
PORT = 8080
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def guardar_cambios():
    try:
        vel = int(entry_vel.get())
        v_p = float(entry_v_p.get())
        v_d = float(entry_v_d.get())
        a_p = float(entry_a_p.get())
        a_d = float(entry_a_d.get())
        r_p = float(entry_r_p.get())
        r_d = float(entry_r_d.get())
        
        # Enviamos la velocidad general seguida de los 6 valores PID directos
        mensaje = f"F,{vel},{v_p},{v_d},{a_p},{a_d},{r_p},{r_d}\n"
        sock.sendto(mensaje.encode('utf-8'), (ROBOT_IP, PORT))
        lbl_estado.config(text=f"✅ ¡Datos enviados! Velocidad: {vel}", fg="green")
    except ValueError:
        messagebox.showerror("Error", "Revisa los números. Usa puntos para decimales.")

def parar_robot(event=None):
    sock.sendto("S\n".encode('utf-8'), (ROBOT_IP, PORT))
    lbl_estado.config(text="🚨 ¡ROBOT DETENIDO! 🚨", fg="red")
    
def reanudar_robot():
    sock.sendto("R\n".encode('utf-8'), (ROBOT_IP, PORT))
    lbl_estado.config(text="▶️ Robot en marcha (PID)", fg="blue")

def girar_derecha_press(event):
    sock.sendto("D\n".encode('utf-8'), (ROBOT_IP, PORT))
    lbl_estado.config(text="🔄 Girando derecha...", fg="#ff9900")

def ir_adelante_press(event):
    sock.sendto("A\n".encode('utf-8'), (ROBOT_IP, PORT))
    lbl_estado.config(text="⬆️ Avanzando recto...", fg="purple")

# ================= DISEÑO =================
root = tk.Tk()
root.title("Control MODO DIOS")
root.geometry("450x650")
root.configure(padx=10, pady=10)

tk.Label(root, text="⚙️ TELEMETRÍA AVANZADA", font=("Arial", 12, "bold")).pack()

# --- VELOCIDAD GENERAL ---
f_vel = tk.LabelFrame(root, text="Velocidad General (Max 255)", fg="purple", font=("Arial", 10, "bold"))
f_vel.pack(fill="x", pady=5, ipadx=5, ipady=5)
tk.Label(f_vel, text="Velocidad Base:").pack(side="left", padx=10)
entry_vel = tk.Entry(f_vel, width=10, justify="center"); entry_vel.insert(0, "210"); entry_vel.pack(side="left")

# --- FRAMES PID DIRECTO ---
f_verde = tk.LabelFrame(root, text="1. Zona Verde (Recta)", fg="green", font=("Arial", 10, "bold"))
f_verde.pack(fill="x", pady=5, ipadx=5, ipady=5)
tk.Label(f_verde, text="P Verde:").grid(row=0, column=0, padx=5); entry_v_p = tk.Entry(f_verde, width=8); entry_v_p.insert(0, "0.2"); entry_v_p.grid(row=0, column=1)
tk.Label(f_verde, text="D Verde:").grid(row=0, column=2, padx=15); entry_v_d = tk.Entry(f_verde, width=8); entry_v_d.insert(0, "5.0"); entry_v_d.grid(row=0, column=3)

f_ama = tk.LabelFrame(root, text="2. Zona Amarilla (Curva)", fg="#b38f00", font=("Arial", 10, "bold"))
f_ama.pack(fill="x", pady=5, ipadx=5, ipady=5)
tk.Label(f_ama, text="P Ama:").grid(row=0, column=0, padx=5); entry_a_p = tk.Entry(f_ama, width=8); entry_a_p.insert(0, "0.4"); entry_a_p.grid(row=0, column=1)
tk.Label(f_ama, text="D Ama:").grid(row=0, column=2, padx=15); entry_a_d = tk.Entry(f_ama, width=8); entry_a_d.insert(0, "10.0"); entry_a_d.grid(row=0, column=3)

f_rojo = tk.LabelFrame(root, text="3. Zona Roja (Peligro)", fg="red", font=("Arial", 10, "bold"))
f_rojo.pack(fill="x", pady=5, ipadx=5, ipady=5)
tk.Label(f_rojo, text="P Roja:").grid(row=0, column=0, padx=5); entry_r_p = tk.Entry(f_rojo, width=8); entry_r_p.insert(0, "1.0"); entry_r_p.grid(row=0, column=1)
tk.Label(f_rojo, text="D Roja:").grid(row=0, column=2, padx=15); entry_r_d = tk.Entry(f_rojo, width=8); entry_r_d.insert(0, "15.0"); entry_r_d.grid(row=0, column=3)

tk.Button(root, text="💾 ENVIAR VALORES AL ROBOT", command=guardar_cambios, bg="#d9f2d9", font=("Arial", 11, "bold"), height=2).pack(fill="x", pady=10)

# --- BOTONES DE CONTROL ---
f_ctrl = tk.Frame(root)
f_ctrl.pack(fill="x", pady=5)

tk.Button(f_ctrl, text="⏹️ PARAR", command=parar_robot, bg="#ff4d4d", fg="white", font=("Arial", 10, "bold"), height=2, width=15).grid(row=0, column=0, padx=10)
tk.Button(f_ctrl, text="▶️ REANUDAR (PID)", command=reanudar_robot, bg="#4da6ff", fg="white", font=("Arial", 10, "bold"), height=2, width=15).grid(row=0, column=1, padx=10)

btn_adelante = tk.Button(f_ctrl, text="⬆️ SOLO ADELANTE (Mantener)", bg="#cc99ff", font=("Arial", 10, "bold"), height=2, width=33)
btn_adelante.grid(row=1, column=0, columnspan=2, pady=5)
btn_adelante.bind('<ButtonPress-1>', ir_adelante_press)
btn_adelante.bind('<ButtonRelease-1>', parar_robot)

btn_der = tk.Button(f_ctrl, text="↪️ GIRAR DER. (Mantener)", bg="#ffcc00", font=("Arial", 10, "bold"), height=2, width=33)
btn_der.grid(row=2, column=0, columnspan=2, pady=5)
btn_der.bind('<ButtonPress-1>', girar_derecha_press)
btn_der.bind('<ButtonRelease-1>', parar_robot)

lbl_estado = tk.Label(root, text="Esperando instrucciones...", font=("Arial", 10, "bold"))
lbl_estado.pack(pady=5)

root.mainloop()