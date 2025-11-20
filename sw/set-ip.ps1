# Smart Parking System - IP Address Configuration Script
# ========================================================
# This script updates the IP address in:
# - Frontend .env (VITE_API_URL)
# - Backend .env (MQTT_BROKER)
# - Hardware env.h (BACKEND_HOST)
# 
# Usage:
#   .\set-ip.ps1                    # Auto-detect local IP
#   .\set-ip.ps1 192.168.1.105      # Set specific IP address

param(
    [string]$NewIP = ""
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

# Colors for output
function Write-Success { Write-Host $args -ForegroundColor Green }
function Write-Info { Write-Host $args -ForegroundColor Cyan }
function Write-Warn { Write-Host $args -ForegroundColor Yellow }
function Write-Err { Write-Host $args -ForegroundColor Red }

# Function to auto-detect local IP address
function Get-LocalIPAddress {
    try {
        # Get all network adapters with IPv4 addresses
        $adapters = Get-NetIPAddress -AddressFamily IPv4 | 
                    Where-Object { 
                        $_.IPAddress -notlike "127.*" -and 
                        $_.IPAddress -notlike "169.254.*" -and
                        $_.PrefixOrigin -eq "Dhcp" -or $_.PrefixOrigin -eq "Manual"
                    } |
                    Sort-Object -Property InterfaceIndex
        
        if ($adapters.Count -eq 0) {
            Write-Err "No active network connection found"
            exit 1
        }
        
        # Prefer WiFi or Ethernet adapter
        $preferredAdapter = $adapters | Where-Object { 
            $if = Get-NetAdapter -InterfaceIndex $_.InterfaceIndex
            $if.InterfaceDescription -match "Wi-Fi|Wireless|Ethernet|802.11"
        } | Select-Object -First 1
        
        if ($null -eq $preferredAdapter) {
            $preferredAdapter = $adapters | Select-Object -First 1
        }
        
        $detectedIP = $preferredAdapter.IPAddress
        $interfaceAlias = (Get-NetAdapter -InterfaceIndex $preferredAdapter.InterfaceIndex).InterfaceAlias
        
        Write-Info "Auto-detected IP: $detectedIP (on $interfaceAlias)"
        return $detectedIP
    }
    catch {
        Write-Err "Failed to detect local IP address: $_"
        exit 1
    }
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
    
    Write-Info "Using provided IP address: $NewIP"
    $IP = $NewIP
} else {
    # Auto-detect IP
    $IP = Get-LocalIPAddress
}

Write-Info ""
Write-Info "Applying IP address to configuration files..."
Write-Info ""

# 1. Update frontend .env (VITE_API_URL)
$frontendEnv = Join-Path $scriptDir "findspot-frontend\.env"
Update-FileIP -FilePath $frontendEnv `
    -Pattern 'VITE_API_URL=http://[0-9.]+:5000' `
    -Replacement "VITE_API_URL=http://$($IP):5000" `
    -Description "Frontend .env (VITE_API_URL)"

# 2. Update backend .env (MQTT_BROKER)
$backendEnv = Join-Path $scriptDir "findspot-backend\.env"
if (Test-Path $backendEnv) {
    Update-FileIP -FilePath $backendEnv `
        -Pattern 'MQTT_BROKER=[0-9.]+' `
        -Replacement "MQTT_BROKER=$IP" `
        -Description "Backend .env (MQTT_BROKER)"
} else {
    Write-Warn "Backend .env not found - skipping MQTT_BROKER update"
}

# 3. Update hardware env.h (BACKEND_HOST, WIFI_SSID, WIFI_PASS)
$hwEnvH = Join-Path (Split-Path -Parent $scriptDir) "hw\src\env.h"
if (Test-Path $hwEnvH) {
    $content = Get-Content $hwEnvH -Raw
    $newContent = $content
    
    # Update BACKEND_HOST
    $newContent = $newContent -replace '#define BACKEND_HOST\s+"[0-9.]+"', "#define BACKEND_HOST             ""$IP"""
    Write-Success "Updated Hardware env.h (BACKEND_HOST)"
    
    # Update WIFI_SSID if provided
    if ($WiFiSSID) {
        $newContent = $newContent -replace '#define WIFI_SSID\s+"[^"]+"', "#define WIFI_SSID ""$WiFiSSID"""
        Write-Success "Updated Hardware env.h (WIFI_SSID)"
    }
    
    # Update WIFI_PASS if provided
    if ($WiFiPass) {
        $newContent = $newContent -replace '#define WIFI_PASS\s+"[^"]+"', "#define WIFI_PASS ""$WiFiPass"""
        Write-Success "Updated Hardware env.h (WIFI_PASS)"
    }
    
    # Write back only if something changed
    if ($content -ne $newContent) {
        $newContent | Set-Content $hwEnvH -NoNewline
    }
} else {
    Write-Warn "Hardware env.h not found - skipping updates"
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
