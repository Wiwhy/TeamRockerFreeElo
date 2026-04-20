import tkinter as tk
from tkinter import messagebox
import socket

# --- CONFIGURACIÓN WI-FI (Igual que tu código del Mecanum) ---
ROBOT_IP = "192.168.4.1" 
PORT = 8080
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

def guardar_cambios():
    try:
        kp = float(entry_kp.get())
        kd = float(entry_kd.get())
        
        # Enviamos un paquete con formato "P,0.8,6.0" (P = PID)
        mensaje = f"P,{kp},{kd}\n"
        sock.sendto(mensaje.encode('utf-8'), (ROBOT_IP, PORT))
        
        lbl_estado.config(text=f"✅ Valores guardados en el robot:\nKp={kp} | Kd={kd}", fg="green")
    except ValueError:
        messagebox.showerror("Error Numérico", "Usa el punto para los decimales (ej: 0.5)")

def parar_robot():
    # Enviamos la letra "S" de STOP
    sock.sendto("S\n".encode('utf-8'), (ROBOT_IP, PORT))
    lbl_estado.config(text="🚨 ¡ROBOT DETENIDO! 🚨", fg="red", font=("Arial", 10, "bold"))
    
def reanudar_robot():
    # Enviamos la letra "R" de REANUDAR
    sock.sendto("R\n".encode('utf-8'), (ROBOT_IP, PORT))
    lbl_estado.config(text="▶️ Robot en marcha", fg="blue", font=("Arial", 10, "bold"))

# ==========================================
# DISEÑO DE LA INTERFAZ GRÁFICA
# ==========================================
root = tk.Tk()
root.title("Telemetría Fuzzy PID (Wi-Fi)")
root.geometry("350x420")
root.configure(padx=20, pady=20)

tk.Label(root, text="📶 Conectado por Wi-Fi UDP", font=("Arial", 10, "bold"), fg="gray").pack(pady=5)
tk.Frame(root, height=2, bd=1, relief="sunken").pack(fill="x", pady=10) 

# --- SECCIÓN DE VALORES PID ---
tk.Label(root, text="Kp Base (Fuerza de giro):", font=("Arial", 10)).pack()
entry_kp = tk.Entry(root, justify="center", font=("Arial", 14), bg="#e8f4f8")
entry_kp.insert(0, "0.5")
entry_kp.pack(pady=5)

tk.Label(root, text="Kd Base (Freno/Amortiguador):", font=("Arial", 10)).pack()
entry_kd = tk.Entry(root, justify="center", font=("Arial", 14), bg="#e8f4f8")
entry_kd.insert(0, "5.5")
entry_kd.pack(pady=5)

# --- BOTÓN DE GUARDAR ---
tk.Button(root, text="💾 GUARDAR CAMBIOS", command=guardar_cambios, bg="lightgreen", font=("Arial", 11, "bold"), height=2).pack(pady=10, fill="x")

tk.Frame(root, height=2, bd=1, relief="sunken").pack(fill="x", pady=10)

# --- BOTONES DE CONTROL ---
frame_botones = tk.Frame(root)
frame_botones.pack(fill="x")

tk.Button(frame_botones, text="⏹️ PARAR", command=parar_robot, bg="#ff4d4d", fg="white", font=("Arial", 10, "bold"), height=2, width=12).pack(side="left", padx=5)
tk.Button(frame_botones, text="▶️ REANUDAR", command=reanudar_robot, bg="#4da6ff", fg="white", font=("Arial", 10, "bold"), height=2, width=12).pack(side="right", padx=5)

# --- ESTADO ---
lbl_estado = tk.Label(root, text="Esperando instrucciones...", fg="black", font=("Arial", 9))
lbl_estado.pack(pady=15)

root.mainloop()