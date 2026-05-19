import sys
import time
import threading


# =========================
# CONFIGURACION DEL USUARIO
# =========================

ADAFRUIT_IO_USERNAME = "jrdagta"
ADAFRUIT_IO_KEY = "aio_fqKv94tuHFrpCxmAz4M0PUZtArUX"

SERIAL_PORT = "COM4"
BAUD_RATE = 9600

# Feeds recomendados en Adafruit IO.
FEED_COMANDO = "carro-comando"
FEED_MODO = "carro-modo"
FEED_MOTOR = "carro-motor"
FEED_DIRECCION = "carro-direccion"
FEED_PUERTA_IZQ = "carro-puerta-izq"
FEED_PUERTA_DER = "carro-puerta-der"
FEED_ESTADO = "carro-estado"

FEEDS_TO_SUBSCRIBE = [
    FEED_COMANDO,
    FEED_MODO,
    FEED_MOTOR,
    FEED_DIRECCION,
    FEED_PUERTA_IZQ,
    FEED_PUERTA_DER,
]

ser = None
client = None
serial_lock = threading.Lock()


def list_available_ports():
    ports = list(list_ports.comports())
    print("\nPuertos seriales detectados:")
    if not ports:
        print("  No se detectaron puertos.")
    for port in ports:
        print(f"  {port.device} - {port.description}")


def open_serial():
    global ser
    try:
        ser = serial.Serial(
            port=SERIAL_PORT,
            baudrate=BAUD_RATE,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=0.2
        )
        time.sleep(2.0)
        print(f"Serial conectado en {SERIAL_PORT} a {BAUD_RATE} baudios.")
    except serial.SerialException as e:
        print(f"No se pudo abrir {SERIAL_PORT}.")
        print(e)
        list_available_ports()
        sys.exit(1)


def read_arduino_lines(duration=0.8):
    """Lee respuestas del Arduino durante un tiempo corto."""
    lines = []
    end_time = time.time() + duration

    while time.time() < end_time:
        try:
            if ser and ser.in_waiting:
                raw = ser.readline()
                text = raw.decode("utf-8", errors="replace").strip()
                if text:
                    lines.append(text)
            else:
                time.sleep(0.03)
        except Exception as e:
            lines.append(f"ERROR leyendo Arduino: {e}")
            break

    return lines


def publish_status(text):
    """Publica estado hacia Adafruit IO y también imprime en consola."""
    print("ESTADO:", text)
    try:
        if client:
            client.publish(FEED_ESTADO, text[:200])
    except Exception as e:
        print("No se pudo publicar estado:", e)


def send_to_arduino(command):
    """Envía comando al Arduino por UART."""
    command = command.strip()
    if not command:
        return

    with serial_lock:
        print(f"\n>> Arduino: {command}")
        try:
            ser.write((command + "\n").encode("utf-8"))
            ser.flush()
        except Exception as e:
            publish_status(f"ERROR serial enviando {command}: {e}")
            return

        responses = read_arduino_lines(duration=0.8)

    if responses:
        for line in responses:
            print("<<", line)
        publish_status(responses[-1])
    else:
        publish_status(f"Comando enviado: {command}")


def clamp_int(payload, min_value, max_value, default_value):
    try:
        value = int(float(str(payload).strip()))
    except ValueError:
        return default_value

    if value < min_value:
        value = min_value
    if value > max_value:
        value = max_value
    return value


def normalize_command(payload):
    cmd = str(payload).strip().upper()

    aliases = {
        "ABRIR": "OPEN",
        "ABRIR PUERTAS": "OPEN",
        "CERRAR": "CLOSE",
        "CERRAR PUERTAS": "CLOSE",
        "PARAR": "STOP",
        "DETENER": "STOP",
        "ADELANTE": "F,150",
        "AVANZAR": "F,150",
        "ATRAS": "B,150",
        "RETROCEDER": "B,150",
        "SECUENCIA": "SEQ1",
        "DEMO": "SEQ1",
        "REPRODUCIR": "PLAY",
        "EEPROM": "PLAY",
        "MANUAL": "M",
        "ADAFRUIT": "A",
        "UART": "A",
    }

    return aliases.get(cmd, cmd)


