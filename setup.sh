#!/bin/bash

# ESP32 MQTT Project Setup Script
# This script sets up the secrets.h file for new developers

set -e  # Exit on any error

echo "🚀 ESP32 MQTT Project Setup"
echo "==========================="

# Check if secrets.h already exists
if [ -f "include/secrets.h" ]; then
    echo "⚠️  secrets.h already exists!"
    read -p "Do you want to overwrite it? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "❌ Setup cancelled."
        exit 1
    fi
fi

# Check if template exists
if [ ! -f "include/secrets.template.h" ]; then
    echo "❌ Error: include/secrets.template.h not found!"
    echo "   This template file is required for setup."
    exit 1
fi

# Copy template to secrets.h
echo "📋 Copying template to secrets.h..."
cp include/secrets.template.h include/secrets.h

echo "✅ secrets.h created successfully!"
echo ""
echo "📝 Next steps:"
echo "1. Edit include/secrets.h with your credentials:"
echo "   - WiFi SSID and password"
echo "   - MQTT broker details"
echo "   - Device configuration"
echo ""
echo "2. Build the project:"
echo "   pio run"
echo ""
echo "3. Upload to your ESP32:"
echo "   pio run --target upload"
echo ""
echo "⚠️  IMPORTANT: Never commit secrets.h to Git!"
echo "   It's already in .gitignore for your safety."
echo ""
echo "🎉 Setup complete! Happy coding!"
