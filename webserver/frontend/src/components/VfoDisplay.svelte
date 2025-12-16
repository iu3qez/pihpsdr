<script>
  import { vfoA } from '../lib/stores.js';
  import { wsClient } from '../lib/websocket.js';

  const MODES = ['LSB', 'USB', 'DSB', 'CWL', 'CWU', 'FM', 'AM', 'DIGU', 'SPEC', 'DIGL', 'SAM', 'DRM'];

  let frequency = 7074000;
  let mode = 1;

  vfoA.subscribe(v => {
    frequency = v.freq;
    mode = v.mode;
  });

  function formatFrequency(hz) {
    const mhz = hz / 1e6;
    return mhz.toFixed(6);
  }

  function handleWheel(event) {
    event.preventDefault();
    const step = event.shiftKey ? 1000 : 100;
    const delta = event.deltaY > 0 ? -step : step;
    const newFreq = frequency + delta;
    wsClient.setFrequency(0, newFreq);
  }

  function handleModeChange(event) {
    const newMode = parseInt(event.target.value);
    wsClient.setMode(0, newMode);
  }
</script>

<div class="vfo-display" on:wheel={handleWheel}>
  <div class="frequency">
    <span class="mhz">{formatFrequency(frequency).split('.')[0]}</span>
    <span class="decimal">.</span>
    <span class="khz">{formatFrequency(frequency).split('.')[1]}</span>
    <span class="unit">MHz</span>
  </div>

  <select class="mode-select" value={mode} on:change={handleModeChange}>
    {#each MODES as modeName, i}
      <option value={i}>{modeName}</option>
    {/each}
  </select>
</div>

<style>
  .vfo-display {
    display: flex;
    align-items: center;
    gap: 20px;
    padding: 15px 20px;
    background: #2a2a2a;
    border-radius: 8px;
    user-select: none;
  }

  .frequency {
    font-family: 'Courier New', monospace;
    font-size: 2.5rem;
    font-weight: bold;
    cursor: ns-resize;
  }

  .mhz {
    color: #4CAF50;
  }

  .decimal {
    color: #666;
  }

  .khz {
    color: #8BC34A;
  }

  .unit {
    font-size: 1rem;
    color: #666;
    margin-left: 8px;
  }

  .mode-select {
    font-size: 1.2rem;
    padding: 8px 16px;
    background: #333;
    color: #fff;
    border: 1px solid #444;
    border-radius: 4px;
    cursor: pointer;
  }

  .mode-select:hover {
    background: #3a3a3a;
  }
</style>
