<script lang="ts">
	import { onMount } from 'svelte';
	import L from 'leaflet';
	import 'leaflet/dist/leaflet.css';
	
	export let locations: any[];
	export let userLocation: { lat: number; lon: number } | null;
	
	let mapContainer: HTMLElement;
	let map: L.Map;
	let markers: L.Marker[] = [];
	
	onMount(() => {
		// Initialize map
		const defaultCenter: [number, number] = userLocation 
			? [userLocation.lat, userLocation.lon]
			: [46.7712, 23.6236]; // Cluj-Napoca default
			
		map = L.map(mapContainer).setView(defaultCenter, 13);
		
		// Add OpenStreetMap tiles
		L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
			attribution: '© OpenStreetMap contributors',
			maxZoom: 19
		}).addTo(map);
		
		// Add user location marker if available
		if (userLocation) {
			const userIcon = L.icon({
				iconUrl: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-2x-blue.png',
				shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-shadow.png',
				iconSize: [25, 41],
				iconAnchor: [12, 41],
				popupAnchor: [1, -34],
				shadowSize: [41, 41]
			});
			
			L.marker([userLocation.lat, userLocation.lon], { icon: userIcon })
				.addTo(map)
				.bindPopup('You are here');
		}
		
		updateMarkers();
		
		return () => {
			if (map) {
				map.remove();
			}
		};
	});
	
	function updateMarkers() {
		if (!map) return;
		
		// Clear existing markers
		markers.forEach(marker => marker.remove());
		markers = [];
		
		// Add markers for each parking location
		locations.forEach(location => {
			const availability = location.available_spots / location.total_spots;
			const color = availability > 0.5 ? 'green' : availability > 0.2 ? 'orange' : 'red';
			
			const icon = L.icon({
				iconUrl: `https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-2x-${color}.png`,
				shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/1.7.1/images/marker-shadow.png',
				iconSize: [25, 41],
				iconAnchor: [12, 41],
				popupAnchor: [1, -34],
				shadowSize: [41, 41]
			});
			
			const marker = L.marker([location.latitude, location.longitude], { icon })
				.addTo(map)
				.bindPopup(`
					<div style="min-width: 200px;">
						<h3 style="margin: 0 0 0.5rem 0;">${location.name}</h3>
						<p style="margin: 0.25rem 0; font-size: 0.9rem;">${location.address}</p>
						<div style="margin-top: 0.5rem; padding: 0.5rem; background: #f3f4f6; border-radius: 4px;">
							<strong>${location.available_spots}/${location.total_spots}</strong> spots available
							<div style="margin-top: 0.25rem; background: #ddd; height: 6px; border-radius: 3px; overflow: hidden;">
								<div style="width: ${(location.available_spots / location.total_spots) * 100}%; height: 100%; background: ${color};"></div>
							</div>
						</div>
					</div>
				`);
			
			markers.push(marker);
		});
	}
	
	$: if (map && locations) {
		updateMarkers();
	}
</script>

<div bind:this={mapContainer} class="map"></div>

<style>
	.map {
		width: 100%;
		height: 400px;
		z-index: 0;
	}
	
	:global(.leaflet-popup-content-wrapper) {
		border-radius: 8px;
	}
	
	:global(.leaflet-popup-content) {
		margin: 12px;
	}
</style>
