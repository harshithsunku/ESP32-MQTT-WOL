# ESP32 MQTT Project Setup Script (PowerShell)
# This script sets up the secrets.h file for new developers

Write-Host "🚀 ESP32 MQTT Project Setup" -ForegroundColor Cyan
Write-Host "===========================" -ForegroundColor Cyan
Write-Host ""

# Check if secrets.h already exists
if (Test-Path "include\secrets.h") {
    Write-Host "⚠️  secrets.h already exists!" -ForegroundColor Yellow
    $overwrite = Read-Host "Do you want to overwrite it? (y/N)"
    if ($overwrite -ne "y" -and $overwrite -ne "Y") {
        Write-Host "❌ Setup cancelled." -ForegroundColor Red
        exit 1
    }
}

# Check if template exists
if (-not (Test-Path "include\secrets.template.h")) {
    Write-Host "❌ Error: include\secrets.template.h not found!" -ForegroundColor Red
    Write-Host "   This template file is required for setup." -ForegroundColor Red
    exit 1
}

# Copy template to secrets.h
Write-Host "📋 Copying template to secrets.h..." -ForegroundColor Blue
try {
    Copy-Item "include\secrets.template.h" "include\secrets.h"
    Write-Host "✅ secrets.h created successfully!" -ForegroundColor Green
} catch {
    Write-Host "❌ Error copying file: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "📝 Next steps:" -ForegroundColor Yellow
Write-Host "1. Edit include\secrets.h with your credentials:" -ForegroundColor White
Write-Host "   - WiFi SSID and password" -ForegroundColor Gray
Write-Host "   - MQTT broker details" -ForegroundColor Gray
Write-Host "   - Device configuration" -ForegroundColor Gray
Write-Host ""
Write-Host "2. Build the project:" -ForegroundColor White
Write-Host "   pio run" -ForegroundColor Gray
Write-Host ""
Write-Host "3. Upload to your ESP32:" -ForegroundColor White
Write-Host "   pio run --target upload" -ForegroundColor Gray
Write-Host ""
Write-Host "⚠️  IMPORTANT: Never commit secrets.h to Git!" -ForegroundColor Red
Write-Host "   It's already in .gitignore for your safety." -ForegroundColor Red
Write-Host ""
Write-Host "🎉 Setup complete! Happy coding!" -ForegroundColor Green

# Pause to let user read the output
Write-Host ""
Write-Host "Press any key to continue..." -ForegroundColor Cyan
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
