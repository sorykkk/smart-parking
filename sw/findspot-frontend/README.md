# FindSpot Frontend

This directory contains the frontend application for the FindSpot smart parking system, built with SvelteKit.

## Components

- **SvelteKit Application** (`src/`): Modern web application with reactive UI
- **Interactive Map** (`src/lib/components/Map.svelte`): Real-time parking space visualization
- **Nginx Configuration**: Web server setup for production deployment
- **Docker Setup**: Containerized deployment configuration

## Development

For local development without Docker:

```bash
npm install
npm run dev

# or start the server and open the app in a new browser tab
npm run dev -- --open
```

## Building

To create a production version of your app:

```bash
npm run build
```

You can preview the production build with `npm run preview`.

> To deploy your app, you may need to install an [adapter](https://svelte.dev/docs/kit/adapters) for your target environment.

## Production Deployment

The frontend is deployed as part of the main FindSpot system. See the root README.md for deployment instructions.

## Configuration

Frontend configuration is handled through the root `.env` file and connects automatically to the backend API running on the same server.
