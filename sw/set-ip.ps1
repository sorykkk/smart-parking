# Smart Parking System - IP Address Configuration Script
# ========================================================
# This script updates the IP address in:
# - Frontend .env (VITE_API_URL)
# - Backend .env (MQTT_BROKER)
# - Hardware env.h (BACKEND_HOST)
# 
# Usage:
#   .\set-ip.ps1                    # Apply IP from config.env
#   .\set-ip.ps1 192.168.1.105      # Set and apply new IP address

param(
    [string]$NewIP = ""
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$configFile = Join-Path $scriptDir "config.env"

# Colors for output
function Write-Success { Write-Host $args -ForegroundColor Green }
function Write-Info { Write-Host $args -ForegroundColor Cyan }
function Write-Warn { Write-Host $args -ForegroundColor Yellow }
function Write-Err { Write-Host $args -ForegroundColor Red }

# Function to read IP from config.env
function Get-ConfiguredIP {
    if (-not (Test-Path $configFile)) {
        Write-Err "Config file not found: $configFile"
        exit 1
    }
    
    $content = Get-Content $configFile -Raw
    if ($content -match 'IP_ADDRESS=(.+)') {
        return $matches[1].Trim()
    }
    
    Write-Err "IP_ADDRESS not found in config.env"
    exit 1
}

# Function to update config.env with new IP
function Set-ConfiguredIP {
    param([string]$IP)
    
    $content = Get-Content $configFile -Raw
    $content = $content -replace 'IP_ADDRESS=.+', "IP_ADDRESS=$IP"
    $content | Set-Content $configFile -NoNewline
    Write-Success "Updated config.env with IP: $IP"
}

# Function to update a file with new IP
function Update-FileIP {
    param(
        [string]$FilePath,
        [string]$Pattern,
        [string]$Replacement,
        [string]$Description
    )
    
    if (-not (Test-Path $FilePath)) {
        Write-Warn "Skipping $Description - File not found: $FilePath"
        return
    }
    
    $content = Get-Content $FilePath -Raw
    $newContent = $content -replace $Pattern, $Replacement
    
    if ($content -ne $newContent) {
        $newContent | Set-Content $FilePath -NoNewline
        Write-Success "Updated $Description"
    } else {
        Write-Info "  No change needed in $Description"
    }
}

# Main script logic
Write-Info "==================================================="
Write-Info "Smart Parking System - IP Configuration"
Write-Info "==================================================="
Write-Info ""

# Determine which IP to use
if ($NewIP) {
    # Validate IP format (basic check)
    if ($NewIP -notmatch '^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$') {
        Write-Err "Invalid IP address format: $NewIP"
        Write-Info "Expected format: xxx.xxx.xxx.xxx"
        exit 1
    }
    
    Write-Info "Setting new IP address: $NewIP"
    Set-ConfiguredIP -IP $NewIP
    $IP = $NewIP
} else {
    $IP = Get-ConfiguredIP
    Write-Info "Using IP address from config.env: $IP"
}

Write-Info ""
Write-Info "Applying IP address to configuration files..."
Write-Info ""

# 1. Update frontend .env (VITE_API_URL)
$frontendEnv = Join-Path $scriptDir "findspot-frontend\.env"
Update-FileIP -FilePath $frontendEnv `
    -Pattern 'VITE_API_URL=http://[\d\.]+:5000' `
    -Replacement "VITE_API_URL=http://$($IP):5000" `
    -Description "Frontend .env (VITE_API_URL)"

# 2. Update backend .env (MQTT_BROKER)
$backendEnv = Join-Path $scriptDir "findspot-backend\.env"
if (Test-Path $backendEnv) {
    Update-FileIP -FilePath $backendEnv `
        -Pattern 'MQTT_BROKER=[\d\.]+' `
        -Replacement "MQTT_BROKER=$IP" `
        -Description "Backend .env (MQTT_BROKER)"
} else {
    Write-Warn "Backend .env not found - skipping MQTT_BROKER update"
}

# 3. Update hardware env.h (BACKEND_HOST)
$hwEnvH = Join-Path (Split-Path -Parent $scriptDir) "hw\src\env.h"
if (Test-Path $hwEnvH) {
    Update-FileIP -FilePath $hwEnvH `
        -Pattern '#define BACKEND_HOST "[\d\.]+"' `
        -Replacement "#define BACKEND_HOST `"$IP`"" `
        -Description "Hardware env.h (BACKEND_HOST)"
} else {
    Write-Warn "Hardware env.h not found - skipping BACKEND_HOST update"
}

Write-Info ""
Write-Success "==================================================="
Write-Success "IP Configuration Complete!"
Write-Success "==================================================="
Write-Info ""
Write-Info "IP Address: $IP"
Write-Info "Frontend API: http://$($IP):5000"
Write-Info "MQTT Broker: $($IP):1883"
Write-Info "Hardware Backend: $($IP):5000"
Write-Info ""
Write-Info "Next step: Restart your development environment"
Write-Info "  .\start-dev.bat"
Write-Info ""
