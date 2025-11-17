<script lang="ts">
	export let location: any;
	export let userLocation: { lat: number; lon: number } | null;
	
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
	
	$: distance = userLocation 
		? calculateDistance(userLocation.lat, userLocation.lon, location.latitude, location.longitude)
		: null;
		
	$: availabilityPercent = (location.available_spots / location.total_spots) * 100;
	$: availabilityClass = availabilityPercent > 50 ? 'high' : availabilityPercent > 20 ? 'medium' : 'low';
</script>

<div class="card">
	<div class="card-header">
		<h3>{location.address}</h3>
		{#if distance !== null}
			<span class="distance">{distance.toFixed(1)} km</span>
		{/if}
	</div>
	
	<p class="address">{location.address}</p>
	
	<div class="availability {availabilityClass}">
		<div class="availability-label">
			<span class="status-icon">🅿️</span>
			<strong>{location.available_spots} / {location.total_spots}</strong>
			<span>spots free</span>
		</div>
		<div class="progress-bar">
			<div class="progress-fill" style="width: {availabilityPercent}%"></div>
		</div>
	</div>
	
	<div class="card-footer">
		<span class="occupancy">
			{location.occupancy_rate}% occupied
		</span>
		<button class="navigate-btn">
			Navigate →
		</button>
	</div>
</div>

<style>
	.card {
		background: white;
		border-radius: 12px;
		padding: 1.25rem;
		box-shadow: 0 2px 8px rgba(0,0,0,0.08);
		transition: transform 0.2s, box-shadow 0.2s;
	}
	
	.card:hover {
		transform: translateY(-2px);
		box-shadow: 0 4px 16px rgba(0,0,0,0.12);
	}
	
	.card-header {
		display: flex;
		justify-content: space-between;
		align-items: flex-start;
		margin-bottom: 0.5rem;
	}
	
	h3 {
		margin: 0;
		font-size: 1.125rem;
		color: #1f2937;
	}
	
	.distance {
		background: #e5e7eb;
		padding: 0.25rem 0.5rem;
		border-radius: 12px;
		font-size: 0.875rem;
		color: #4b5563;
		white-space: nowrap;
	}
	
	.address {
		margin: 0 0 1rem 0;
		color: #6b7280;
		font-size: 0.875rem;
	}
	
	.availability {
		margin-bottom: 1rem;
		padding: 0.75rem;
		border-radius: 8px;
	}
	
	.availability.high {
		background: #d1fae5;
		border: 1px solid #10b981;
	}
	
	.availability.medium {
		background: #fed7aa;
		border: 1px solid #f59e0b;
	}
	
	.availability.low {
		background: #fee2e2;
		border: 1px solid #ef4444;
	}
	
	.availability-label {
		display: flex;
		align-items: center;
		gap: 0.5rem;
		margin-bottom: 0.5rem;
		font-size: 0.95rem;
	}
	
	.status-icon {
		font-size: 1.25rem;
	}
	
	.progress-bar {
		height: 6px;
		background: rgba(0,0,0,0.1);
		border-radius: 3px;
		overflow: hidden;
	}
	
	.progress-fill {
		height: 100%;
		transition: width 0.3s ease;
	}
	
	.availability.high .progress-fill {
		background: #10b981;
	}
	
	.availability.medium .progress-fill {
		background: #f59e0b;
	}
	
	.availability.low .progress-fill {
		background: #ef4444;
	}
	
	.card-footer {
		display: flex;
		justify-content: space-between;
		align-items: center;
	}
	
	.occupancy {
		font-size: 0.875rem;
		color: #6b7280;
	}
	
	.navigate-btn {
		background: #2563eb;
		color: white;
		border: none;
		padding: 0.5rem 1rem;
		border-radius: 6px;
		font-size: 0.875rem;
		font-weight: 500;
		cursor: pointer;
		transition: background 0.2s;
	}
	
	.navigate-btn:hover {
		background: #1d4ed8;
	}
</style>
