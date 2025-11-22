#!/bin/bash

echo "🚀 Iniciando la instalación de ftpack..."


# Crear directorios del sistema y mover archivos
echo "📂 Instalando archivos en el sistema..."
mkdir -p /etc/ftpack
cp packages.json /etc/ftpack/
cp config.json /etc/ftpack/       # <-- ¡LÍNEA AÑADIDA!
chmod 644 /etc/ftpack/packages.json
chmod 644 /etc/ftpack/config.json # <-- Buena práctica también

mv ftpack /usr/local/bin/
chmod +x /usr/local/bin/ftpack

echo "✅ ¡Instalación completada!"
echo ""
echo "Ahora puedes usar 'ftpack' desde cualquier lugar en tu terminal."
echo "Prueba ejecutando: ftpack help"
echo "O actualiza el catálogo con: ftpack -update"