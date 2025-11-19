<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
	import { io } from 'socket.io-client';
	import Map from '$lib/components/Map.svelte';
	import ParkingCard from '$lib/components/ParkingCard.svelte';
	import { Geolocation } from '@capacitor/geolocation';
	
	const API_URL = import.meta.env.VITE_API_URL || 'https://192.168.1.103:8443';
	
	interface ParkingLocation {
		id: number;
		name: string;
		latitude: number;
		longitude: number;
		address: string;
		status: string;
		total_spots: number;
		available_spots: number;
		occupancy_rate: number;
	}
	
	let locations: ParkingLocation[] = [];
	let userLocation: { lat: number; lon: number } | null = null;
	let loading = true;
	let error: string | null = null;
	let socket: any = null;
	let selectedLocation: ParkingLocation | null = null;
	let showRoute = false;
	let isSearchingRoute = false;
	let selectionMethod: 'nearest' | 'manual' | null = null;
	
	async function getUserLocation() {
		try {
			console.log('Requesting user location...');
			const position = await Geolocation.getCurrentPosition({
				enableHighAccuracy: true,
				timeout: 10000,
				maximumAge: 0
			});
			userLocation = {
				lat: position.coords.latitude,
				lon: position.coords.longitude
			};
			console.log('User location acquired:', userLocation);
		} catch (err: unknown) {
			console.error('Error getting location:', err);
			
			// Type guard for GeolocationPositionError
			const geoError = err as { code?: number; message?: string };
			
			if (geoError.message) {
				console.error('Error message:', geoError.message);
			}
			
			// Check if it's a permission error
			if (geoError.code === 1) {
				console.warn('Location permission denied by user');
			} else if (geoError.code === 2) {
				console.warn('Location position unavailable');
			} else if (geoError.code === 3) {
				console.warn('Location request timeout');
			}
			
			// Default to Timisoara if location not available
			userLocation = { lat: 45.7489, lon: 21.2087 };
			console.log('Using default location (Timisoara):', userLocation);
		}
	}
	
	async function fetchLocations(retryCount = 0) {
		try {
			loading = true;
			// Always fetch all locations first to show in database
			let url = `${API_URL}/api/parking/status`;
			
			console.log(`Attempting to fetch from: ${url} (attempt ${retryCount + 1})`);
			
			const response = await fetch(url);
			if (!response.ok) throw new Error(`HTTP ${response.status}: ${response.statusText}`);
			
			const data = await response.json();
			// Transform the data to match frontend expectations
			locations = data.map((device: any) => ({
				id: device.id,
				name: device.name,
				latitude: device.latitude,
				longitude: device.longitude,
				address: device.location || device.name,
				status: device.status || 'registered',
				total_spots: device.total_spots || 0,
				available_spots: device.available_spots || 0,
				occupancy_rate: device.occupancy_rate || 0
			})).filter((location: ParkingLocation) => 
				// Only show locations that have valid coordinates
				location.latitude && location.longitude
			);
			
			console.log('Successfully loaded locations:', locations.length);
			error = null;
		} catch (err) {
			console.error('Error fetching locations:', err);
			
			// Auto-retry on first load (up to 3 times with increasing delays)
			if (retryCount < 3) {
				const delay = (retryCount + 1) * 1000; // 1s, 2s, 3s delays
				console.log(`Retrying in ${delay}ms...`);
				setTimeout(() => fetchLocations(retryCount + 1), delay);
				return;
			}
			
			error = `Failed to load parking locations. Please check your connection. (${err instanceof Error ? err.message : 'Unknown error'})`;
		} finally {
			loading = false;
		}
	}
	
	function initWebSocket() {
		socket = io(API_URL, {
			transports: ['websocket', 'polling'],
			reconnection: true,
			reconnectionDelay: 1000,
			reconnectionDelayMax: 5000
		});
		
		socket.on('connect', () => {
			console.log('✓ WebSocket connected to', API_URL);
		});
		
		socket.on('parking_update', (data: any[]) => {
			console.log('📡 Received parking update:', data);
			// Transform backend data to frontend format
			locations = data.map((device: any) => ({
				id: device.id,
				name: device.name,
				latitude: device.latitude,
				longitude: device.longitude,
				address: device.location || device.name,
				status: device.status || 'registered',
				total_spots: device.total_spots || 0,
				available_spots: device.available_spots || 0,
				occupancy_rate: device.occupancy_rate || 0
			})).filter((location: ParkingLocation) => 
				location.latitude && location.longitude
			);
			console.log(`✓ Updated ${locations.length} locations:`, locations.map(l => `${l.name}: ${l.available_spots}/${l.total_spots} (${l.status})`));
		});
		
		// Handle new device registration
		socket.on('device_registered', (data: any) => {
			console.log('New device registered:', data);
			// Refresh locations to show new device
			fetchLocations();
		});
		
		// Handle sensor registration
		socket.on('sensor_registered', (data: any) => {
			console.log('New sensor registered:', data);
			// Refresh locations to update sensor count
			fetchLocations();
		});
		
		// Handle real-time sensor updates
		socket.on('sensor_update', (data: any) => {
			console.log('Sensor update:', data);
			// Refresh locations to get updated sensor data
			fetchLocations();
		});
		
		// Handle device status updates
		socket.on('device_update', (data: any) => {
			console.log('Device status update:', data);
			// Update device status in the locations array
			const locationIndex = locations.findIndex(loc => loc.id === data.device_id);
			if (locationIndex !== -1) {
				locations[locationIndex].status = data.status;
				locations = [...locations]; // Trigger reactivity
				console.log(`Updated device ${data.device_id} status to ${data.status}`);
			}
		});
		
		socket.on('disconnect', () => {
			console.log('WebSocket disconnected');
		});
		
		socket.on('error', (err: any) => {
			console.error('WebSocket error:', err);
		});
	}
	
	onMount(async () => {
		console.log('Page mounted, initializing...');
		
		// Small delay to ensure page is fully loaded
		await new Promise(resolve => setTimeout(resolve, 100));
		
		await getUserLocation();
		await fetchLocations();
		
		// Small delay before websocket to ensure backend is ready
		setTimeout(() => initWebSocket(), 500);
	});
	
	onDestroy(() => {
		if (socket) {
			socket.disconnect();
		}
	});
	
	function calculateDistance(lat1: number, lon1: number, lat2: number, lon2: number): number {
		const R = 6371; // Radius of Earth in km
		const dLat = (lat2 - lat1) * Math.PI / 180;
		const dLon = (lon2 - lon1) * Math.PI / 180;
		const a = 
			Math.sin(dLat/2) * Math.sin(dLat/2) +
			Math.cos(lat1 * Math.PI / 180) * Math.cos(lat2 * Math.PI / 180) *
			Math.sin(dLon/2) * Math.sin(dLon/2);
		const c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
		return R * c;
	}

	function findNearestParking() {
		if (!userLocation) {
			alert('Location not available. Please enable location services.');
			return;
		}

		// Filter active locations (have available spots)
		const activeLocations = locations.filter(location => location.available_spots > 0);
		
		if (activeLocations.length === 0) {
			alert('No available parking spots found anywhere. All parking spots are currently occupied or inactive.');
			return;
		}

		// Find the nearest active location
		let nearest = activeLocations[0];
		let minDistance = calculateDistance(
			userLocation.lat, userLocation.lon, 
			nearest.latitude, nearest.longitude
		);

		for (const location of activeLocations) {
			const distance = calculateDistance(
				userLocation.lat, userLocation.lon,
				location.latitude, location.longitude
			);
			
			if (distance < minDistance) {
				minDistance = distance;
				nearest = location;
			}
		}

		// Set selected location and show route
		selectedLocation = nearest;
		showRoute = true;
		isSearchingRoute = true;
		selectionMethod = 'nearest';
	}

	function handleLocationSelected(event: CustomEvent) {
		const location = event.detail;
		selectedLocation = location;
		showRoute = true;
		isSearchingRoute = true;
		selectionMethod = 'manual';
		console.log('Manually selected location:', location.name);
	}

	function cancelRoute() {
		selectedLocation = null;
		showRoute = false;
		isSearchingRoute = false;
		selectionMethod = null;
	}
	
	$: sortedLocations = userLocation 
		? [...locations].sort((a, b) => {
				const distA = calculateDistance(userLocation!.lat, userLocation!.lon, a.latitude, a.longitude);
				const distB = calculateDistance(userLocation!.lat, userLocation!.lon, b.latitude, b.longitude);
				return distA - distB;
			})
		: locations;
