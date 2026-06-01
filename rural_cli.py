import sys
import os
import urllib.request
import urllib.parse
import json
import urllib.error
import platform

GREEN = "\033[92m"
RESET = "\033[0m"

print(GREEN + "=========================================" + RESET)
print(GREEN + " RuralKV CLI (Cliente Interactivo)" + RESET)
print(GREEN + "=========================================" + RESET)
print(GREEN + "Escribe HELP para ver comandos disponibles.\n" + RESET)

URL_BASE = "http://localhost:8080"


def print_help():
    print("Comandos disponibles:")
    print("  PUT <llave> <valor>   – guarda un valor en RuralKV")
    print("  GET <llave>           – obtiene un valor de RuralKV")
    print("  DEL <llave>           – borra una clave")
    print("  EXISTS <llave>        – comprueba si existe una clave")
    print("  KEYS                  – lista todas las claves")
    print("  EXPIRE <llave> <s>    – pone TTL en segundos")
    print("  TTL <llave>           – muestra segundos restantes")
    print("  PING                  – prueba la conexión")
    print("  INFO                  – muestra información del servidor")
    print("  CLEAR                 – limpia la pantalla")
    print("  HELP                  – muestra esta ayuda")
    print("  EXIT                  – salir del cliente")


def print_memory_usage():
    try:
        import psutil
        mem = psutil.Process().memory_info()
        print(f"RSS: {mem.rss} bytes | VMS: {mem.vms} bytes")
        if hasattr(mem, 'shared'):
            print(f"Compartida: {mem.shared} bytes")
    except ImportError:
        if os.name == 'nt':
            try:
                import ctypes
                import ctypes.wintypes as wintypes

                class PROCESS_MEMORY_COUNTERS(ctypes.Structure):
                    _fields_ = [
                        ('cb', wintypes.DWORD),
                        ('PageFaultCount', wintypes.DWORD),
                        ('PeakWorkingSetSize', ctypes.c_size_t),
                        ('WorkingSetSize', ctypes.c_size_t),
                        ('QuotaPeakPagedPoolUsage', ctypes.c_size_t),
                        ('QuotaPagedPoolUsage', ctypes.c_size_t),
                        ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t),
                        ('QuotaNonPagedPoolUsage', ctypes.c_size_t),
                        ('PagefileUsage', ctypes.c_size_t),
                        ('PeakPagefileUsage', ctypes.c_size_t),
                    ]

                GetCurrentProcess = ctypes.windll.kernel32.GetCurrentProcess
                GetProcessMemoryInfo = ctypes.windll.psapi.GetProcessMemoryInfo
                process = GetCurrentProcess()
                counters = PROCESS_MEMORY_COUNTERS()
                counters.cb = ctypes.sizeof(counters)
                if GetProcessMemoryInfo(process, ctypes.byref(counters), counters.cb):
                    print(f"WorkingSetSize: {counters.WorkingSetSize} bytes | PagefileUsage: {counters.PagefileUsage} bytes")
                else:
                    print("No se pudo obtener el uso de memoria en Windows.")
            except Exception:
                print("No se pudo obtener el uso de memoria en Windows.")
        else:
            try:
                import resource
                usage = resource.getrusage(resource.RUSAGE_SELF)
                rss = usage.ru_maxrss
                if platform.system() != 'Darwin':
                    rss *= 1024
                print(f"RSS: {rss} bytes")
                print(f"Máximo uso de memoria: {usage.ru_maxrss} unidades de OS")
            except Exception:
                print("No se pudo obtener el uso de memoria en este sistema.")

