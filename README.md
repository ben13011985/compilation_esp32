# ESP32 AP OTA

Projet ESP32 qui crée son propre point d'accès Wi-Fi pour OTA.  
LED clignote pendant OTA et passe en blink normal après timeout.

## Paramètres

- Point d'accès : `test_OTA`  
- Mot de passe : `1234`  
- IP fixe : `192.168.1.123`  
- Timeout OTA : 1 minute  
- LED : GPIO 2  
- Blink normal : 1 s

## Compilation avec Arduino IDE

1. Ouvrir `ESP32_AP_OTA.ino` dans Arduino IDE
2. Choisir la carte ESP32 : **ESP32 Dev Module**
3. Partition scheme : **Default 4MB with OTA**
4. Compiler et uploader via USB pour la première fois
5. Après démarrage, connecter un téléphone au Wi-Fi `test_OTA`
6. Ouvrir un navigateur → `http://192.168.1.123`
7. Choisir le fichier `.bin` si mise à jour OTA

## Structure du code

- `setup()` : démarre AP, serveur web OTA, initialise LED
- `loop()` : gère le timeout OTA et le blink LED
- LED rapide pendant OTA, LED lente après timeout

## Notes

- Ne jamais uploader un `.bin` qui ne contient pas le code OTA, sinon tu perds la capacité OTA
- `.gitignore` inclus pour ignorer les fichiers temporaires Arduino