</script>

<svelte:head>
	<title>FindSpot - Smart Parking</title>
	<meta name="description" content="Find available parking spots near you" />
</svelte:head>

<div class="app">
	<header>
		<div class="container">
			<h1>FindSpot</h1>
			<p class="subtitle">Don't stress about parking</p>
		</div>
	</header>
	
	<main class="main-content">
		{#if loading}
			<div class="loading">
				<div class="spinner"></div>
				<p>Loading parking locations...</p>
			</div>
		{:else if error}
			<div class="error">
				<p>{error}</p>
				<button on:click={() => fetchLocations()}>Retry</button>
			</div>
		{:else}
			<div class="map-section">
				<Map 
					{locations} 
					{userLocation} 
					{selectedLocation} 
					{showRoute}
					on:locationSelected={handleLocationSelected}
				/>
				
				<div class="map-controls">
					{#if !isSearchingRoute}
						<button 
							class="find-parking-btn"
							on:click={findNearestParking}
							disabled={!userLocation}
						>
							Find Nearest Parking
						</button>
					{:else}
						<div class="route-info">
							<div class="route-details">
								<h3>📍 Route to: {selectedLocation?.name}</h3>
								<p>{selectedLocation?.address}</p>
								<p><strong>{selectedLocation?.available_spots}/{selectedLocation?.total_spots}</strong> spots available</p>
								<p class="selection-method">
									{selectionMethod === 'nearest' ? 'Auto-selected (nearest)' : 'Manually selected'}
								</p>
							</div>
							<button class="cancel-btn" on:click={cancelRoute}>
								Cancel Route
							</button>
						</div>
					{/if}
				</div>
			</div>
			
			<div class="locations-section">
				<h2>Nearby Parking ({sortedLocations.length})</h2>
				
				{#if sortedLocations.length === 0}
					<p class="no-data">No parking locations available at the moment.</p>
				{:else}
					<div class="locations-grid">
						{#each sortedLocations as location (location.id)}
							<ParkingCard {location} {userLocation} />
						{/each}
					</div>
				{/if}
			</div>
		{/if}
	</main>
</div>

<style>
	:global(body) {
		margin: 0;
		padding: 0;
		font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
		background: #f5f7fa;
		overflow: hidden;
	}
	
	:global(html) {
		overflow: hidden;
	}
	
	.app {
		height: 100vh;
		display: flex;
		flex-direction: column;
		overflow: hidden;
	}
	
	header {
		background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
		color: white;
		padding: 0.75rem 0;
		box-shadow: 0 2px 8px rgba(0,0,0,0.1);
		flex-shrink: 0;
	}
	
	h1 {
		margin: 0;
		font-size: 1.5rem;
		font-weight: 700;
	}
	
	.subtitle {
		margin: 0.15rem 0 0 0;
		opacity: 0.9;
		font-size: 0.85rem;
	}
	
	.container {
		max-width: 1200px;
		margin: 0 auto;
		padding: 0 1rem;
		width: 100%;
	}
	
	main {
		flex: 1;
		padding: 0;
		overflow: hidden;
		display: flex;
		flex-direction: column;
	}

	.main-content {
		max-width: 1200px;
		margin: 0 auto;
		padding: 1.5rem 1rem;
		width: 100%;
		flex: 1;
		overflow: hidden;
		display: flex;
		flex-direction: column;
	}
	
	.loading, .error {
		text-align: center;
		padding: 3rem 1rem;
		margin: 0 1rem;
	}
	
	.spinner {
		border: 4px solid #f3f3f3;
		border-top: 4px solid #2563eb;
		border-radius: 50%;
		width: 50px;
		height: 50px;
		animation: spin 1s linear infinite;
		margin: 0 auto 1rem;
	}
	
	@keyframes spin {
		0% { transform: rotate(0deg); }
		100% { transform: rotate(360deg); }
	}
	
	.error {
		background: #fee2e2;
		border: 1px solid #ef4444;
		border-radius: 8px;
		color: #991b1b;
	}
	
	.error button {
		margin-top: 1rem;
		padding: 0.5rem 1.5rem;
		background: #ef4444;
		color: white;
		border: none;
		border-radius: 6px;
		cursor: pointer;
		font-size: 1rem;
	}
	
	.map-section {
		margin-bottom: 2rem;
		border-radius: 0;
		overflow: hidden;
		box-shadow: none;
		position: relative;
		flex: 1;
		min-height: 0;
	}

	.locations-section {
		margin-top: 2rem;
		padding: 0 1rem;
		overflow-y: auto;
		flex-shrink: 0;
		max-height: 400px;
	}

	.locations-section h2 {
		margin: 0 0 1rem 0;
		color: #1f2937;
	}

	.map-controls {
		position: absolute;
		bottom: 1rem;
		left: 1rem;
		right: 1rem;
		z-index: 1000;
	}

	.find-parking-btn {
		width: 100%;
		padding: 1rem 1.5rem;
		background: linear-gradient(135deg, #10b981 0%, #059669 100%);
		color: white;
		border: none;
		border-radius: 12px;
		font-size: 1.1rem;
		font-weight: 600;
		cursor: pointer;
		box-shadow: 0 4px 12px rgba(16, 185, 129, 0.3);
		transition: all 0.2s ease;
	}

	.find-parking-btn:hover:not(:disabled) {
		transform: translateY(-2px);
		box-shadow: 0 6px 16px rgba(16, 185, 129, 0.4);
	}

	.find-parking-btn:disabled {
		background: #9ca3af;
		cursor: not-allowed;
		box-shadow: none;
	}

	.route-info {
		background: white;
		border-radius: 12px;
		padding: 1rem;
		box-shadow: 0 4px 12px rgba(0,0,0,0.15);
		display: flex;
		justify-content: space-between;
		align-items: center;
		gap: 1rem;
	}

	.route-details h3 {
		margin: 0 0 0.25rem 0;
		font-size: 1rem;
		color: #1f2937;
	}

	.route-details p {
		margin: 0.125rem 0;
		font-size: 0.875rem;
		color: #6b7280;
	}

	.selection-method {
		font-size: 0.8rem !important;
		font-style: italic;
		color: #2563eb !important;
		margin-top: 0.25rem !important;
	}

	.cancel-btn {
		padding: 0.75rem 1rem;
		background: #ef4444;
		color: white;
		border: none;
		border-radius: 8px;
		font-size: 0.9rem;
		font-weight: 500;
		cursor: pointer;
		transition: all 0.2s ease;
		white-space: nowrap;
	}

	.cancel-btn:hover {
		background: #dc2626;
		transform: translateY(-1px);
	}
	
	.locations-grid {
		display: grid;
		grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
		gap: 1rem;
	}

	.no-data {
		text-align: center;
		padding: 2rem;
		color: #6b7280;
	}
	
	@media (max-width: 640px) {
		h1 {
			font-size: 1.5rem;
		}
		
		.locations-grid {
			grid-template-columns: 1fr;
		}

		.map-controls {
			bottom: 0.5rem;
			left: 0.5rem;
			right: 0.5rem;
		}

		.route-info {
			flex-direction: row;
			align-items: center;
			gap: 0.75rem;
		}

		.route-details {
			text-align: left;
			flex: 1;
		}

		.cancel-btn {
			width: auto;
			flex-shrink: 0;
		}

		/* Hide locations section on mobile to focus on map */
		.locations-section {
			display: none;
		}

		.main-content {
			padding: 0;
			margin: 0;
			max-width: none;
		}

		.map-section {
			margin: 0;
			border-radius: 0;
			flex: 1;
			height: 100%;
		}

		main {
			padding: 0;
		}

		/* Remove container padding on mobile for header too */
		header .container {
			padding: 0 1rem;
		}
	}
</style>

