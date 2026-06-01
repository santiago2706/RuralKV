import sys
import urllib.request
import json
import urllib.error

print("=========================================")
print(" RuralKV CLI (Cliente Interactivo)")
print("=========================================")
print("Escribe comandos como si fuera Redis.")
print("Ejemplo: PUT DNI_01 Gripe_Alta")
print("Ejemplo: GET DNI_01")
print("Escribe EXIT para salir.\n")

URL_BASE = "http://localhost:8080"

while True:
    try:
        comando = input("rural-kv> ").strip().split(" ", 2)
        if not comando or comando[0] == "":
            continue
            
        verbo = comando[0].upper()
        
        if verbo == "EXIT":
            print("Desconectando...")
            break
            
        elif verbo == "PUT":
            if len(comando) < 3:
                print("(error) Faltan argumentos. Uso: PUT <llave> <valor>")
                continue
            
            url = f"{URL_BASE}/put?k={comando[1]}&v={comando[2]}"
            with urllib.request.urlopen(url) as response:
                if response.status == 200:
                    print("OK")
                else:
                    print(f"(error) Falla conexión: {response.status}")
                
        elif verbo == "GET":
            if len(comando) < 2:
                print("(error) Falta la llave. Uso: GET <llave>")
                continue
            
            url = f"{URL_BASE}/get?k={comando[1]}"
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
                
        else:
            print(f"(error) Comando desconocido '{comando[0]}'")
            
    except urllib.error.URLError:
        print("(error fatal) No se pudo conectar a RuralKV. ¿Seguro que el motor '.exe' está encendido en otra consola?")
        break
    except Exception as e:
        print(f"Error: {e}")
