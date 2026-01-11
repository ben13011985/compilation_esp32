```markdown
# OTA_ESP32_BM

Ce dépôt contient un exemple minimal PlatformIO pour construire et flasher un firmware `.bin` pour un ESP32 WROOM, avec prise en charge d'OTA (ArduinoOTA).

Prérequis
- PlatformIO CLI (`pip install platformio`) ou PlatformIO dans VSCode.
- Câble USB (pour le premier flash si nécessaire).
- Réseau Wi‑Fi pour l'OTA.
- (Optionnel) GitHub CLI `gh` pour créer le repo depuis la ligne de commande.

Structure
- `platformio.ini` : configuration PlatformIO
- `src/main.cpp` : exemple avec WiFi + ArduinoOTA + blink
- `src/secrets.example.h` : modèle pour créer `src/secrets.h` (ne pas commiter `secrets.h`)
- `.gitignore`
- `LICENSE` (MIT)

Installer et builder
1. Créer `src/secrets.h` à partir de `src/secrets.example.h` et renseigner `WIFI_SSID` et `WIFI_PASS`.
2. Construire :
   ```
   pio run
   ```
3. Récupérer la .bin :
   ```
   .pio/build/esp32dev/firmware.bin
   ```

Uploader (USB)
- Avec PlatformIO :
  ```
  pio run -t upload
  ```
  (ou `pio run -t upload -e esp32dev --upload-port /dev/ttyUSB0`)

Uploader Over‑The‑Air (OTA)
- Option A (PlatformIO espota) :
  - Décommentez et ajustez les lignes `upload_protocol = espota` et `upload_port = <device_ip>` dans `platformio.ini` (ou utilisez la ligne de commande `pio run -t upload --upload-port <device_ip>`).
  - Puis : `pio run -t upload`

- Option B (esptool) : pas applicable pour OTA. Utilisez PlatformIO/espota ou un outil compatible.

Notes
- Copiez `src/secrets.example.h` vers `src/secrets.h` et remplissez `WIFI_SSID`/`WIFI_PASS`.
- Le nom d'hôte peut être modifié dans `secrets.h` (HOSTNAME).
- Le `board = esp32dev` convient pour la plupart des modules WROOM. Si vous avez une carte spécifique, indiquez-le et j'ajusterai.

Licence
- MIT
```