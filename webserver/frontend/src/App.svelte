<script>
  import { onMount } from 'svelte';
  import { connected, meter } from './lib/stores.js';
  import { wsClient } from './lib/websocket.js';
  import { audioPlayer } from './lib/audioPlayer.js';
  import Waterfall from './components/Waterfall.svelte';
  import VfoDisplay from './components/VfoDisplay.svelte';

  let gatewayUrl = `ws://${window.location.hostname}:8765`;
  let sMeter = -120;

  meter.subscribe(m => {
    sMeter = m.s;
  });

  onMount(async () => {
    // Initialize audio
    await audioPlayer.init();

    // Connect audio callback
    wsClient.onAudio = (rxId, samples) => {
      audioPlayer.addSamples(samples);
    };

    // Connect to gateway
    wsClient.connect(gatewayUrl);
  });

  function formatSMeter(dbm) {
    if (dbm >= -73) return 'S9+' + Math.round(dbm + 73) + 'dB';
    const s = Math.max(0, Math.round((dbm + 127) / 6));
    return 'S' + s;
  }
</script>

<main>
  <header>
    <h1>piHPSDR Web</h1>
    <div class="status" class:connected={$connected}>
      {$connected ? 'Connected' : 'Disconnected'}
    </div>
  </header>

  <section class="controls">
    <VfoDisplay />
    <div class="meter">
      <span class="label">S-Meter:</span>
      <span class="value">{formatSMeter(sMeter)}</span>
      <span class="dbm">({sMeter.toFixed(0)} dBm)</span>
    </div>
  </section>

  <section class="display">
    <Waterfall width={800} height={400} waterfallHeight={250} />
  </section>
</main>

<style>
  main {
    padding: 20px;
    max-width: 1000px;
    margin: 0 auto;
  }

  header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
  }

  h1 {
    color: #4CAF50;
    margin: 0;
  }

  .status {
    padding: 6px 12px;
    border-radius: 4px;
    background: #ff5252;
    color: white;
    font-weight: bold;
  }

  .status.connected {
    background: #4CAF50;
  }

  .controls {
    display: flex;
    gap: 20px;
    align-items: center;
    margin-bottom: 20px;
    flex-wrap: wrap;
  }

  .meter {
    background: #2a2a2a;
    padding: 15px 20px;
    border-radius: 8px;
  }

  .meter .label {
    color: #888;
    margin-right: 10px;
  }

  .meter .value {
    font-size: 1.5rem;
    font-weight: bold;
    color: #4CAF50;
  }

  .meter .dbm {
    color: #666;
    margin-left: 10px;
  }

  .display {
    margin-top: 20px;
  }
</style>