def handle_feed(feed_id, payload):
    feed_id = str(feed_id)
    payload = str(payload).strip()

    print(f"\nFeed recibido: {feed_id} = {payload}")

    if feed_id == FEED_COMANDO:
        command = normalize_command(payload)
        send_to_arduino(command)

    elif feed_id == FEED_MODO:
        mode = payload.strip().lower()
        if mode in ["manual", "m", "0"]:
            send_to_arduino("M")
        elif mode in ["adafruit", "uart", "a", "u", "1"]:
            send_to_arduino("A")
        elif mode in ["eeprom", "e", "2"]:
            send_to_arduino("PLAY")
        else:
            publish_status(f"Modo no reconocido: {payload}")

    elif feed_id == FEED_MOTOR:
        value = clamp_int(payload, -255, 255, 0)
        if value > 0:
            send_to_arduino(f"F,{value}")
        elif value < 0:
            send_to_arduino(f"B,{abs(value)}")
        else:
            send_to_arduino("STOP")

    elif feed_id == FEED_DIRECCION:
        value = clamp_int(payload, 0, 180, 90)
        send_to_arduino(f"DIR,{value}")

    elif feed_id == FEED_PUERTA_IZQ:
        value = clamp_int(payload, 0, 180, 90)
        send_to_arduino(f"LDOOR,{value}")

    elif feed_id == FEED_PUERTA_DER:
        value = clamp_int(payload, 0, 180, 90)
        send_to_arduino(f"RDOOR,{value}")

    else:
        publish_status(f"Feed no manejado: {feed_id}")


def connected(mqtt_client):
    print("\nConectado a Adafruit IO.")
    for feed in FEEDS_TO_SUBSCRIBE:
        print(f"Suscrito a feed: {feed}")
        mqtt_client.subscribe(feed)

    publish_status("Python bridge conectado")


def disconnected(mqtt_client):
    print("Desconectado de Adafruit IO.")
    sys.exit(1)


def message(mqtt_client, feed_id, payload):
    handle_feed(feed_id, payload)


def main():
    global client

    if ADAFRUIT_IO_USERNAME == "TU_USUARIO" or ADAFRUIT_IO_KEY == "TU_KEY":
        print("Edita ADAFRUIT_IO_USERNAME y ADAFRUIT_IO_KEY antes de ejecutar.")
        print("No compartas tu key en capturas públicas.")
        sys.exit(1)

    open_serial()

    # Mensaje inicial del Arduino si hay datos pendientes.
    initial_lines = read_arduino_lines(duration=1.0)
    if initial_lines:
        print("\nMensajes iniciales Arduino:")
        for line in initial_lines:
            print("<<", line)

    client = MQTTClient(ADAFRUIT_IO_USERNAME, ADAFRUIT_IO_KEY)
    client.on_connect = connected
    client.on_disconnect = disconnected
    client.on_message = message

    print("\nConectando a Adafruit IO...")
    client.connect()
    client.loop_background()

    print("\nPuente activo.")
    print("Cambia widgets en Adafruit IO para controlar el carro.")
    print("Presiona Ctrl+C para salir.\n")

    try:
        while True:
            # Lee mensajes espontáneos del Arduino, si los hubiera.
            with serial_lock:
                lines = read_arduino_lines(duration=0.1)
            for line in lines:
                print("<<", line)
                publish_status(line)
            time.sleep(0.2)

    except KeyboardInterrupt:
        print("\nCerrando puente.")
    finally:
        try:
            if ser:
                ser.close()
        except Exception:
            pass


if __name__ == "__main__":
    main()
