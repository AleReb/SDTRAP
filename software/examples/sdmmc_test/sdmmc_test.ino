#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>

#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Selecciona una placa ESP32-S3 en Arduino IDE."
#endif

// Completar cuando se defina la placa y el cableado real.
constexpr int SD_CLK = -1;
constexpr int SD_CMD = -1;
constexpr int SD_D0 = -1;
constexpr bool ENABLE_WRITE_TEST = false;
constexpr uint32_t SD_FREQUENCY_KHZ = 10000;
constexpr uint8_t MAX_DEPTH = 3;
constexpr unsigned MAX_ENTRIES = 200;
constexpr char TEST_PATH[] = "/SDTRAP_TEST.bin";

unsigned entries = 0;
String firstJpeg;

bool listDirectory(const char *path, uint8_t depth) {
  File dir = SD_MMC.open(path, FILE_READ);
  if (!dir || !dir.isDirectory()) {
    Serial.printf("[ERROR] No se puede listar %s\n", path);
    return false;
  }
  bool ok = true;
  while (entries < MAX_ENTRIES) {
    File entry = dir.openNextFile();
    if (!entry) break;
    ++entries;
    String child = entry.path();
    bool directory = entry.isDirectory();
    Serial.printf("[%s] %s (%lu bytes)\n", directory ? "DIR" : "FILE",
                  child.c_str(), static_cast<unsigned long>(entry.size()));
    entry.close();
    if (directory && depth > 0) {
      if (!listDirectory(child.c_str(), depth - 1)) ok = false;
    } else if (!directory && firstJpeg.isEmpty()) {
      String lower = child;
      lower.toLowerCase();
      if (lower.endsWith(".jpg") || lower.endsWith(".jpeg")) firstJpeg = child;
    }
    delay(1);
  }
  dir.close();
  return ok;
}

bool readJpeg() {
  if (firstJpeg.isEmpty()) {
    Serial.println("[SKIP] No se encontro JPG dentro de los limites del listado.");
    return true;
  }
  File file = SD_MMC.open(firstJpeg.c_str(), FILE_READ);
  if (!file || file.isDirectory()) {
    Serial.println("[ERROR] No se puede abrir el JPG.");
    return false;
  }
  const size_t expected = file.size();
  size_t total = 0;
  uint8_t buffer[1024];
  const uint32_t start = millis();
  while (total < expected) {
    const size_t remaining = expected - total;
    const size_t wanted = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
    const size_t received = file.read(buffer, wanted);
    if (received == 0 || received > wanted) break;
    total += received;
    delay(1);
  }
  file.close();
  const bool ok = expected > 0 && total == expected;
  Serial.printf("[%s] Lectura %s: %lu/%lu bytes, %lu ms\n",
                ok ? "OK" : "ERROR", firstJpeg.c_str(),
                static_cast<unsigned long>(total), static_cast<unsigned long>(expected),
                static_cast<unsigned long>(millis() - start));
  return ok;
}

bool writeReadTest() {
  if (!ENABLE_WRITE_TEST) {
    Serial.println("[SKIP] Escritura desactivada por configuracion.");
    return true;
  }
  if (SD_MMC.exists(TEST_PATH)) {
    Serial.printf("[ERROR] Ya existe %s; no se modifica.\n", TEST_PATH);
    return false;
  }
  File file = SD_MMC.open(TEST_PATH, FILE_WRITE);
  if (!file) {
    Serial.println("[ERROR] No se puede crear el archivo de prueba.");
    return false;
  }
  uint8_t block[512];
  bool ok = true;
  for (unsigned b = 0; b < 8 && ok; ++b) {
    for (unsigned i = 0; i < sizeof(block); ++i) block[i] = (i + b * 31) & 0xff;
    ok = file.write(block, sizeof(block)) == sizeof(block);
  }
  file.flush();
  file.close();
  file = SD_MMC.open(TEST_PATH, FILE_READ);
  ok = ok && file && file.size() == 4096;
  for (unsigned b = 0; b < 8 && ok; ++b) {
    ok = file.read(block, sizeof(block)) == sizeof(block);
    for (unsigned i = 0; i < sizeof(block) && ok; ++i) {
      ok = block[i] == static_cast<uint8_t>((i + b * 31) & 0xff);
    }
  }
  file.close();
  Serial.println(ok ? "[OK] 4096 bytes escritos y verificados byte a byte."
                    : "[ERROR] Fallo en escritura o verificacion.");
  // Solo se elimina el archivo creado por esta ejecucion.
  if (!SD_MMC.remove(TEST_PATH)) {
    Serial.println("[ERROR] No se pudo eliminar el archivo de prueba.");
    ok = false;
  }
  return ok;
}

void setup() {
  Serial.begin(115200);
  const uint32_t start = millis();
  while (!Serial && millis() - start < 3000) delay(10);
  Serial.println("\nSDTRAP: prueba SD_MMC de 1 bit");
  if (SD_CLK < 0 || SD_CMD < 0 || SD_D0 < 0 ||
      SD_CLK == SD_CMD || SD_CLK == SD_D0 || SD_CMD == SD_D0) {
    Serial.println("[ERROR] Configura SD_CLK, SD_CMD y SD_D0 con GPIO distintos y libres.");
    return;
  }
  if (!SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0)) {
    Serial.println("[ERROR] SD_MMC rechazo la asignacion de pines.");
    return;
  }
  // 1 bit, sin formatear al fallar; margen para directorios anidados.
  if (!SD_MMC.begin("/sdcard", true, false, SD_FREQUENCY_KHZ, 8)) {
    Serial.println("[ERROR] Montaje: revisar alimentacion, pines, pull-ups y formato FAT.");
    SD_MMC.end();
    return;
  }
  if (SD_MMC.cardType() == CARD_NONE) {
    Serial.println("[ERROR] No hay tarjeta.");
    SD_MMC.end();
    return;
  }
  Serial.printf("[OK] SD montada. Tipo=%u; capacidad=%llu MiB; volumen=%llu MiB; usados=%llu MiB\n",
                static_cast<unsigned>(SD_MMC.cardType()),
                static_cast<unsigned long long>(SD_MMC.cardSize() / 1048576),
                static_cast<unsigned long long>(SD_MMC.totalBytes() / 1048576),
                static_cast<unsigned long long>(SD_MMC.usedBytes() / 1048576));
  bool ok = listDirectory("/", MAX_DEPTH);
  if (entries >= MAX_ENTRIES) Serial.println("[INFO] Listado limitado a 200 entradas.");
  if (!readJpeg()) ok = false;
  if (!writeReadTest()) ok = false;
  SD_MMC.end();
  Serial.println(ok ? "[OK] Pruebas habilitadas terminadas; SD desmontada."
                    : "[ERROR] Hubo fallos; SD desmontada.");
  Serial.println("Reinicia la placa para repetir.");
}

void loop() {
  delay(100);
}
