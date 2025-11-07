<script lang="ts">
	import { onMount, onDestroy } from 'svelte';
	import { io } from 'socket.io-client';
	import Map from '$lib/components/Map.svelte';
	import ParkingCard from '$lib/components/ParkingCard.svelte';
	import { Geolocation } from '@capacitor/geolocation';
	
	const API_URL = import.meta.env.VITE_API_URL || 'https://your-raspberry-pi-domain.com';
	
	interface ParkingLocation {
		id: number;
		name: string;
		latitude: number;
		longitude: number;
		address: string;
		total_spots: number;
		available_spots: number;
		occupancy_rate: number;
	}
	
	let locations: ParkingLocation[] = [];
	let userLocation: { lat: number; lon: number } | null = null;
	let loading = true;
	let error: string | null = null;
	let socket: any = null;
	
	async function getUserLocation() {
		try {
			const position = await Geolocation.getCurrentPosition();
			userLocation = {
				lat: position.coords.latitude,
				lon: position.coords.longitude
			};
			console.log('User location:', userLocation);
		} catch (err) {
			console.error('Error getting location:', err);
			// Default to Cluj-Napoca if location not available
			userLocation = { lat: 46.7712, lon: 23.6236 };
		}
	}
	
	async function fetchLocations() {
		try {
			loading = true;
			let url = `${API_URL}/api/locations`;
			
			if (userLocation) {
				url += `?lat=${userLocation.lat}&lon=${userLocation.lon}&radius=10`;
			}
			
			const response = await fetch(url);
			if (!response.ok) throw new Error('Failed to fetch locations');
			
			locations = await response.json();
			error = null;
		} catch (err) {
			console.error('Error fetching locations:', err);
			error = 'Failed to load parking locations. Please check your connection.';
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
			console.log('WebSocket connected');
		});
		
		socket.on('parking_update', (data: ParkingLocation[]) => {
			console.log('Received parking update:', data);
			locations = data;
		});
		
		socket.on('disconnect', () => {
			console.log('WebSocket disconnected');
		});
		
		socket.on('error', (err: any) => {
			console.error('WebSocket error:', err);
		});
	}
	
	onMount(async () => {
		await getUserLocation();
		await fetchLocations();
		initWebSocket();
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
	
	$: sortedLocations = userLocation 
		? [...locations].sort((a, b) => {
				const distA = calculateDistance(userLocation.lat, userLocation.lon, a.latitude, a.longitude);
				const distB = calculateDistance(userLocation.lat, userLocation.lon, b.latitude, b.longitude);
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
			<h1>🅿️ FindSpot</h1>
			<p class="subtitle">Find your perfect parking spot</p>
		</div>
	</header>
	
	<main class="container">
		{#if loading}
			<div class="loading">
				<div class="spinner"></div>
				<p>Loading parking locations...</p>
			</div>
		{:else if error}
			<div class="error">
				<p>⚠️ {error}</p>
				<button on:click={fetchLocations}>Retry</button>
			</div>
		{:else}
			<div class="map-section">
				<Map {locations} {userLocation} />
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
	}
	
	.app {
		min-height: 100vh;
		display: flex;
		flex-direction: column;
	}
	
	header {
		background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
		color: white;
		padding: 1.5rem 0;
		box-shadow: 0 2px 8px rgba(0,0,0,0.1);
	}
	
	h1 {
		margin: 0;
		font-size: 2rem;
		font-weight: 700;
	}
	
	.subtitle {
		margin: 0.25rem 0 0 0;
		opacity: 0.9;
		font-size: 0.95rem;
	}
	
	.container {
		max-width: 1200px;
		margin: 0 auto;
		padding: 0 1rem;
		width: 100%;
	}
	
	main {
		flex: 1;
		padding: 1.5rem 1rem;
	}
	
	.loading, .error {
		text-align: center;
		padding: 3rem 1rem;
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
		border-radius: 12px;
		overflow: hidden;
		box-shadow: 0 4px 12px rgba(0,0,0,0.1);
	}
	
	.locations-section h2 {
		margin: 0 0 1rem 0;
		color: #1f2937;
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
	}
</style>