while True:
    try:
        comando = input(f"{GREEN}rural-kv>{RESET} ").strip().split(" ", 2)
        if not comando or comando[0] == "":
            continue
            
        verbo = comando[0].upper()
        
        if verbo == "EXIT":
            print("Desconectando...")
            break
            
        elif verbo == "HELP" or verbo == "?":
            print_help()
            continue

        elif verbo == "CLEAR":
            os.system('cls' if os.name == 'nt' else 'clear')
            continue

        elif verbo == "MEM":
            print_memory_usage()
            continue

        elif verbo == "PUT":
            if len(comando) < 3:
                print("(error) Faltan argumentos. Uso: PUT <llave> <valor>")
                continue
            
            key = urllib.parse.quote_plus(comando[1])
            value = urllib.parse.quote_plus(comando[2])
            url = f"{URL_BASE}/put?k={key}&v={value}"
            with urllib.request.urlopen(url) as response:
                if response.status == 200:
                    print("OK")
                else:
                    print(f"(error) Falla conexión: {response.status}")
                
        elif verbo == "GET":
            if len(comando) < 2:
                print("(error) Falta la llave. Uso: GET <llave>")
                continue
            
            key = urllib.parse.quote_plus(comando[1])
            url = f"{URL_BASE}/get?k={key}"
            try:
                with urllib.request.urlopen(url) as response:
                    if response.status == 200:
                        data = json.loads(response.read().decode('utf-8'))
                        if "valor" in data:
                            print(f"\"{data['valor']}\"")
                        else:
                            print("(nil) o no encontrado")
            except urllib.error.HTTPError as e:
                if e.code == 404:
                    print("(nil)")
                else:
                    print(f"(error) HTTP {e.code}")

        elif verbo == "DEL":
            if len(comando) < 2:
                print("(error) Falta la llave. Uso: DEL <llave>")
                continue
            key = urllib.parse.quote_plus(comando[1])
            url = f"{URL_BASE}/del?k={key}"
            with urllib.request.urlopen(url) as response:
                body = response.read().decode('utf-8')
                print(body)

        elif verbo == "EXISTS":
            if len(comando) < 2:
                print("(error) Falta la llave. Uso: EXISTS <llave>")
                continue
            key = urllib.parse.quote_plus(comando[1])
            url = f"{URL_BASE}/exists?k={key}"
            with urllib.request.urlopen(url) as response:
                print(response.read().decode('utf-8'))

        elif verbo == "KEYS":
            url = f"{URL_BASE}/keys"
            with urllib.request.urlopen(url) as response:
                print(response.read().decode('utf-8'))

        elif verbo == "EXPIRE":
            if len(comando) < 3:
                print("(error) Faltan argumentos. Uso: EXPIRE <llave> <segundos>")
                continue
            key = urllib.parse.quote_plus(comando[1])
            timeout = urllib.parse.quote_plus(comando[2])
            url = f"{URL_BASE}/expire?k={key}&t={timeout}"
            with urllib.request.urlopen(url) as response:
                print(response.read().decode('utf-8'))

        elif verbo == "TTL":
            if len(comando) < 2:
                print("(error) Falta la llave. Uso: TTL <llave>")
                continue
            key = urllib.parse.quote_plus(comando[1])
            url = f"{URL_BASE}/ttl?k={key}"
            with urllib.request.urlopen(url) as response:
                print(response.read().decode('utf-8'))

        elif verbo == "PING":
            url = f"{URL_BASE}/ping"
            with urllib.request.urlopen(url) as response:
                print(response.read().decode('utf-8'))

        elif verbo == "INFO":
            url = f"{URL_BASE}/info"
            with urllib.request.urlopen(url) as response:
                print(response.read().decode('utf-8'))

        else:
            print(f"(error) Comando desconocido '{comando[0]}'")
            
    except urllib.error.URLError as e:
        print(f"(error) No se pudo conectar a RuralKV: {e.reason}")
        print("Asegúrate de que el servidor esté encendido y presiona ENTER para continuar.")
        continue
    except KeyboardInterrupt:
        print("\nUso CTRL+C o EXIT para salir.")
        continue
    except EOFError:
        print("\nDesconectando...")
        break
    except Exception as e:
        print(f"Error: {e}")
        continue
