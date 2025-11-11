<script lang="ts">
	import { onMount } from 'svelte';
	import { browser } from '$app/environment';
	import { createEventDispatcher } from 'svelte';
	
	const dispatch = createEventDispatcher();
	
	export let locations: any[];
	export let userLocation: { lat: number; lon: number } | null;
	export let selectedLocation: any = null;
	export let showRoute: boolean = false;
	
	let mapContainer: HTMLElement;
	let map: any;
	let markers: any[] = [];
	let userMarker: any = null;
	let routeControl: any = null;
	let L: any;
	
	onMount(async () => {
		if (!browser) return;
		
		// Dynamically import Leaflet only on the client side
		const leafletModule = await import('leaflet');
		L = leafletModule.default;
		
		// Import CSS dynamically
		await import('leaflet/dist/leaflet.css');
		
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
		
		// Add user location as a circle if available
		if (userLocation) {
			updateUserLocation();
		}
		
		updateMarkers();
		
		return () => {
			if (map) {
				map.remove();
			}
		};
	});

	function updateUserLocation() {
		if (!map || !L || !userLocation) return;
		
		// Remove existing user marker
		if (userMarker) {
			userMarker.remove();
		}
		
		// Add user location as a blue circle
		userMarker = L.circleMarker([userLocation.lat, userLocation.lon], {
			color: '#2563eb',
			fillColor: '#3b82f6',
			fillOpacity: 0.7,
			radius: 10,
			weight: 3
		}).addTo(map).bindPopup('📍 Your Location');
	}
	
	function updateMarkers() {
		if (!map || !L) return;
		
		// Clear existing markers (but keep user location)
		markers.forEach(marker => marker.remove());
		markers = [];
		
		// Add markers for each parking location
		locations.forEach(location => {
			// Determine marker color based on availability
			const availability = location.total_spots > 0 ? location.available_spots / location.total_spots : 0;
			const isActive = location.available_spots > 0;
			
			let color = 'grey'; // Default for inactive/no spots
			if (isActive) {
				color = availability > 0.5 ? 'green' : availability > 0.2 ? 'orange' : 'red';
			}
			
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
								<div style="width: ${availability * 100}%; height: 100%; background: ${color};"></div>
							</div>
							<div style="margin-top: 0.25rem; font-size: 0.8rem; color: #666;">
								Status: ${isActive ? 'Active' : 'Inactive'}
							</div>
						</div>
						${isActive ? `
							<button 
								onclick="window.selectParkingLocation(${location.id})"
								style="
									width: 100%; 
									margin-top: 0.5rem; 
									padding: 0.5rem; 
									background: #2563eb; 
									color: white; 
									border: none; 
									border-radius: 6px; 
									cursor: pointer;
									font-size: 0.9rem;
									font-weight: 500;
								"
								onmouseover="this.style.background='#1d4ed8'"
								onmouseout="this.style.background='#2563eb'"
							>
								🧭 Navigate Here
							</button>
						` : ''}
					</div>
				`);
			
			// Remove the automatic click handler - only popup should show
			// marker.on('click', () => {
			// 	if (isActive) {
			// 		selectLocation(location);
			// 	}
			// });
			
			markers.push(marker);
		});
		
		// Make selectParkingLocation available globally for popup buttons
		if (typeof window !== 'undefined') {
			(window as any).selectParkingLocation = (locationId: number) => {
				const location = locations.find(l => l.id === locationId);
				if (location && location.available_spots > 0) {
					selectLocation(location);
				}
			};
		}
	}

	function selectLocation(location: any) {
		console.log('User selected parking location:', location.name);
		
		// Close any open popups
		if (map) {
			map.closePopup();
		}
		
		dispatch('locationSelected', location);
	}

	async function showRouteToLocation(location: any) {
		if (!map || !L || !userLocation) return;
		
		// Clear existing route
		clearRoute();
		
		try {
			console.log('Requesting route from', userLocation, 'to', location);
			
			// Use OSRM (Open Source Routing Machine) - more reliable and no API key needed
			const response = await fetch(
				`https://router.project-osrm.org/route/v1/driving/${userLocation.lon},${userLocation.lat};${location.longitude},${location.latitude}?overview=full&geometries=geojson`,
				{
					method: 'GET',
					headers: {
						'Accept': 'application/json'
					}
				}
			);
			
			console.log('Routing response status:', response.status);
			
			if (response.ok) {
				const data = await response.json();
				console.log('Routing data:', data);
				
				if (data.routes && data.routes.length > 0) {
					const routeCoords = data.routes[0].geometry.coordinates;
					console.log('Route coordinates count:', routeCoords.length);
					
					// Convert [lon, lat] to [lat, lon] for Leaflet
					const leafletCoords = routeCoords.map((coord: number[]) => [coord[1], coord[0]]);
					
					routeControl = L.polyline(leafletCoords, {
						color: '#2563eb',
						weight: 5,
						opacity: 0.8,
						smoothFactor: 1
					}).addTo(map);
					
					console.log('✅ Real road route loaded successfully with', leafletCoords.length, 'points');
					
					// Show route distance and duration
					const distance = (data.routes[0].distance / 1000).toFixed(1); // km
					const duration = Math.round(data.routes[0].duration / 60); // minutes
					console.log(`Route: ${distance}km, ${duration}min`);
					
					// Center map on user location for driving view
					if (userLocation) {
						map.setView([userLocation.lat, userLocation.lon], 16); // Zoom level 16 for driving
						console.log('🎯 Map centered on user location for navigation');
					}
					
				} else {
					throw new Error('No route found in response');
				}
			} else {
				throw new Error(`Routing service error: ${response.status}`);
			}
		} catch (error) {
			console.error('❌ Routing failed:', error);
			console.log('🔄 Using fallback straight-line route');
			
			// Fallback to straight line if routing service fails
			const routeCoords = [
				[userLocation.lat, userLocation.lon],
				[location.latitude, location.longitude]
			];
			
			routeControl = L.polyline(routeCoords, {
				color: '#ef4444',
				weight: 4,
				opacity: 0.7,
				dashArray: '10, 10'
			}).addTo(map);
			
			// Even with fallback, center on user location
			if (userLocation) {
				map.setView([userLocation.lat, userLocation.lon], 16);
				console.log('🎯 Map centered on user location (fallback route)');
			}
		}
		
		// Remove the old fit bounds code that showed both points
		// const group = L.featureGroup([userMarker, ...markers.filter(m => 
		// 	m.getLatLng().lat === location.latitude && m.getLatLng().lng === location.longitude
		// )]);
		// map.fitBounds(group.getBounds().pad(0.1));
	}

	function clearRoute() {
		if (routeControl) {
			routeControl.remove();
			routeControl = null;
		}
		
		// Reset map view to user location with moderate zoom
		if (userLocation && map) {
			map.setView([userLocation.lat, userLocation.lon], 13);
			console.log('🗺️ Map reset to overview mode');
		}
	}

	// Reactive updates
	$: if (map && locations && L) {
		updateMarkers();
	}

	$: if (map && userLocation && L) {
		updateUserLocation();
	}

	$: if (map && selectedLocation && showRoute && L) {
		showRouteToLocation(selectedLocation);
	}

	$: if (map && !showRoute && L) {
		clearRoute();
	}
</script>

<div bind:this={mapContainer} class="map"></div>

<style>
	.map {
		width: 100%;
		height: 400px;
		z-index: 0;
	}
	
	/* Mobile full-screen map */
	@media (max-width: 640px) {
		.map {
			height: calc(100vh - 140px); /* Full screen minus header */
			min-height: 500px;
		}
	}
	
	:global(.leaflet-popup-content-wrapper) {
		border-radius: 8px;
	}
	
	:global(.leaflet-popup-content) {
		margin: 12px;
	}
</style>
